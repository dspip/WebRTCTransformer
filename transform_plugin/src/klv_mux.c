#include <string.h>
#include <gst/gst.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

static GMainLoop *loop;
static GstElement *pipeline, *appsrc, *src, *tee, *encoder, *muxer, *filesink, *videoconvert, *videosink, *queue_record, *queue_display, *caps_filter, *capsfilter_klv;
static GstBus *bus;

/* KLV data from Day_Flight.mpg */
static const guint8 rtp_KLV_frame_data[] = {
  0x06, 0x0e, 0x2b, 0x34, 0x02, 0x0b, 0x01, 0x01,
  0x0e, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0x00,
  0x81, 0x91, 0x02, 0x08, 0x00, 0x04, 0x6c, 0x8e,
  0x20, 0x03, 0x83, 0x85, 0x41, 0x01, 0x01, 0x05,
  0x02, 0x3d, 0x3b, 0x06, 0x02, 0x15, 0x80, 0x07,
  0x02, 0x01, 0x52, 0x0b, 0x03, 0x45, 0x4f, 0x4e,
  0x0c, 0x0e, 0x47, 0x65, 0x6f, 0x64, 0x65, 0x74,
  0x69, 0x63, 0x20, 0x57, 0x47, 0x53, 0x38, 0x34,
  0x0d, 0x04, 0x4d, 0xc4, 0xdc, 0xbb, 0x0e, 0x04,
  0xb1, 0xa8, 0x6c, 0xfe, 0x0f, 0x02, 0x1f, 0x4a,
  0x10, 0x02, 0x00, 0x85, 0x11, 0x02, 0x00, 0x4b,
  0x12, 0x04, 0x20, 0xc8, 0xd2, 0x7d, 0x13, 0x04,
  0xfc, 0xdd, 0x02, 0xd8, 0x14, 0x04, 0xfe, 0xb8,
  0xcb, 0x61, 0x15, 0x04, 0x00, 0x8f, 0x3e, 0x61,
  0x16, 0x04, 0x00, 0x00, 0x01, 0xc9, 0x17, 0x04,
  0x4d, 0xdd, 0x8c, 0x2a, 0x18, 0x04, 0xb1, 0xbe,
  0x9e, 0xf4, 0x19, 0x02, 0x0b, 0x85, 0x28, 0x04,
  0x4d, 0xdd, 0x8c, 0x2a, 0x29, 0x04, 0xb1, 0xbe,
  0x9e, 0xf4, 0x2a, 0x02, 0x0b, 0x85, 0x38, 0x01,
  0x2e, 0x39, 0x04, 0x00, 0x8d, 0xd4, 0x29, 0x01,
  0x02, 0x1c, 0x5f
};

static gboolean
message_cb (GstBus * bus, GstMessage * message, gpointer user_data)
{
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:{
      g_main_loop_quit (loop);

      GError *err = NULL;
      gchar *name, *debug = NULL;
      name = gst_object_get_path_string (message->src);
      gst_message_parse_error (message, &err, &debug);

      gchar * err_message = err->message;
      if(err_message == "Output window was closed"){
          g_print ("Error message %s\n", err_message);
      }
      else{
          g_printerr ("\nERROR: from element %s: %s\n", name, err->message);
          if (debug != NULL)
              g_printerr ("Additional debug info:\n%s\n", debug);
      }

      g_error_free (err);
      g_free (debug);
      g_free (name);
      break;
    }
    case GST_MESSAGE_WARNING:{
        GError *err = NULL;
        gchar *name, *debug = NULL;

        name = gst_object_get_path_string (message->src);
        gst_message_parse_warning (message, &err, &debug);

        g_printerr ("ERROR: from element %s: %s\n", name, err->message);
        if (debug != NULL)
        g_printerr ("Additional debug info:\n%s\n", debug);

        g_error_free (err);
        g_free (debug);
        g_free (name);
        break;
    }
    case GST_MESSAGE_EOS: {
        g_print ("Got EOS\n");
        g_main_loop_quit(loop);
        break;
    }
    default:
        break;
  }

  return TRUE;
}
void need_data(GstElement* element, guint data, gpointer userData)
{
    static int cnt = 0;
    //std::cout << "need_data" << std::endl;
    int size = sizeof(rtp_KLV_frame_data);
    GstFlowReturn ret;
    GstBuffer *buffer = gst_buffer_new();
    GstMemory *mem = gst_allocator_alloc (NULL, size, NULL);
    gst_buffer_append_memory(buffer, mem);
    gst_buffer_set_size(buffer, size);
    GstMapInfo info;
    gst_buffer_map   (buffer, &info, GST_MAP_READWRITE);
    memcpy (info.data, rtp_KLV_frame_data, size);
    gst_buffer_unmap (buffer, &info);
    //GST_BUFFER_TIMESTAMP(buffer) = (GstClockTime)((cnt++ / 30.0) * 1e9);
    guint64 val = 1000000000;
    GST_BUFFER_CAST(buffer)->pts = (cnt++ / 30.0)*val;
    g_signal_emit_by_name (element, "push-buffer", buffer, &ret);
    //getchar();
    // TODO push binary data (rtp_KLV_frame_data) in Big-Endian.
}
void sigintHandler(int unused)
{
    g_print("You ctrl-c-ed!");
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_element_send_event(pipeline, gst_event_new_eos());
}
int main(int argc, char *argv[])
{
    signal(SIGINT, sigintHandler);
    gst_init (&argc, &argv);
    //videotestsrc ! videoconvert ! video/x-raw, format=I420 ! x264enc ! h264parse ! queue ! mux.sink_0 meta/x-klv, parsed=true! mpegtsmux ! 
    char p_string[1024];
    sprintf (p_string,"appsrc name=src do-timestamp=true caps=\"meta/x-klv, parsed=true\" !  queue ! mux.sink_0 filesrc  location=%s ! h264parse !  queue ! mux.sink_1  mpegtsmux name=mux ! udpsink host=%s port=%s", argv[1], argv[2], argv[3]);
    g_print ("pipeline %s\n", p_string);
    pipeline = gst_parse_launch(p_string, NULL);
    GstElement *appsrc = gst_bin_get_by_name (GST_BIN (pipeline), "src");
    g_object_set(appsrc, "format", GST_FORMAT_TIME, NULL);
    g_signal_connect (appsrc, "need-data", G_CALLBACK (need_data), (void *)rtp_KLV_frame_data);
    gst_element_set_state (pipeline, GST_STATE_PLAYING);
    
    loop = g_main_loop_new(NULL, FALSE);
    g_print("Starting loop, press enter to quit the application");
    getchar ();
   // g_main_loop_run(loop);
    gst_object_unref (bus);
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);

        
    return 0;
}

