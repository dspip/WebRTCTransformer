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
#define STUN_SERVER  "stun.l.google.com:19302"

typedef struct
{
    GstElement *pipeline;
    GstElement *webrtcbin;
    user_cb     userCb;
    void       *user_data;
    gboolean   sender;

}WEBRTC_CTX;

static gchar *
get_string_from_json_object (JsonObject * object)
{
    JsonNode *root;
    JsonGenerator *generator;
    gchar *text;

    /* Make it the root node */
    root = json_node_init_object (json_node_alloc (), object);
    generator = json_generator_new ();
    json_generator_set_root (generator, root);
    text = json_generator_to_data (generator, NULL);

    /* Release everything */
    g_object_unref (generator);
    json_node_free (root);
    return text;
}
void on_offer_created_cb (GstPromise * promise, gpointer user_data)
{
    gchar *sdp_string;
    gchar *json_string;
    JsonObject *sdp_json;
    JsonObject *sdp_data_json;
    GstStructure const *reply;
    GstPromise *local_desc_promise;
    GstWebRTCSessionDescription *offer = NULL;
    WEBRTC_CTX *ctx =  (WEBRTC_CTX *)user_data;
    GstElement *webrtcbin = (GstElement *)(ctx->webrtcbin);

    reply = gst_promise_get_reply (promise);
    if (ctx->sender == TRUE)
     gst_structure_get (reply, "offer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION,
       &offer, NULL);
    else
     gst_structure_get (reply, "answer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION,
       &offer, NULL);
    gst_promise_unref (promise);

    local_desc_promise = gst_promise_new ();
    g_signal_emit_by_name (webrtcbin, "set-local-description",
      offer, local_desc_promise);
    gst_promise_interrupt (local_desc_promise);
    gst_promise_unref (local_desc_promise);

    sdp_string = gst_sdp_message_as_text (offer->sdp);

    sdp_json = json_object_new ();
    json_object_set_string_member (sdp_json, "type", "sdp");

    sdp_data_json = json_object_new ();
    if (ctx->sender == TRUE)
    {
     json_object_set_string_member (sdp_data_json, "type", "offer");
    }
    else
    {
     json_object_set_string_member (sdp_data_json, "type", "answer");
    }
    json_object_set_string_member (sdp_data_json, "sdp", sdp_string);
    json_object_set_object_member (sdp_json, "data", sdp_data_json);

    json_string = get_string_from_json_object (sdp_json);
    // g_print ("Negotiation offer created:\n%s\n", json_string);

    if (ctx->userCb)
     ctx->userCb (json_string, ctx->user_data);
    json_object_unref (sdp_json);
}





//Set Remote description to webrtc
int  webrtc_set_remote_sdp(void *ctx1, char *sdp_string)
{
    GstPromise *promise;
    GstSDPMessage *sdp;
    int ret;
    WEBRTC_CTX *ctx = (WEBRTC_CTX *)ctx1;

    ret = gst_sdp_message_new (&sdp);
    g_assert_cmphex (ret, ==, GST_SDP_OK);

    ret =
     gst_sdp_message_parse_buffer ((guint8 *) sdp_string,
       strlen (sdp_string), sdp);
    if (ret != GST_SDP_OK) {
     g_error ("Could not parse SDP string\n");
     return -1;
    }

    if (ctx->sender == TRUE)
    {
     GstWebRTCSessionDescription *answer;
     answer = gst_webrtc_session_description_new (GST_WEBRTC_SDP_TYPE_ANSWER,
       sdp);
     g_assert_nonnull (answer);

     promise = gst_promise_new ();
     g_signal_emit_by_name (ctx->webrtcbin, "set-remote-description",
       answer, promise);
     gst_promise_interrupt (promise);
     gst_promise_unref (promise);
     gst_webrtc_session_description_free (answer);
    }
    else
    {
     GstWebRTCSessionDescription *offer;
     offer = gst_webrtc_session_description_new (GST_WEBRTC_SDP_TYPE_OFFER, sdp);
     g_assert_nonnull (offer);

     /* Set remote description on our pipeline */
     {
      promise = gst_promise_new ();
      g_signal_emit_by_name (ctx->webrtcbin, "set-remote-description", offer,
        promise);
      gst_promise_interrupt (promise);
      gst_promise_unref (promise);
     }
     gst_webrtc_session_description_free (offer);

     promise = gst_promise_new_with_change_func (on_offer_created_cb, ctx,
       NULL);
     g_signal_emit_by_name (ctx->webrtcbin, "create-answer", NULL, promise);

    }
    return 0;
}

//Send Webrtc bin remote ice candiadate
int webrtc_set_remote_ice(void *ctx1, int mline_index, char *candidate_string)
{
   WEBRTC_CTX *ctx = (WEBRTC_CTX *)ctx1;
   g_signal_emit_by_name (ctx->webrtcbin, "add-ice-candidate",
        mline_index, candidate_string);
   return 0;
}


