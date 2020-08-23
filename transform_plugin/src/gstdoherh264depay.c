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
 * SECTION:element-doherh264depay
 *
 * FIXME:Describe doherh264depay here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! doherh264depay ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>

#include <gst/rtp/gstrtpbuffer.h>
#include "gstdoherh264depay.h"
#include <inttypes.h>
GST_DEBUG_CATEGORY_STATIC (gst_doherh264depay_debug);
#define GST_CAT_DEFAULT gst_doherh264depay_debug

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

#define gst_doherh264depay_parent_class parent_class
G_DEFINE_TYPE (Gstdoherh264depay, gst_doherh264depay, GST_TYPE_ELEMENT);

static void gst_doherh264depay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_doherh264depay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_doherh264depay_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_doherh264depay_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */

/* initialize the doherh264depay's class */
static void
gst_doherh264depay_class_init (Gstdoherh264depayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_doherh264depay_set_property;
  gobject_class->get_property = gst_doherh264depay_get_property;

  g_object_class_install_property (gobject_class, PROP_ENABLE,
      g_param_spec_boolean ("enable", "Enable", "enable plugin ?",
          FALSE, G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class,
    "doherh264depay",
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
gst_doherh264depay_init (Gstdoherh264depay * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_doherh264depay_sink_event));
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_doherh264depay_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->enable = FALSE;
  filter->sent_sei = FALSE;
  filter->first_frame = FALSE;
}

static void
gst_doherh264depay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  Gstdoherh264depay *filter = GST_DOHERH264DEPAY (object);

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
gst_doherh264depay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  Gstdoherh264depay *filter = GST_DOHERH264DEPAY (object);

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
gst_doherh264depay_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstdoherh264depay *filter;
  gboolean ret;

  filter = GST_DOHERH264DEPAY (parent);

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
    case GST_EVENT_EOS:
         filter->first_frame = FALSE;
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}

/* chain function
 * this function does the actual processing
 */
unsigned char  g_blackIdr704x576[]={
0,0,0x01,0x65,0x88,0x84,0,0x37,0x39,0xFF,
0xFE,0xF6,0xF0,0xFE,0x05,0x36,0x56,0x04,0x50,0x97,0x11,0xCD,0xE8,0x9D,0x33,0x1F,0xBF,0x15,0xB8,0xBA,0x34,0x37,
0xBD,0xCE,0,0,0x03,0,0,0x03,0,0,0x03,0,0x19,0xE1,0x1E,0x6F,0x6C,0xDD,0x91,0x19,0x6A,0x70,0,0,0x03,0x01,0x3F,0,0x49,
0x81,0x04,0x04,0x88,0x1B,0xA0,0xBD,0x06,0xC8,0x3D,0x02,0x48,0x20,0x81,0xBE,0x17,0x61,0xA2,0,0,0x03,0,0,0x03,0,0,0x03,0,0,
0x03,0,0,0x03,0,0,0x03,0,0,0x03,0,0,0x03,0,0,0x03,0,0,0x03,0,0,0x03,0,0,0x03,0,0,0x03,0,0,0x03,0,0,0x21,0xE1};

unsigned char g_recover_sei[]={0, 0, 0 ,1, 6, 6, 1, 0xE4, 0x80};

static GstFlowReturn insert_sei (Gstdoherh264depay *filter)
{

  GstBuffer *buf1;
  GstMemory *memory;
  GstMapInfo info1;
  int sei_size = sizeof(g_recover_sei);
  buf1 = gst_buffer_new ();
  memory = gst_allocator_alloc (NULL, sizeof(g_recover_sei), NULL);
  gst_buffer_insert_memory (buf1, -1, memory);
  gst_buffer_map (buf1, &info1, GST_MAP_WRITE);
  memcpy (info1.data,  g_recover_sei, sei_size);
  gst_buffer_unmap (buf1, &info1);
  return gst_pad_push (filter->srcpad, buf1); 
}

static unsigned char  sps_pps[] = {0, 0, 0 ,1, 6, 6, 1, 0xE4, 0x80};
//{0x00, 0x00, 0x01, 0x27, 0x42, 0x80, 0x28, 0x8b, 0x95, 0x01, 0x80, 0x24, 0xfd, 0xa0, 0x00, 0x00, 0x00, 0x01, 0x28, 0xde, 0x01, 0xae, 0x20};

