/*
 * GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 * Copyright (C) 2020 Niels De Graef <niels.degraef@gmail.com>
 * Copyright (C) 2020 Michael de Gans <michael.john.degans@gmail.com>
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
 * SECTION:element-dspayloadbroker
 *
 * DsPayloadBroker is an element for coloring osd boxes with DeepStream.
 *
 * DsPayloadBroker operates on metadata only.
 *
 * <refsect2>
 * <title>Example usage</title>
 * |[
 * ... ! nvinfer ! nvtracker ! dspayloadbroker ! nvosd ...
 * ]|
 * </refsect2>
 */

#include "gstdspayloadbroker.h"

#include "PyPayloadBroker.hpp"
#include "FileMetaBroker.hpp"

#include "config.h"

// gstreamer
#include <gst/base/base.h>
#include <gst/controller/controller.h>
#include <gst/gst.h>
#include <gst/video/video-format.h>

GST_DEBUG_CATEGORY_STATIC(gst_dspayloadbroker_debug);
#define GST_CAT_DEFAULT gst_dspayloadbroker_debug

static const char ELEMENT_NAME[] = "dspayloadbroker";
static const char ELEMENT_LONG_NAME[] = "metadata payload broker";
static const char ELEMENT_TYPE[] = "Filter";
static const char ELEMENT_DESCRIPTION[] = "send string based metadata around.";
static const char ELEMENT_AUTHOR_AND_EMAIL[] = PACKAGE_AUTHOR " " PACKAGE_EMAIL;

/* Filter signals and args */
enum {
  /* FILL ME */
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_SILENT,
  PROP_RESULTS,
  PROP_MODE,
  PROP_BASEPATH,
};

#define GST_TYPE_PAYLOAD_BROKER_MODE (gst_payload_broker_mode_get_type())
static GType
gst_payload_broker_mode_get_type (void)
{
  static GType dspayloadbroker_mode_type = 0;
  static const GEnumValue dspayloadbroker_mode[] = {
    {PAYLOAD_BROKER_MODE_PROPERTY, "return protobuf from results property", "property"},
    {PAYLOAD_BROKER_MODE_PROTO, "write coded protobuf to file", "proto"},
    {PAYLOAD_BROKER_MODE_CSV, "write csv to file (smart_distancing format).", "csv"},
    {0, nullptr, nullptr},
  };

  if (!dspayloadbroker_mode_type) {
    dspayloadbroker_mode_type =
        g_enum_register_static ("GstDsPayloadBrokerModeType", dspayloadbroker_mode);
  }
  return dspayloadbroker_mode_type;
}

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

#define gst_dspayloadbroker_parent_class parent_class
G_DEFINE_TYPE(GstDsPayloadBroker, gst_dspayloadbroker, GST_TYPE_BASE_TRANSFORM);

static void gst_dspayloadbroker_set_property(GObject* object,
                                           guint prop_id,
                                           const GValue* value,
                                           GParamSpec* pspec);
static void gst_dspayloadbroker_get_property(GObject* object,
                                           guint prop_id,
                                           GValue* value,
                                           GParamSpec* pspec);

static GstFlowReturn gst_dspayloadbroker_transform_ip(GstBaseTransform* base,
                                                    GstBuffer* outbuf);
static gboolean gst_dspayloadbroker_start(GstBaseTransform* base);
static gboolean gst_dspayloadbroker_stop(GstBaseTransform* base);

/* GObject vmethod implementations */