//Send Remote ICE Candidate Information
void on_ice_candidate_cb (G_GNUC_UNUSED GstElement * webrtcbin, guint mline_index,
     gchar * candidate, gpointer user_data)
{
    JsonObject *ice_json;
    JsonObject *ice_data_json;
    gchar *json_string;
    WEBRTC_CTX *ctx = (WEBRTC_CTX *)user_data;

    ice_json = json_object_new ();
    json_object_set_string_member (ice_json, "type", "ice");

    ice_data_json = json_object_new ();
    json_object_set_int_member (ice_data_json, "sdpMLineIndex", mline_index);
    json_object_set_string_member (ice_data_json, "candidate", candidate);
    json_object_set_object_member (ice_json, "data", ice_data_json);

    json_string = get_string_from_json_object (ice_json);
    json_object_unref (ice_json);

    if (ctx->userCb)
     ctx->userCb (json_string, ctx->user_data);  
    g_free (json_string);
}

//Whenever Sending a stream need to create negotiation offer
void on_negotiation_needed_cb (GstElement * webrtcbin, gpointer user_data)
{
    GstPromise *promise;
    WEBRTC_CTX *ctx = (WEBRTC_CTX *)user_data;
    //If receiver no need to create negotiation
    if (ctx->sender == FALSE)
    {
     return;
    }
    g_print ("Creating negotiation offer\n");
    promise = gst_promise_new_with_change_func (on_offer_created_cb,
      (gpointer) user_data, NULL);
    g_signal_emit_by_name (G_OBJECT (webrtcbin), "create-offer", NULL, promise);
}

void initialize_ctx()
{
    gst_init (NULL, NULL); 
}

static void enable_factory (const gchar *name, gboolean enable) {
    GstRegistry *registry = NULL;
    GstElementFactory *factory = NULL;

    registry = gst_registry_get();
    if (!registry) return;

    factory = gst_element_factory_find (name);
    if (!factory) return;

    if (enable) {
        gst_plugin_feature_set_rank (GST_PLUGIN_FEATURE (factory), GST_RANK_PRIMARY + 1);
    }
    else {
        gst_plugin_feature_set_rank (GST_PLUGIN_FEATURE (factory), GST_RANK_NONE);
    }

    gst_registry_add_feature (registry, GST_PLUGIN_FEATURE (factory));
    return;
}


void*  start_webrtc_stream(
    char *address,
    char *port,
    char *interface,
     int add_filter,
     user_cb userCb,
     void *user_data
     )
{
    GError *error = NULL;
    WEBRTC_CTX *ctx = malloc (sizeof(WEBRTC_CTX));
    memset (ctx, 0, sizeof(WEBRTC_CTX));
        ctx->userCb = userCb;
    ctx->user_data = user_data;
    ctx->sender = TRUE;

    char pipeline_str[1204];

    enable_factory("nvv4l2decoder", FALSE);
    //Add SEI header
    if (add_filter == 0)
    { 
     g_print ("Adding IDRSERT Filter\n") ;	    
     sprintf(pipeline_str, "webrtcbin name=webrtcbin "
       "udpsrc address=%s port=%s ! application/x-rtp,media=video,encoding-name=H264, payload=96 ! rtph264depay ! h264parse ! idrinsert enable=true ! rtph264pay config-interval=-1 !  "
       "application/x-rtp,media=video,encoding-name=H264,payload="
       RTP_PAYLOAD_TYPE " ! webrtcbin. ", address, port);
    }    
    //Disable filter
    else if (add_filter == 1)
    {
     sprintf(pipeline_str, "webrtcbin name=webrtcbin "
       "udpsrc address=%s port=%s ! application/x-rtp,media=video,encoding-name=H264, payload=96 ! rtph264depay ! h264parse ! idrinsert enable=false ! rtph264pay config-interval=-1 !  "
       "application/x-rtp,media=video,encoding-name=H264,payload="
       RTP_PAYLOAD_TYPE " ! webrtcbin. ", address, port);
    }
    //Remove filter
    else if (add_filter == 2)
    { 
     sprintf(pipeline_str, "webrtcbin name=webrtcbin "
       "udpsrc address=%s port=%s ! application/x-rtp,media=video,encoding-name=H264, payload=96 ! rtph264depay ! h264parse !  rtph264pay config-interval=-1 !  "
       "application/x-rtp,media=video,encoding-name=H264,payload="
       RTP_PAYLOAD_TYPE " ! webrtcbin. ", address, port);
    }
    //Pass through H264
    else if (add_filter == 3)
    {
     sprintf(pipeline_str, "webrtcbin name=webrtcbin  "
       "udpsrc address=%s port=%s ! "
       "application/x-rtp,media=video,encoding-name=H264,payload="
       RTP_PAYLOAD_TYPE " ! webrtcbin. ", address, port);
    }
    //Pass through VP9
    else if (add_filter == 4)
    {
     sprintf(pipeline_str, "webrtcbin name=webrtcbin  "
       "udpsrc address=%s port=%s ! "
       "application/x-rtp,media=video,encoding-name=VP9,payload="
       RTP_PAYLOAD_TYPE " ! webrtcbin. ", address, port);
    }
    else if ((add_filter == 5))
    {
     sprintf(pipeline_str, "webrtcbin name=webrtcbin  "
       "udpsrc address=%s port=%s ! application/x-rtp, clock-rate=90000, encoding-name=MP4V-ES ! rtpmp4vdepay ! avdec_mpeg4 ! timestamp ! videoconvert ! x264enc tune=zerolatency !  video/x-h264, profile=baseline ! rtph264pay config-interval=-1 !  "
       "application/x-rtp,media=video,encoding-name=H264,payload="
       RTP_PAYLOAD_TYPE " ! webrtcbin. ", address, port);
    }
    else if ((add_filter == 6))
    {
     sprintf(pipeline_str, "webrtcbin name=webrtcbin  "
       "udpsrc address=%s port=%s  multicast-iface=%s  ! mpeg4filter ! mpeg4videoparse  !  avdec_mpeg4 skip-frame=5 ! videoconvert ! videorate ! video/x-raw, framerate=50/1 ! x264enc tune=zerolatency !  video/x-h264, profile=baseline ! rtph264pay config-interval=-1 !  "
       "application/x-rtp,media=video,encoding-name=H264,payload="
       RTP_PAYLOAD_TYPE " ! webrtcbin. ", address, port, interface);
    }
    else if ((add_filter == 7))
    {
     sprintf(pipeline_str, "webrtcbin name=webrtcbin  "
       "filesrc location=%s ! decodebin ! videoconvert !  x264enc ! video/x-h264, profile=baseline  !   rtph264pay config-interval=-1 !  "
       "application/x-rtp,media=video,encoding-name=H264,payload="
       RTP_PAYLOAD_TYPE " ! webrtcbin. ", address);
    }
    else
    { 
     g_print ("Not valid filter option selected\n");
     g_print ("select\n 0-enablefilter\n 1-disablefilter\n 2-removefilter\n 3-passthrough\n");
     return NULL;
    }    

    g_print ("Created Pipeline %s\n", pipeline_str);
        ctx->pipeline = gst_parse_launch (pipeline_str , &error);

    if (error != NULL) {
     g_error ("Could not create WebRTC pipeline: %s\n", error->message);
     g_error_free (error);
     free (ctx);
     return NULL;
    }

    ctx->webrtcbin =
     gst_bin_get_by_name (GST_BIN (ctx->pipeline), "webrtcbin");
    g_assert (ctx->webrtcbin != NULL);

    g_signal_connect (ctx->webrtcbin, "on-negotiation-needed",
      G_CALLBACK (on_negotiation_needed_cb), (gpointer) ctx);

    g_signal_connect (ctx->webrtcbin, "on-ice-candidate",
      G_CALLBACK (on_ice_candidate_cb), (gpointer) ctx);

    gst_element_set_state (ctx->pipeline, GST_STATE_PLAYING);
    return ctx;
}

