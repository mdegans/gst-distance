/*
 * GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 * Copyright (C) 2020 Michael de Gans <michael.john.degans@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
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
 * SECTION:element-dsdistance
 *
 * DsDistance is an element for coloring osd boxes with DeepStream.
 * 
 * DsDistance operates on metadata only.
 *
 * <refsect2>
 * <title>Example usage</title>
 * |[
 * ... ! nvinfer ! nvtracker ! dsdistance ! nvosd ...
 * ]|
 * </refsect2>
 */

#include "gstdsdistance.h"

#include "config.h"

// gstreamer
#include <gst/base/base.h>
#include <gst/controller/controller.h>
#include <gst/gst.h>
#include <gst/video/video-format.h>

GST_DEBUG_CATEGORY_STATIC(gst_dsdistance_debug);
#define GST_CAT_DEFAULT gst_dsdistance_debug

static const char ELEMENT_NAME[] = "dsdistance";
static const char ELEMENT_LONG_NAME[] = "DeepStream social distancing element";
static const char ELEMENT_TYPE[] = "Filter";
static const char ELEMENT_DESCRIPTION[] = "Make close objects red.";
static const char ELEMENT_AUTHOR_AND_EMAIL[] = PACKAGE_AUTHOR " " PACKAGE_EMAIL;

/**
 * The maximum class id that can be set.
 */
static const int MAX_CLASS_ID = 4096;
static const int DEFAULT_CLASS_ID = 0;
static const bool DEFAULT_DO_DRAWING = true;

/* Filter signals and args */
enum {
  /* FILL ME */
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_SILENT,
  PROP_CLASS_ID,
  PROP_DO_DRAWING,
};

/* the capabilities of the inputs and outputs.
 *
 * static src and sink are both: video/x-raw(memory:NVMM) {NV12, RGBA}
 */
static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
    "sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS(
        GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:NVMM", "{ NV12, RGBA }")));

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
    "src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS(
        GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:NVMM", "{ NV12, RGBA }")));

#define gst_dsdistance_parent_class parent_class
G_DEFINE_TYPE(GstDsDistance, gst_dsdistance, GST_TYPE_BASE_TRANSFORM);

static void gst_dsdistance_set_property(GObject* object,
                                        guint prop_id,
                                        const GValue* value,
                                        GParamSpec* pspec);
static void gst_dsdistance_get_property(GObject* object,
                                        guint prop_id,
                                        GValue* value,
                                        GParamSpec* pspec);

static GstFlowReturn gst_dsdistance_transform_ip(GstBaseTransform* base,
                                                 GstBuffer* outbuf);
static gboolean gst_dsdistance_start(GstBaseTransform* base);
static gboolean gst_dsdistance_stop(GstBaseTransform* base);

/* GObject vmethod implementations */

/* initialize the dsdistance's class */
static void gst_dsdistance_class_init(GstDsDistanceClass* klass) {
  GObjectClass* gobject_class;
  GstElementClass* gstelement_class;

  gobject_class = (GObjectClass*)klass;
  gstelement_class = (GstElementClass*)klass;

  gobject_class->set_property = gst_dsdistance_set_property;
  gobject_class->get_property = gst_dsdistance_get_property;

  // silent property
  g_object_class_install_property(
      gobject_class, PROP_SILENT,
      g_param_spec_boolean(
          "silent", "Silent", "Produce verbose output ?", FALSE,
          GParamFlags(G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE)));

  // do-drawing property
  g_object_class_install_property(
      gobject_class, PROP_DO_DRAWING,
      g_param_spec_boolean(
          "do-drawing", "Do Drawing",
          "Modify osd metadata for nvdsosd (make stuff red).",
          (gboolean) DEFAULT_DO_DRAWING,
          GParamFlags(G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE)));

  // class-id property
  g_object_class_install_property(
      gobject_class, PROP_CLASS_ID,
      g_param_spec_int(
          "class-id", "ClassID", "Class id of a person.", 0, MAX_CLASS_ID, DEFAULT_CLASS_ID,
          GParamFlags(G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE)));

  gst_element_class_set_details_simple(gstelement_class, ELEMENT_LONG_NAME,
                                       ELEMENT_TYPE, ELEMENT_DESCRIPTION,
                                       ELEMENT_AUTHOR_AND_EMAIL);

  gst_element_class_add_pad_template(
      gstelement_class, gst_static_pad_template_get(&src_template));
  gst_element_class_add_pad_template(
      gstelement_class, gst_static_pad_template_get(&sink_template));

  /* register vmethods
   */
  GST_BASE_TRANSFORM_CLASS(klass)->transform_ip =
      GST_DEBUG_FUNCPTR(gst_dsdistance_transform_ip);
  GST_BASE_TRANSFORM_CLASS(klass)->start =
      GST_DEBUG_FUNCPTR(gst_dsdistance_start);
  GST_BASE_TRANSFORM_CLASS(klass)->stop =
      GST_DEBUG_FUNCPTR(gst_dsdistance_stop);

  /* debug category for fltering log messages
   */
  GST_DEBUG_CATEGORY_INIT(gst_dsdistance_debug, ELEMENT_NAME, 0,
                          ELEMENT_DESCRIPTION);
}

