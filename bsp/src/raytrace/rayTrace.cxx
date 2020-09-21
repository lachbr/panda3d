/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file rayTrace.cxx
 * @author Brian Lach
 * @date 2020-09-21
 */

#include "rayTrace.h"

#include <geomVertexReader.h>

#include <embree3/rtcore.h>

bool RayTrace::_initialized = false;
RTCDevice RayTrace::_device = nullptr;

/**
 * Initializes the ray trace device.
 */
void RayTrace::
initialize() {
  if ( _initialized ) {
    return;
  }

  _device = rtcNewDevice( "" );

  _initialized = true;
}

/**
 * Destructs the ray trace device.
 */
void RayTrace::
destruct() {
  _initialized = false;
  if ( _device )
    rtcReleaseDevice( _device );
  _device = nullptr;
}
