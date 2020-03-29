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


#define SOUP_HTTP_PORT 57778
#define STUN_SERVER "stun.l.google.com:19302"
typedef struct
{
 char *filename;
 int  use_filter;
}SOUP_DATA;

void sendMessage (void *message, void *user_data)
{
  SoupWebsocketConnection * connection = (SoupWebsocketConnection *) user_data;
  g_print ("Sending Message %s %p\n", (char *)message, connection);

  soup_websocket_connection_send_text (connection, message);
  g_print ("Sending Message Done\n");
}

gboolean
exit_sighandler (gpointer user_data)
{
  g_print ("Caught signal, stopping mainloop\n");
  GMainLoop *mainloop = (GMainLoop *) user_data;
  g_main_loop_quit (mainloop);
  return TRUE;
}

const gchar *html_source = " \n \
<html> \n \
  <head> \n \
  <meta charset=\"utf-8\">\n \
    <script type=\"text/javascript\" src=\"https://webrtc.github.io/adapter/adapter-latest.js\"></script> \n \
    <script type=\"text/javascript\"> \n \
      var html5VideoElement; \n \
      var websocketConnection; \n \
      var webrtcPeerConnection; \n \
      var webrtcConfiguration; \n \
      var reportError; \n \
 \n \
 \n \
      function onLocalDescription(desc) { \n \
        console.log(\"Local description: \" + JSON.stringify(desc)); \n \
        webrtcPeerConnection.setLocalDescription(desc).then(function() { \n \
          websocketConnection.send(JSON.stringify({ type: \"sdp\", \"data\": webrtcPeerConnection.localDescription })); \n \
        }).catch(reportError); \n \
      } \n \
 \n \
 \n \
      function onIncomingSDP(sdp) { \n \
        console.log(\"Incoming SDP: \" + JSON.stringify(sdp)); \n \
        webrtcPeerConnection.setRemoteDescription(sdp).catch(reportError); \n \
        webrtcPeerConnection.createAnswer().then(onLocalDescription).catch(reportError); \n \
      } \n \
 \n \
 \n \
      function onIncomingICE(ice) { \n \
        var candidate = new RTCIceCandidate(ice); \n \
        console.log(\"Incoming ICE: \" + JSON.stringify(ice)); \n \
        webrtcPeerConnection.addIceCandidate(candidate).catch(reportError); \n \
      } \n \
 \n \
 \n \
      function onAddRemoteStream(event) { \n \
        html5VideoElement.srcObject = event.streams[0]; \n \
      } \n \
 \n \
 \n \
      function onIceCandidate(event) { \n \
        if (event.candidate == null) \n \
          return; \n \
 \n \
        console.log(\"Sending ICE candidate out: \" + JSON.stringify(event.candidate)); \n \
        websocketConnection.send(JSON.stringify({ \"type\": \"ice\", \"data\": event.candidate })); \n \
      } \n \
 \n \
 \n \
      function onServerMessage(event) { \n \
        var msg; \n \
 \n \
        try { \n \
          msg = JSON.parse(event.data); \n \
        } catch (e) { \n \
          return; \n \
        } \n \
 console.log (\"On Server Message\")\n \
 \n \
        if (!webrtcPeerConnection) { \n \
          webrtcPeerConnection = new RTCPeerConnection(webrtcConfiguration); \n \
          webrtcPeerConnection.ontrack = onAddRemoteStream; \n \
          webrtcPeerConnection.onicecandidate = onIceCandidate; \n \
        } \n \
 \n \
        switch (msg.type) { \n \
          case \"sdp\": onIncomingSDP(msg.data); break; \n \
          case \"ice\": onIncomingICE(msg.data); break; \n \
          default: break; \n \
        } \n \
      } \n \
 \n \
 \n \
      function playStream(videoElement, hostname, port, path, configuration, reportErrorCB) { \n \
        var l = window.location;\n \
        var wsHost = (hostname != undefined) ? hostname : l.hostname; \n \
        var wsPort = (port != undefined) ? port : l.port; \n \
        var wsPath = (path != undefined) ? path : \"ws\"; \n \
        if (wsPort) \n\
          wsPort = \":\" + wsPort; \n\
        var wsUrl = \"ws://\" + wsHost + wsPort + \"/\" + wsPath; \n \
 \n \
  console.log (\"WS URL \"+wsUrl) \n \
        html5VideoElement = videoElement; \n \
        webrtcConfiguration = configuration; \n \
        reportError = (reportErrorCB != undefined) ? reportErrorCB : function(text) {}; \n \
 \n \
        websocketConnection = new WebSocket(wsUrl); \n \
        websocketConnection.addEventListener(\"message\", onServerMessage); \n \
      } \n \
 \n \
      window.onload = function() { \n \
        var vidstream = document.getElementById(\"stream\"); \n \
        var config = { 'iceServers': [{ 'urls': 'stun:" STUN_SERVER "' }] }; \n\
        playStream(vidstream, null, null, null, config, function (errmsg) { console.error(errmsg); }); \n \
      }; \n \
 \n \
    </script> \n \
  </head> \n \
 \n \
  <body> \n \
    <div> \n \
      <video id=\"stream\" autoplay playsinline>Your browser does not support video</video> \n \
    </div> \n \
  </body> \n \
</html> \n \
";
void
soup_http_handler (G_GNUC_UNUSED SoupServer * soup_server,
    SoupMessage * message, const char *path, G_GNUC_UNUSED GHashTable * query,
    G_GNUC_UNUSED SoupClientContext * client_context,
    G_GNUC_UNUSED gpointer user_data)
{
  SoupBuffer *soup_buffer;

  if ((g_strcmp0 (path, "/") != 0) && (g_strcmp0 (path, "/index.html") != 0)) {
    soup_message_set_status (message, SOUP_STATUS_NOT_FOUND);
    return;
  }

  soup_buffer =
      soup_buffer_new (SOUP_MEMORY_STATIC, html_source, strlen (html_source));

  soup_message_headers_set_content_type (message->response_headers, "text/html",
      NULL);
  soup_message_body_append_buffer (message->response_body, soup_buffer);
  soup_buffer_free (soup_buffer);

  soup_message_set_status (message, SOUP_STATUS_OK);
}