void *receive_webrtc_stream (user_cb userCb,
     void *user_data)
{
    GError *error = NULL;
    WEBRTC_CTX *ctx = malloc (sizeof(WEBRTC_CTX));
    memset (ctx, 0, sizeof(WEBRTC_CTX));
    ctx->userCb = userCb;
    ctx->user_data = user_data;
    ctx->sender = FALSE;

    char pipeline_str[1204];
#if 0
    sprintf(pipeline_str, "webrtcbin name=webrtcbin stun-server=stun://" STUN_SERVER " "
      " ! decodebin ! videoconvert ! videorate ! video/x-raw, framerate=25/1 "
      " ! timestamp ! autovideosink");
#else
    sprintf(pipeline_str, "webrtcbin name=webrtcbin stun-server=stun://" STUN_SERVER " "
      " ! decodebin ! videoconvert ! autovideosink ");
#endif
    g_print ("Created Pipeline %s\n", pipeline_str);
    ctx->pipeline = gst_parse_launch (pipeline_str , &error);
    g_print ("start webrtc stream\n");
    if (error != NULL) {
     g_error ("Could not create WebRTC pipeline: %s\n", error->message);
     g_error_free (error);
     free (ctx);
     return NULL;
    }
    ctx->webrtcbin =
     gst_bin_get_by_name (GST_BIN (ctx->pipeline), "webrtcbin");
    g_print ("wbrtcbin %p\n", ctx->webrtcbin);
    g_assert (ctx->webrtcbin != NULL);

    g_print ("connect signals\n");
    g_signal_connect (ctx->webrtcbin, "on-negotiation-needed",
      G_CALLBACK (on_negotiation_needed_cb), (gpointer) ctx);

    g_signal_connect (ctx->webrtcbin, "on-ice-candidate",
      G_CALLBACK (on_ice_candidate_cb), (gpointer) ctx);

    gst_element_set_state (ctx->pipeline, GST_STATE_PLAYING);
    return ctx;
}


void delete_ctx (void *ctx1)
{
    WEBRTC_CTX *ctx = (WEBRTC_CTX *)ctx1;
    gst_element_set_state (GST_ELEMENT (ctx->pipeline),
      GST_STATE_NULL);
    gst_object_unref (GST_OBJECT (ctx->webrtcbin));
    gst_object_unref (GST_OBJECT (ctx->pipeline));
 //   gst_deinit (); 
}

