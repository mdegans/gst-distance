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

#include "gstdistance.h"

#include "config.h"
#include "gstdsdistance.h"
#include "gstdsprotopayload.h"
#include "gstdspayloadbroker.h"

static gboolean distance_init(GstPlugin* plugin) {
  if (!gst_element_register(plugin, "dsdistance", GST_RANK_NONE,
                            GST_TYPE_DSDISTANCE)) {
    GST_ERROR("could not register dsdistance");
    return false;
  };
  if (!gst_element_register(plugin, "dsprotopayload", GST_RANK_NONE,
                            GST_TYPE_DSPROTOPAYLOAD)) {
    GST_ERROR("could not register dsprotopayload");
    return false;
  };
  if (!gst_element_register(plugin, "dspayloadbroker", GST_RANK_NONE,
                            GST_TYPE_DSPAYLOADBROKER)) {
    GST_ERROR("could not register dspayloadbroker");
    return false;
  };
  return true;
}

/** gstreamer looks for this structure to register the plugin */
// clang-format off
GST_PLUGIN_DEFINE(
  GST_VERSION_MAJOR,
  GST_VERSION_MINOR,
  distance,
  PACKAGE_DESCRIPTION,
  distance_init,
  PACKAGE_VERSION,
  GST_LICENSE,
  GST_PACKAGE_NAME,
  GST_PACKAGE_ORIGIN
)
// clang-format on
