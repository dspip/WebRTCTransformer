#include <glib.h>
#include <glib-unix.h>
#include <libsoup/soup.h>

//static gchar   *ws_server_addr = "echo.websocket.org";
//static gint     ws_server_port = 80;
static gboolean is_wss = FALSE;
static GMainLoop *main_loop;
#include <locale.h>
#include <glib.h>
#include <glib-unix.h>
#include <gst/gst.h>
#include <gst/sdp/sdp.h>

#define GST_USE_UNSTABLE_API
#include <gst/webrtc/webrtc.h>

#include <libsoup/soup.h>
#include <json-glib/json-glib.h>
#include <string.h>
#include <webrtc.h>

#define RTP_PAYLOAD_TYPE "96"
#define SOUP_HTTP_PORT 57778
#define STUN_SERVER "stun.l.google.com:19302"

static gboolean sig_handler(gpointer data)
{
    g_print ("Caught Signal\n");	
    SoupSession *session = data;
  //  soup_session_abort (session);
    g_main_loop_quit(main_loop);
    return G_SOURCE_REMOVE;
}

static void on_message(SoupWebsocketConnection *conn, gint data_type, GBytes *message, gpointer user_data)
{
  gsize size;
  gchar *data;
  gchar *data_string;
  const gchar *type_string;
  JsonNode *root_json;
  JsonObject *root_json_object;
  JsonObject *data_json_object;
  JsonParser *json_parser = NULL;

  switch (data_type) {
    case SOUP_WEBSOCKET_DATA_BINARY:
      g_error ("Received unknown binary message, ignoring\n");
      g_bytes_unref (message);
      return;

    case SOUP_WEBSOCKET_DATA_TEXT:
      data = g_bytes_unref_to_data (message, &size);
      /* Convert to NULL-terminated string */
      data_string = g_strndup (data, size);
      g_free (data);
      break;

    default:
      g_assert_not_reached ();
  }

  json_parser = json_parser_new ();
  if (!json_parser_load_from_data (json_parser, data_string, -1, NULL))
    goto unknown_message;

  root_json = json_parser_get_root (json_parser);
  if (!JSON_NODE_HOLDS_OBJECT (root_json))
    goto unknown_message;

  root_json_object = json_node_get_object (root_json);

  if (!json_object_has_member (root_json_object, "type")) {
    g_error ("Received message without type field\n");
    goto cleanup;
  }
  type_string = json_object_get_string_member (root_json_object, "type");

  if (!json_object_has_member (root_json_object, "data")) {
    g_error ("Received message without data field\n");
    goto cleanup;
  }
  data_json_object = json_object_get_object_member (root_json_object, "data");

  if (g_strcmp0 (type_string, "sdp") == 0) {
    const gchar *sdp_type_string;
    const gchar *sdp_string;
    GstPromise *promise;
    GstSDPMessage *sdp;
    GstWebRTCSessionDescription *answer;
    int ret;

    if (!json_object_has_member (data_json_object, "type")) {
      g_error ("Received SDP message without type field\n");
      goto cleanup;
    }
    sdp_type_string = json_object_get_string_member (data_json_object, "type");

    if (g_strcmp0 (sdp_type_string, "offer") != 0) {
      g_error ("Expected SDP message type \"offer\", got \"%s\"\n",
          sdp_type_string);
      goto cleanup;
    }

    if (!json_object_has_member (data_json_object, "sdp")) {
      g_error ("Received SDP message without SDP string\n");
      goto cleanup;
    }
    sdp_string = json_object_get_string_member (data_json_object, "sdp");

    g_print ("Received SDP:\n%s\n", sdp_string);
    webrtc_set_remote_sdp (user_data, sdp_string);
  } else if (g_strcmp0 (type_string, "ice") == 0) {
    guint mline_index;
    const gchar *candidate_string;

    if (!json_object_has_member (data_json_object, "sdpMLineIndex")) {
      g_error ("Received ICE message without mline index\n");
      goto cleanup;
    }
    mline_index =
        json_object_get_int_member (data_json_object, "sdpMLineIndex");

    if (!json_object_has_member (data_json_object, "candidate")) {
      g_error ("Received ICE message without ICE candidate string\n");
      goto cleanup;
    }
    candidate_string = json_object_get_string_member (data_json_object,
        "candidate");

    g_print ("Received ICE candidate with mline index %u; candidate: %s\n",
        mline_index, candidate_string);
    webrtc_set_remote_ice (user_data, mline_index, candidate_string);
  } else
    goto unknown_message;

cleanup:
  if (json_parser != NULL)
    g_object_unref (G_OBJECT (json_parser));
  g_free (data_string);
  return;

unknown_message:
  g_error ("Unknown message \"%s\", ignoring", data_string);
  goto cleanup;
}

static void on_close(SoupWebsocketConnection *conn, gpointer data)
{
    g_print("WebSocket connection closed\n");
    soup_websocket_connection_close(conn, SOUP_WEBSOCKET_CLOSE_NORMAL, NULL);
}

void send_message_server (void *message, void *user_data)
{
    printf ("Sending Server Message %s\n", (char *)message);
    SoupWebsocketConnection *conn = user_data;
    soup_websocket_connection_send_text(conn, (char *)message);

}
static void on_connection(SoupSession *session, GAsyncResult *res, gpointer data)
{
    SoupWebsocketConnection *conn;
    GError *error = NULL;

    conn = soup_session_websocket_connect_finish(session, res, &error);
    if (error) {
        g_print("Error: %s\n", error->message);
        g_error_free(error);
        g_main_loop_quit(main_loop);
        return;
    }

    initialize_ctx ();
    void *ctx = receive_webrtc_stream (send_message_server, conn);
    g_signal_connect(conn, "message", G_CALLBACK(on_message), ctx);
    g_signal_connect(conn, "closed",  G_CALLBACK(on_close),   ctx);
   
    g_print ("Connected with server\n");
}

int main(int argc, char **argv)
{
    GError *error = NULL;
    GOptionContext *context;

    context = g_option_context_new("- WebSocket testing client");
   // g_option_context_add_main_entries(context, opt_entries, NULL);
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        g_error_free(error);
        return 1;
    }

    main_loop = g_main_loop_new(NULL, FALSE);



    SoupSession *session;
    SoupMessage *msg;

    // Create the soup session with WS or WSS
    gchar *uri = NULL;
    session = soup_session_new();
    uri = argv[1];
    msg = soup_message_new(SOUP_METHOD_GET, uri);

    soup_session_websocket_connect_async(
            session,
            msg,
            NULL, NULL, NULL,
            (GAsyncReadyCallback)on_connection,
            NULL
    );

    g_unix_signal_add(SIGINT, (GSourceFunc)sig_handler, session);
    g_unix_signal_add(SIGTERM, (GSourceFunc)sig_handler, session);
    g_print("start main loop\n");
    g_main_loop_run(main_loop);

    g_main_loop_unref(main_loop);
    g_print ("Exit Loop\n");
    return 0;
}