void
soup_websocket_closed_cb (SoupWebsocketConnection * connection,
    gpointer user_data)
{
  delete_ctx (user_data); 
  g_print ("Closed websocket connection %p\n", (gpointer) connection);
}

void
soup_websocket_message_cb (G_GNUC_UNUSED SoupWebsocketConnection * connection,
    SoupWebsocketDataType data_type, GBytes * message, gpointer user_data)
{
  //WEBRTC_CTX *ctx = (WEBRTC_CTX *)user_data;
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
    g_error ("Received message without type field %s", data_string);
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

    if (g_strcmp0 (sdp_type_string, "answer") != 0) {
      g_error ("Expected SDP message type \"answer\", got \"%s\"\n",
          sdp_type_string);
      goto cleanup;
    }

    if (!json_object_has_member (data_json_object, "sdp")) {
      g_error ("Received SDP message without SDP string\n");
      goto cleanup;
    }
    sdp_string = json_object_get_string_member (data_json_object, "sdp");

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



void
soup_websocket_handler (G_GNUC_UNUSED SoupServer * server,
    SoupWebsocketConnection * connection, G_GNUC_UNUSED const char *path,
    G_GNUC_UNUSED SoupClientContext * client_context, gpointer user_data)
{
  SOUP_DATA *sdata = (SOUP_DATA *)user_data;

  g_print ("Processing new websocket connection %p", (gpointer) connection); 
  g_print ("Create gstreamer connection here\n");
  void *ctx = start_webrtc_stream (sdata->filename, sdata->use_filter, sendMessage, g_object_ref(connection));
  g_signal_connect (G_OBJECT (connection), "message",
      G_CALLBACK (soup_websocket_message_cb), (gpointer)ctx);
  g_signal_connect (G_OBJECT (connection), "closed",
      G_CALLBACK (soup_websocket_closed_cb), (gpointer) ctx);
  printf ("Websocket handling done\n");
}

int main (int argc, char *argv[])
{
  GMainLoop *mainloop;
  SoupServer *soup_server;
  void *ctx = NULL;
  SOUP_DATA sdata;

  if (argc != 3 )
  {
     g_print ("Usage ./test filename <use filter 1/0>\n");
     return -1;
  }
  
  sdata.filename = argv[1];
  sdata.use_filter = atoi(argv[2]);
  initialize_ctx ();
  mainloop = g_main_loop_new (NULL, FALSE);
  g_unix_signal_add (SIGINT, exit_sighandler, mainloop);
  g_unix_signal_add (SIGTERM, exit_sighandler, mainloop);

  soup_server =
      soup_server_new (SOUP_SERVER_SERVER_HEADER, "webrtc-soup-server", NULL);
  soup_server_add_handler (soup_server, "/", soup_http_handler, NULL, NULL);
  soup_server_add_websocket_handler (soup_server, "/ws", NULL, NULL,
      soup_websocket_handler, &sdata, NULL);
  soup_server_listen_all (soup_server, SOUP_HTTP_PORT,
      (SoupServerListenOptions) 0, ctx);

  g_print ("WebRTC page link: http://127.0.0.1:%d/\n", (gint) SOUP_HTTP_PORT);

 // void *ctx = start_webrtc_stream ("test", 0, sendMessage, NULL);
  g_main_loop_run (mainloop);
  g_print ("Unref Main Loop\n");
  g_main_loop_unref (mainloop);
  g_print ("Unref Soup Server\n");
  g_object_unref (G_OBJECT (soup_server));
  g_print ("Unref gstreamer deinit\n");
 // gst_deinit();
  g_print ("Return from code %p\n", ctx);
 // delete_ctx (ctx);
  return 0;
}