/* initialize the dspayloadbroker's class */
static void gst_dspayloadbroker_class_init(GstDsPayloadBrokerClass* klass) {
  GObjectClass* gobject_class;
  GstElementClass* gstelement_class;

  gobject_class = (GObjectClass*)klass;
  gstelement_class = (GstElementClass*)klass;

  gobject_class->set_property = gst_dspayloadbroker_set_property;
  gobject_class->get_property = gst_dspayloadbroker_get_property;

  // silent property
  g_object_class_install_property(
    gobject_class, PROP_SILENT,
    g_param_spec_boolean(
      "silent", "Silent", "Produce verbose output ?", FALSE,
      GParamFlags(G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE)));

  // results property
  g_object_class_install_property(
    gobject_class, PROP_RESULTS,
    g_param_spec_string("results", "Results",
      "Latest serialized results as "
      "libdistanceproto::Batch protobuf string (in property mode).", nullptr,
      GParamFlags(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

  // broker mode property
  g_object_class_install_property(
    gobject_class, PROP_MODE,
    g_param_spec_enum("mode", "Mode",
      "The mode for the element to operate in.",
      GST_TYPE_PAYLOAD_BROKER_MODE, PAYLOAD_BROKER_MODE_PROPERTY,
      GParamFlags(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
        GST_PARAM_MUTABLE_READY)));

  // basename property
  g_object_class_install_property(
    gobject_class, PROP_BASEPATH,
    g_param_spec_string("basepath", "BasePath",
      "The full base path (minus extension) in proto or csv mode", nullptr,
      GParamFlags(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
        GST_PARAM_MUTABLE_READY)));

  gst_element_class_set_details_simple(
    gstelement_class, ELEMENT_LONG_NAME,
    ELEMENT_TYPE, ELEMENT_DESCRIPTION,
    ELEMENT_AUTHOR_AND_EMAIL);

  gst_element_class_add_pad_template(
    gstelement_class, gst_static_pad_template_get(&src_template));
  gst_element_class_add_pad_template(
    gstelement_class, gst_static_pad_template_get(&sink_template));

  /* register vmethods
   */
  GST_BASE_TRANSFORM_CLASS(klass)->transform_ip =
    GST_DEBUG_FUNCPTR(gst_dspayloadbroker_transform_ip);
  GST_BASE_TRANSFORM_CLASS(klass)->start =
    GST_DEBUG_FUNCPTR(gst_dspayloadbroker_start);
  GST_BASE_TRANSFORM_CLASS(klass)->stop =
    GST_DEBUG_FUNCPTR(gst_dspayloadbroker_stop);

  /* debug category for fltering log messages
   */
  GST_DEBUG_CATEGORY_INIT(gst_dspayloadbroker_debug, ELEMENT_NAME, 0,
                          ELEMENT_DESCRIPTION);
}

/* initialize the new element
 * initialize instance structure
 */
static void gst_dspayloadbroker_init(GstDsPayloadBroker* self) {
  GST_DEBUG("dspayloadbroker init");

  self->silent = FALSE;
  /* create a PyPayloadBroker for this instance
   *
   * this is just a temporary implementation of PyPayloadBroker to get
   * metadata out via a gobject property. It's hacky and ugly.
   *
   * Eventually the hope is to set a python callback, but this work for now.
   */
  self->mode = PAYLOAD_BROKER_MODE_PROPERTY;
  self->filter = nullptr;
  self->basepath = nullptr;
}

/* start the element and create external resources
 *
 * https://gstreamer.freedesktop.org/documentation/base/gstbasetransform.html?gi-language=c#GstBaseTransformClass::start
 */

static gboolean gst_dspayloadbroker_start(GstBaseTransform* base) {
  GstDsPayloadBroker* self = GST_DSPAYLOADBROKER(base);
  GST_DEBUG_OBJECT(self, "dspayloadbroker start");

  switch (self->mode)
  {
    case PAYLOAD_BROKER_MODE_PROPERTY:
      self->filter = new ds::PyPayloadBroker();
      break;
    case PAYLOAD_BROKER_MODE_PROTO:
      if (self->basepath == nullptr) {
        GST_ERROR_OBJECT(self, "basepath must be set for proto mode");
        return false;
      }
      GST_DEBUG("creating FileMetaBroker with path %s and mode: proto", self->basepath);
      self->filter = new ds::FileMetaBroker(self->basepath, ds::FileMetaBroker::proto);
      ((ds::FileMetaBroker*)self->filter)->start();
      break;
    case PAYLOAD_BROKER_MODE_CSV:
      if (self->basepath == nullptr) {
        GST_ERROR_OBJECT(self, "basepath must be set for csv mode");
        return false;
      }
      GST_DEBUG("creating FileMetaBroker with path %s and mode: csv", self->basepath);
      self->filter = new ds::FileMetaBroker(self->basepath, ds::FileMetaBroker::csv);
      ((ds::FileMetaBroker*)self->filter)->start();
      break;
    default:
      GST_ERROR_OBJECT(self, "mode property broken");
      return false;
      break;
  }
  return true;
}

/* stop the element and free external resources
 *
 * https://gstreamer.freedesktop.org/documentation/base/gstbasetransform.html?gi-language=c#GstBaseTransformClass::stop
 */

static gboolean gst_dspayloadbroker_stop(GstBaseTransform* base) {
  GstDsPayloadBroker* self = GST_DSPAYLOADBROKER(base);
  GST_DEBUG_OBJECT(self, "stop");
  /* destroy the DistanceFilter
   */

  switch (self->mode)
  {
    case PAYLOAD_BROKER_MODE_PROTO:
    case PAYLOAD_BROKER_MODE_CSV:
      ((ds::FileMetaBroker*)self->filter)->stop();
      break;
    default:
      break;
  }

  delete self->filter;
  self->filter = nullptr;

  return true;
}

/* do in-place work on the buffer (override the 'transform' method for copy)
 */
static GstFlowReturn gst_dspayloadbroker_transform_ip(GstBaseTransform* object,
                                                    GstBuffer* outbuf) {
  GstDsPayloadBroker* self = GST_DSPAYLOADBROKER(object);

  if (self->silent == FALSE)
    GST_LOG("%s got buffer.", GST_ELEMENT_NAME(self));

  if (GST_CLOCK_TIME_IS_VALID(GST_BUFFER_TIMESTAMP(outbuf)))
    gst_object_sync_values(GST_OBJECT(self), GST_BUFFER_TIMESTAMP(outbuf));

  return self->filter->on_buffer(outbuf);
}

/* __setattr__
 */
static void gst_dspayloadbroker_set_property(GObject* object,
                                           guint prop_id,
                                           const GValue* value,
                                           GParamSpec* pspec) {
  GstDsPayloadBroker* self = GST_DSPAYLOADBROKER(object);

  switch (prop_id) {
    case PROP_SILENT:
      self->silent = g_value_get_boolean(value);
      break;
    case PROP_RESULTS:
      G_OBJECT_WARN_INVALID_PSPEC(object, "results", prop_id, pspec);
      break;
    case PROP_MODE:
      self->mode = (GstDsPayloadBrokerMode) g_value_get_enum(value);
      break;
    case PROP_BASEPATH:
      g_free(self->basepath);
      self->basepath = g_value_dup_string(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

/* __getattr__
 */
static void gst_dspayloadbroker_get_property(GObject* object,
                                           guint prop_id,
                                           GValue* value,
                                           GParamSpec* pspec) {
  GstDsPayloadBroker* self = GST_DSPAYLOADBROKER(object);
  gchararray results = nullptr;
  ds::PyPayloadBroker* pybroker = nullptr;
  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean(value, self->silent);
      break;
    case PROP_RESULTS:
      if (self->mode != PAYLOAD_BROKER_MODE_PROPERTY 
          || self->filter == nullptr) {
        // i don't know if this is necessary but it (probably)
        // can't hurt
        g_value_set_string(value, nullptr);
        break;
      }
      pybroker = (ds::PyPayloadBroker*) self->filter;
      results = pybroker->get_payload();
      if (results != nullptr) {
        g_value_take_string(value, results);
      }
      break;
    case PROP_MODE:
      g_value_set_enum(value, self->mode);
      break;
    case PROP_BASEPATH:
      g_value_set_string(value, self->basepath);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}