/* initialize the new element
 * initialize instance structure
 */
static void gst_dsdistance_init(GstDsDistance* self) {
  GST_DEBUG("dsdistance init");
  self->silent = false;

  /* create a DistanceFilter for this instance
   */
  self->filter = new DistanceFilter();

  self->filter->class_id = DEFAULT_CLASS_ID;
  self->filter->do_drawing = DEFAULT_DO_DRAWING;
}

/* start the element and create external resources
 *
 * https://gstreamer.freedesktop.org/documentation/base/gstbasetransform.html?gi-language=c#GstBaseTransformClass::start
 */

static gboolean gst_dsdistance_start(GstBaseTransform* base) {
  GST_DEBUG_OBJECT(base, "start");
  return true;
}

/* stop the element and free external resources
 *
 * https://gstreamer.freedesktop.org/documentation/base/gstbasetransform.html?gi-language=c#GstBaseTransformClass::stop
 */

static gboolean gst_dsdistance_stop(GstBaseTransform* base) {
  GST_DEBUG_OBJECT(base, "stop");
  GstDsDistance* self = GST_DSDISTANCE(base);

  /* destroy the DistanceFilter
   */
  delete self->filter;

  return true;
}

/* do in-place work on the buffer (override the 'transform' method for copy)
 */
static GstFlowReturn gst_dsdistance_transform_ip(GstBaseTransform* base,
                                                 GstBuffer* outbuf) {
  GstDsDistance* filter = GST_DSDISTANCE(base);

  if (filter->silent == FALSE)
    GST_LOG("%s got buffer.", GST_ELEMENT_NAME(filter));

  if (GST_CLOCK_TIME_IS_VALID(GST_BUFFER_TIMESTAMP(outbuf)))
    gst_object_sync_values(GST_OBJECT(filter), GST_BUFFER_TIMESTAMP(outbuf));

  return filter->filter->on_buffer(outbuf);
}

/* __setattr__
 */
static void gst_dsdistance_set_property(GObject* object,
                                        guint prop_id,
                                        const GValue* value,
                                        GParamSpec* pspec) {
  GstDsDistance* filter = GST_DSDISTANCE(object);

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean(value);
      break;
    case PROP_DO_DRAWING:
      filter->filter->do_drawing = (bool) g_value_get_boolean(value);
      break;
    case PROP_CLASS_ID:
      filter->filter->class_id = g_value_get_int(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

/* __getattr__
 */
static void gst_dsdistance_get_property(GObject* object,
                                        guint prop_id,
                                        GValue* value,
                                        GParamSpec* pspec) {
  GstDsDistance* filter = GST_DSDISTANCE(object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean(value, filter->silent);
      break;
    case PROP_DO_DRAWING:
      g_value_set_boolean(value, (gboolean) filter->filter->do_drawing);
      break;
    case PROP_CLASS_ID:
      g_value_set_int(value, filter->filter->class_id);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}