static GstFlowReturn  push_rtp (Gstdoherh264depay *filter, unsigned char *data, int size, uint64_t ts)
{   
  GstBuffer *buf1;
  GstMemory *memory;
  GstMapInfo info1;
  filter->first_frame = TRUE;

  unsigned char  sseq[] = {0,0};
  buf1 = gst_buffer_new ();
  if ((data[1]&0xf0)==0x80)
  {
	  memory = gst_allocator_alloc (NULL, size+2, NULL);
	  gst_buffer_insert_memory (buf1, -1, memory);
	  gst_buffer_map (buf1, &info1, GST_MAP_WRITE);
	  data[0] = 1;
	  data[1] =  (data[1]&0x0f)+0x20;
	  memcpy (info1.data, sseq, sizeof(sseq));
	  memcpy ((unsigned char*)(info1.data)+2, data, size);

  }
  else
  {
	  memory = gst_allocator_alloc (NULL, size-2, NULL);
	  gst_buffer_insert_memory (buf1, -1, memory);
	  gst_buffer_map (buf1, &info1, GST_MAP_WRITE);
	  memcpy (info1.data,  data+2, size-2);
  }

  gst_buffer_unmap (buf1, &info1);
  buf1->pts = ts;
  buf1->dts = ts;

  return gst_pad_push (filter->srcpad, buf1); 
}

static GstFlowReturn
gst_doherh264depay_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
	Gstdoherh264depay *filter;
	GstMapInfo info;
	int i;
	filter = GST_DOHERH264DEPAY (parent);
	GstFlowReturn ret;
	int size = info.size;
	GstRTPBuffer rtpbuffer = GST_RTP_BUFFER_INIT;
	gst_rtp_buffer_map (buf, GST_MAP_READ, &rtpbuffer);
	GstBuffer  *pbuf = gst_rtp_buffer_get_payload_buffer (&rtpbuffer);
	gst_rtp_buffer_unmap (&rtpbuffer);
	gst_buffer_map (pbuf, &info, GST_MAP_READ);
	unsigned char *data = info.data;
#if 0
	for (i=0; i<20; i++)
		g_print ("%02x ", data[i]);
	g_print("\n");
#endif
	ret =push_rtp (filter, info.data, info.size, buf->pts);
	gst_buffer_unmap (pbuf, &info);
	       //	gst_pad_push (filter->srcpad, pbuf);
	return ret;

#if 0
	gboolean present_extention =  gst_rtp_buffer_get_extension (&rtpbuffer);

	if (present_extention == 1)
	{
		guint16  bits;
		GBytes * bytes = NULL;
		bytes = gst_rtp_buffer_get_extension_bytes (&rtpbuffer,
				&bits);
		GByteArray *byteArray = g_bytes_unref_to_array (bytes);
		guint8 *data = byteArray->data;
		ret = send_extention_data (filter,  byteArray->data, byteArray->len);
	}
#if 1
	g_print ("Old  ");
	for (i=0; i<40; i++)
		g_print ("%02x ", data[i]);
	g_print("\n");
	if ((data[16] != 0x3c) &&  (data[24] != 0x3c))
	{

		gst_buffer_unmap (buf, &info);
		return GST_FLOW_OK;
	} 

	if (((data[25]&0xf0)==0x80))
	{
		data[22] = 0x0;
		data[23] = 0x0;
		data[24] = 0x1;
		data[25] =  (data[25]&0x0f)+0x20;
		ret = push_rtp(filter, data+22, size-22, buf->pts);
		for (i=0; i<40; i++)
			g_print ("%02x ", data[i+22]);
		g_print("\n");
	}
	/*
	   else if (((data[20]&0xf0)==0x80))
	   {
	   data[20] = 0x0;
	   data[21] = 0x0;
	   data[22] = 0x1;
	   data[23] =  (data[23]&0x0f)+0x20;
	   ret = push_rtp(filter, data+20, size-20, buf->pts);
	   }
	   */
	else
	{
		ret=  push_rtp(filter, data+18, size-18,  buf->pts);
		for (i=0; i<40; i++)
			g_print ("%02x ", data[i+18]);
		g_print ("\n");
	}
#else
	for (i=0; i<20; i++)
		g_print ("%02x ", data[i+24]);
	g_print("\n");
#endif
	getchar();
	gst_buffer_unmap (buf, &info);
	return ret;
#endif
	//return gst_pad_push (filter->srcpad, buf);
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
doherh264depay_init (GstPlugin * doherh264depay)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template doherh264depay' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_doherh264depay_debug, "doherh264depay",
      0, "Template doherh264depay");

  return gst_element_register (doherh264depay, "doherh264depay", GST_RANK_NONE,
      GST_TYPE_DOHERH264DEPAY);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstdoherh264depay"
#endif

/* gstreamer looks for this structure to register doherh264depays
 *
 * exchange the string 'Template doherh264depay' with your doherh264depay description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    doherh264depay,
    "Template doherh264depay",
    doherh264depay_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
