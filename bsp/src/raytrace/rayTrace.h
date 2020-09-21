/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file rayTrace.h
 * @author Brian Lach
 * @date 2020-09-21
 */

#ifndef BSP_RAYTRACE_H
#define BSP_RAYTRACE_H

#include "config_raytrace.h"

/**
 *
 */
class EXPCL_BSP_RAYTRACE RayTrace {
PUBLISHED:
  static void initialize();
  static void destruct();

public:
  INLINE static RTCDevice get_device();

private:
  static bool _initialized;
  static RTCDevice _device;
};

#include "rayTrace.I"

#endif // BSP_RAYTRACE_H
