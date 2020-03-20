/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2020 karan <<user@hostname.org>>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-mpeg4filter
 *
 * FIXME:Describe mpeg4filter here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! mpeg4filter ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <stdio.h>
#include "gstmpeg4filter.h"
#include <inttypes.h>


GST_DEBUG_CATEGORY_STATIC (gst_mpeg4filter_debug);
#define GST_CAT_DEFAULT gst_mpeg4filter_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_ENABLE
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

#define gst_mpeg4filter_parent_class parent_class
G_DEFINE_TYPE (Gstmpeg4filter, gst_mpeg4filter, GST_TYPE_ELEMENT);

static void gst_mpeg4filter_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_mpeg4filter_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_mpeg4filter_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_mpeg4filter_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */

/* initialize the mpeg4filter's class */
static void
gst_mpeg4filter_class_init (Gstmpeg4filterClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_mpeg4filter_set_property;
  gobject_class->get_property = gst_mpeg4filter_get_property;

  g_object_class_install_property (gobject_class, PROP_ENABLE,
      g_param_spec_boolean ("enable", "Enable", "enable plugin ?",
          FALSE, G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class,
    "mpeg4filter",
    "FIXME:Generic",
    "FIXME:Generic Template Element",
    "karan <<user@hostname.org>>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_mpeg4filter_init (Gstmpeg4filter * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_mpeg4filter_sink_event));
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_mpeg4filter_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->enable = FALSE;
  filter->sent_sei = FALSE;
}

static void
gst_mpeg4filter_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  Gstmpeg4filter *filter = GST_MPEG4FILTER (object);

  switch (prop_id) {
    case PROP_ENABLE:
      filter->enable = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_mpeg4filter_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  Gstmpeg4filter *filter = GST_MPEG4FILTER (object);

  switch (prop_id) {
    case PROP_ENABLE:
      g_value_set_boolean (value, filter->enable);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean
gst_mpeg4filter_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstmpeg4filter *filter;
  gboolean ret;

  filter = GST_MPEG4FILTER (parent);

  GST_LOG_OBJECT (filter, "Received %s event: %" GST_PTR_FORMAT,
      GST_EVENT_TYPE_NAME (event), event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps * caps;

      gst_event_parse_caps (event, &caps);
      /* do something with the caps */

      /* and forward */
      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}

/* chain function
 * this function does the actual processing
 */

//static unsigned char header[]={0x00 ,0x00 ,0x01 ,0xB0 ,0x01 ,0x00 ,0x00 ,0x01 ,0xB5 ,0x89 ,0x13 ,0x00 ,0x00 ,0x01 ,0x00 ,0x00 ,0x00 ,0x01 ,0x20 ,0x00 ,0xC4 ,0x8D ,0x88 ,0x01 ,0x95 ,0x28 ,0x04 ,0x5A ,0x14 ,0x43 ,0x00 ,0x00 ,0x01 ,0xB2 ,0x4C ,0x61 ,0x76 ,0x63 ,0x35 ,0x38 ,0x2E ,0x39 ,0x30 ,0x2E ,0x31 ,0x30 ,0x30};

static unsigned char header[]={0x00,0x00,0x01,0xb0,0x01,0x00,0x00,0x01,0xb5,0x89,0x13,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x20,0x00,0xc4,0xad,0x88,0x01,0x95,0x16,0x04,0x3c,0x14,0x43,0x00,0x00,0x01,0xb2,0x4c,0x61,0x76,0x63,0x35,0x38,0x2e,0x39,0x30,0x2e,0x31,0x30,0x30};

static GstFlowReturn insert_mpeg_header(Gstmpeg4filter *filter)

{
#if 0
	unsigned char header[1400];
	FILE *ptr;
	ptr = fopen("rtp1.bin","rb");  // r for read, b for binary
	fread(header,sizeof(header),1,ptr);
	fclose (ptr);
	ptr = NULL;
#endif	
	GstBuffer *buf1;
	GstMemory *memory;
	GstMapInfo info1;
	int sei_size = sizeof(header);
	buf1 = gst_buffer_new ();
	memory = gst_allocator_alloc (NULL, sizeof(header), NULL);
	gst_buffer_insert_memory (buf1, -1, memory);
	gst_buffer_map (buf1, &info1, GST_MAP_WRITE);
	memcpy (info1.data,  header, sei_size);
	gst_buffer_unmap (buf1, &info1);
	return gst_pad_push (filter->srcpad, buf1);
}
static GstFlowReturn push_mpeg4(Gstmpeg4filter *filter, unsigned char *data, int size)

{
#if 0
	unsigned char header[1400];
	FILE *ptr;
	ptr = fopen("rtp1.bin","rb");  // r for read, b for binary
	fread(header,sizeof(header),1,ptr);
	fclose (ptr);
	ptr = NULL;
#endif	
	GstBuffer *buf1;
	GstMemory *memory;
	GstMapInfo info1;
	buf1 = gst_buffer_new ();
	memory = gst_allocator_alloc (NULL, size, NULL);
	gst_buffer_insert_memory (buf1, -1, memory);
	gst_buffer_map (buf1, &info1, GST_MAP_WRITE);
	memcpy (info1.data,  data, size);
	gst_buffer_unmap (buf1, &info1);
	return gst_pad_push (filter->srcpad, buf1);
}

#define CHECK_BIT(var, pos) !!((var) & (1 << (pos)))


struct rtp_header {
    unsigned int version:2;     /* protocol version */
    unsigned int padding:1;     /* padding flag */
    unsigned int extension:1;   /* header extension flag */
    unsigned int cc:4;          /* CSRC count */
    unsigned int marker:1;      /* marker bit */
    unsigned int pt:7;          /* payload type */
    uint16_t seq:16;            /* sequence number */
    uint32_t ts;                /* timestamp */
    uint32_t ssrc;              /* synchronization source */
    uint32_t csrc[1];           /* optional CSRC list */
};

#define MAX_BUF_SIZE 1500

unsigned int rtp_parse(unsigned char *raw, unsigned int size)
{
    unsigned int raw_offset = 0;
    unsigned int rtp_length = size;
    unsigned int paysize;
    unsigned char payload[MAX_BUF_SIZE];
    struct rtp_header rtp_h;

    rtp_h.version = raw[raw_offset] >> 6;
    rtp_h.padding = CHECK_BIT(raw[raw_offset], 5);
    rtp_h.extension = CHECK_BIT(raw[raw_offset], 4);
    rtp_h.cc = raw[raw_offset] & 0xFF;

    /* next byte */
    raw_offset++;

    rtp_h.marker = CHECK_BIT(raw[raw_offset], 8);
    rtp_h.pt     = raw[raw_offset] & 0x7f;

    /* next byte */
    raw_offset++;

    /* Sequence number */
    rtp_h.seq = raw[raw_offset] * 256 + raw[raw_offset + 1];
    raw_offset += 2;

    /* time stamp */
    rtp_h.ts = \
        (raw[raw_offset    ] << 24) |
        (raw[raw_offset + 1] << 16) |
        (raw[raw_offset + 2] <<  8) |
        (raw[raw_offset + 3]);
    raw_offset += 4;

    /* ssrc / source identifier */
    rtp_h.ssrc = \
        (raw[raw_offset    ] << 24) |
        (raw[raw_offset + 1] << 16) |
        (raw[raw_offset + 2] <<  8) |
        (raw[raw_offset + 3]);
    raw_offset += 4;
 //   rtp_st.rtp_identifier = rtp_h.ssrc;

    /* Payload size */
    paysize = (rtp_length - raw_offset);

    memset(payload, '\0', sizeof(payload));
    memcpy(&payload, raw + raw_offset, paysize);
    for (int i=0; i<40; i++)
	    g_print ("%02x  ", payload[i]);
    g_print ("\n");

    /* Display RTP header info */
    printf("   >> RTP\n");
    printf("      Version     : %i\n", rtp_h.version);
    printf("      Padding     : %i\n", rtp_h.padding);
    printf("      Extension   : %i\n", rtp_h.extension);
    printf("      CSRC Count  : %i\n", rtp_h.cc);
    printf("      Marker      : %i\n", rtp_h.marker);
    printf("      Payload Type: %i\n", rtp_h.pt);
    printf("      Sequence    : %i\n", rtp_h.seq);
    printf("      Timestamp   : %u\n", rtp_h.ts);
    printf("      Sync Source : %u\n", rtp_h.ssrc);

    return rtp_h.ts;

}
static GstFlowReturn
gst_mpeg4filter_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  Gstmpeg4filter *filter;
  GstMapInfo info;
  int i;
  static int first_frame = 0;
  static int found  = 0;
  filter = GST_MPEG4FILTER (parent);
  gst_buffer_map (buf, &info, GST_MAP_READWRITE);
  unsigned char *data = info.data;
  unsigned char *data1 = info.data;
  int offset = 0;
  //int32_t pts  = rtp_parse (data, info.size);
  if (data[1] != 0x61)
	  return GST_FLOW_OK;
  data += 12;
  /*
  for (i=0;  i<40;  i++)
	  g_print ("%02x ", data[i]);
  g_print ("\n");
  */
  /*
  if (  (data[0]==0x00) && 
	(data[1]==0x01) && 
	(data[2]==0x00) && 
	(data[3]==0x05) )
  {
	  offset += 24;
  }
  else if (  (data[0]==0x00) && 
	(data[1]==0x01) && 
	(data[2]==0x00) && 
	(data[3]==0x02) )
  {
	  offset += 12;
  }
  else if (  (data[0]==0x00) && 
	(data[1]==0x02) && 
	(data[2]==0x00) && 
	(data[3]==0x07) )
  {
	  offset += 32;
  }
  */
  if (  (data[0]==0x00) && 
	(data[1]==0x01) )
  {
	  offset += 4 + (((data[2]*256) + data[3])*4);
  }
  else if (  (data[0]==0x00) && 
	(data[1]==0x02) )
  {
	  offset += 4 + (((data[2]*256) + data[3])*4);
  }

  int bsize = (info.size)-offset-12;
  if (bsize < 0)
	  offset = 0;
//  printf ("bsize %d %d\n", bsize, offset);
  push_mpeg4 (filter, data+offset, (info.size)-offset-12);
  /*
   if (rtp_bytes[12:16].hex() == '00010005'):
        print (rtp_bytes[12:17])
        offset += 24
    elif (rtp_bytes[12:16].hex() == '00010002'):
        offset += 12
    elif (rtp_bytes[12:16].hex() == '00020007'):
        offset += 32
  */
  gst_buffer_unmap (buf, &info);
  gst_buffer_unref (buf);
  /*
  if (!first_frame)
  {
	  insert_mpeg_header (filter);
	  first_frame = 1;
  } 
  */
  return GST_FLOW_OK;
	 // gst_pad_push (filter->srcpad, buf);
#if 0
  gst_buffer_map (buf, &info, GST_MAP_READWRITE);
  unsigned char *data = info.data;

  for (i=0; i<20; i++)   
  {
	  if (  (data[i]   == 0) &&
		(data[i+1] == 0) &&
		(data[i+2] == 1) &&
		(data[i+3] == 0x20) 
	     )
	  {
		  found = 1;
	  }
      g_print  ("%02x  ", data[i]);
  }
  g_print ("\n");
  gst_buffer_unmap (buf, &info);
  if (found == 1)
	  return gst_pad_push (filter->srcpad, buf);
  else
	  return GST_FLOW_OK;
#endif
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
mpeg4filter_init (GstPlugin * mpeg4filter)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template mpeg4filter' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_mpeg4filter_debug, "mpeg4filter",
      0, "Template mpeg4filter");

  return gst_element_register (mpeg4filter, "mpeg4filter", GST_RANK_NONE,
      GST_TYPE_MPEG4FILTER);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstmpeg4filter"
#endif

/* gstreamer looks for this structure to register mpeg4filters
 *
 * exchange the string 'Template mpeg4filter' with your mpeg4filter description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    mpeg4filter,
    "Template mpeg4filter",
    mpeg4filter_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
