/* GStreamer
 *
 * Copyright (C) 2020 Micheal de Gans <michael.john.degans@gmail.com>
 * Copyright (C) 2006 Thomas Vander Stichele <thomas at apestaart dot org>
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <gst/check/check.h>

#include "gstdsdistance.h"

static const char* ELEMENT_NAME = "dsdistance";
static const char* ELEMENT_TYPE_NAME = "GstDsDistance";

// test pad formats
static const char* ELEMENT_CAPS_TEST_NV12_STR =
    "video/x-raw(memory:NVMM), format=(string)NV12, width=1280, height=720, "
    "framerate=(fraction)1/30";
static const char* ELEMENT_CAPS_TEST_RGBA_STR =
    "video/x-raw(memory:NVMM), format=(string)RGBA, width=1280, height=720, "
    "framerate=(fraction)1/30";

// pad templates
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

/* basic tests */

GST_START_TEST(test_setup_teardown) {
  GstElement* filter = gst_check_setup_element(ELEMENT_NAME);
  gst_check_teardown_element(filter);
}
GST_END_TEST;

GST_START_TEST(test_type) {
  GstElement* filter = gst_check_setup_element(ELEMENT_NAME);

  ck_assert(GST_IS_ELEMENT(filter));
  ck_assert(GST_IS_DSDISTANCE(filter));
  ck_assert_str_eq(G_OBJECT_TYPE_NAME(filter), ELEMENT_TYPE_NAME);

  gst_check_teardown_element(filter);
}
GST_END_TEST;

GST_START_TEST(test_name_property) {
  GstElement* filter;
  filter = gst_element_factory_make(ELEMENT_NAME, "foo");

  ck_assert_str_eq(GST_ELEMENT_NAME(filter), "foo");

  gst_check_teardown_element(filter);
}
GST_END_TEST;

GST_START_TEST(test_autoptr) {
  // this just tests it doensn't crash
  // TODO(mdegans): research if this test is adequate, or even necessary
  g_autoptr(GstElement) filter = gst_element_factory_make(ELEMENT_NAME, nullptr);
}
GST_END_TEST;

// https://gstreamer.freedesktop.org/documentation/check/gstcheck.html?gi-language=c#gst_check_setup_src_pad_by_name
static inline void _test_pads(const gchar* caps_str) {
  GstElement* filter = gst_check_setup_element(ELEMENT_NAME);
  GstPad* src_pad = gst_check_setup_src_pad(filter, &src_template);
  GstPad* sink_pad = gst_check_setup_sink_pad(filter, &sink_template);
  GstCaps* caps = gst_caps_from_string(caps_str);

  // activate pads
  ck_assert(gst_pad_set_active(src_pad, TRUE));
  ck_assert(gst_pad_set_active(sink_pad, TRUE));

  // activate element
  ck_assert(gst_element_set_state(filter, GST_STATE_PLAYING) ==
            GST_STATE_CHANGE_SUCCESS);

  // check source and sink pad setup
  gst_check_setup_events(src_pad, filter, caps, GST_FORMAT_TIME);

  // shutdown element
  ck_assert(gst_element_set_state(filter, GST_STATE_NULL) ==
            GST_STATE_CHANGE_SUCCESS);

  // check cleanup
  gst_caps_unref(caps);
  gst_check_object_destroyed_on_unref(src_pad);
  gst_check_object_destroyed_on_unref(sink_pad);
  gst_check_teardown_element(filter);
}

GST_START_TEST(test_pads_nv12) {
  _test_pads(ELEMENT_CAPS_TEST_NV12_STR);
}
GST_END_TEST;

GST_START_TEST(test_pads_rgba) {
  _test_pads(ELEMENT_CAPS_TEST_RGBA_STR);
}
GST_END_TEST;

GST_START_TEST(test_silent_property) {
  GstElement* filter;
  filter = gst_element_factory_make(ELEMENT_NAME, nullptr);

  gboolean returned;

  GST_DEBUG("** test silent proprety set true");
  g_object_set(filter, "silent", true, nullptr);

  GST_DEBUG("** test silent proprety get true");
  g_object_get(filter, "silent", &returned, nullptr);
  g_assert_true(returned);

  // TODO(mdegans): check the property silences stuff

  GST_DEBUG("** test silent property set false");
  g_object_set(filter, "silent", false, nullptr);

  GST_DEBUG("** test silent proprety get false");
  g_object_get(filter, "silent", &returned, nullptr);
  g_assert_false(returned);

  gst_object_unref(filter);
}
GST_END_TEST;

/* harness tests */
/* https://gstreamer.freedesktop.org/documentation/check/gstharness.html */

static inline void _test_harness_passthrough(const char* caps_str) {
  GstHarness* h;
  GstBuffer* in_buf;
  GstBuffer* out_buf;

  // create a new harness
  h = gst_harness_new(ELEMENT_NAME);
  g_assert_nonnull(h);

  // set caps
  gst_harness_set_src_caps_str(h, caps_str);
  gst_harness_set_sink_caps_str(h, caps_str);

  // create a buffer of size 42
  in_buf = gst_harness_create_buffer(h, 42);

  // push the buffer into the element
  gst_harness_push(h, in_buf);

  // pull the buffer out
  out_buf = gst_harness_pull(h);

  // validate in_buf is the same as out_buf
  fail_unless(in_buf == out_buf);

  // cleanup
  gst_buffer_unref(out_buf);
  gst_harness_teardown(h);
}

GST_START_TEST(test_harness_nv12_passthrough) {
  _test_harness_passthrough(ELEMENT_CAPS_TEST_NV12_STR);
}
GST_END_TEST;

GST_START_TEST(test_harness_rgba_passthrough) {
  _test_harness_passthrough(ELEMENT_CAPS_TEST_RGBA_STR);
}
GST_END_TEST;

static inline void _test_with_src(const gchar* src_str) {
  GstHarness* h;

  h = gst_harness_new(ELEMENT_NAME);
  g_assert_nonnull(h);

  // add a source from string
  gst_harness_add_src_parse(h, src_str, TRUE);
  // add a fake sink
  gst_harness_add_sink(h, "fakesink");

  gst_harness_play(h);

  gst_harness_teardown(h);
}


GST_START_TEST(test_simple_pipeline) {
  const gchar * src_str = "videotestsrc pattern=snow ! video/x-raw,width=1280,height=720 ! nvvideoconvert";
  _test_with_src(src_str);
}
GST_END_TEST;

static Suite* dsdistance_suite(void) {
  Suite* s = suite_create(ELEMENT_NAME);
  TCase* bc = tcase_create("basic");
  TCase* cc = tcase_create("check");
  TCase* hc = tcase_create("harness");
  TCase* ic = tcase_create("integration");

  suite_add_tcase(s, bc);
  tcase_add_test(bc, test_setup_teardown);
  tcase_add_test(bc, test_autoptr);
  tcase_add_test(bc, test_type);
  tcase_add_test(bc, test_name_property);
  tcase_add_test(bc, test_silent_property);

  suite_add_tcase(s, cc);
  tcase_add_test(cc, test_pads_nv12);
  tcase_add_test(cc, test_pads_rgba);

  suite_add_tcase(s, hc);
  tcase_add_test(hc, test_harness_nv12_passthrough);
  tcase_add_test(hc, test_harness_rgba_passthrough);

  suite_add_tcase(s, ic);
  tcase_add_test(ic, test_simple_pipeline);

  return s;
}

GST_CHECK_MAIN(dsdistance);
