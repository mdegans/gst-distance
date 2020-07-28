#ifndef CONFIG_H__
#define CONFIG_H__

#pragma once

#ifdef HAVE_CONFIG_MESON_H
#include "config_meson.h"
#else
#include "config_dev.h"
#endif

#endif  // CONFIG_H__