#include <gst/gst.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
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

int
main (int argc, char *argv[])
{
  GstElement *Pipeline;
  GstBus *bus;
  GstMessage *msg;

  /* Initialize GStreamer */

  gst_init (&argc, &argv);
 
  if (argc < 6)
  {
      g_print ("Usage ./generate <filename> <transcoding 1/0> <h264-0 mpeg4-1><address> <port>");
      return -1;
  }
 
#define  PIPELINE_MP4_STR  "filesrc location=%s ! qtdemux ! "
#define  PIPELINE_RTP_H264 "rtph264pay config-interval=1 ! udpsink host=%s port=%s"
#define  PIPELINE_RTP_MPEG "rtpmp4vpay config-interval=1 ! udpsink host=%s port=%s"
#define  PIPELINE_H264_TRANSCODE  "filesrc location=%s ! decodebin ! videoconvert ! x264enc ! video/x-h264, profile=baseline ! "
#define  PIPELINE_MPEG4_TRANSCODE  "filesrc location=%s ! decodebin ! videoconvert ! avenc_mpeg4 ! "

  char *filename  = argv[1];
  int transcoding = atoi(argv[2]);
  int codec_type  = atoi(argv[3]);
  char *addr = argv[4];
  char *port = argv[5];
  char *pipeline = (char *)malloc (2048);

  if (transcoding  == 0)
  {

      if (codec_type == 0)
      {
          sprintf (pipeline, PIPELINE_MP4_STR PIPELINE_RTP_H264, filename, addr, port);
      }
      else  if (codec_type == 1)
      {
          sprintf (pipeline, PIPELINE_MP4_STR PIPELINE_RTP_MPEG, filename, addr, port);
      }
      else
      {
          g_print ("Wrong codec option %d\n" , codec_type );
          return -1;
      }

  }
  else if (transcoding  == 1)
  {

      if (codec_type == 0)
      {
          sprintf (pipeline, PIPELINE_H264_TRANSCODE PIPELINE_RTP_H264, filename, addr, port);
      }
      else  if (codec_type == 1)
      {
          sprintf (pipeline, PIPELINE_MPEG4_TRANSCODE PIPELINE_RTP_MPEG, filename, addr, port);
      }
      else
      {
          g_print ("Wrong codec option %d\n",  codec_type );
          return -1;
      }

  }
  else
  {
          g_print ("Wrong transcoding option %d\n", transcoding);
          return -1;
  }
  enable_factory("nvv4l2decoder", FALSE);
  /* Build the pipeline */

  g_print ("Running pipeline %s\n", pipeline);
  Pipeline =
      gst_parse_launch
      (pipeline,
      NULL);


  /* Start playing */
  gst_element_set_state (Pipeline, GST_STATE_PLAYING);

  /* Wait until error or EOS */
  bus = gst_element_get_bus (Pipeline);
  msg =
      gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
      GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

  /* Free resources */
  if (msg != NULL)
    gst_message_unref (msg);
  gst_object_unref (bus);
  gst_element_set_state (Pipeline, GST_STATE_NULL);
  gst_object_unref (Pipeline);
  enable_factory("nvv4l2decoder", TRUE);
  return 0;
}

