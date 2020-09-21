/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file config_raytrace.cxx
 * @author Brian Lach
 * @date 2020-09-21
 */

#include "config_raytrace.h"
#include "rayTraceGeometry.h"
#include "rayTraceTriangleMesh.h"

NotifyCategoryDef(raytrace, "");

ConfigureDef(config_raytrace);

ConfigureFn(config_raytrace) {
  init_libraytrace();
}

void
init_libraytrace() {
  static bool initialized = false;
  if (initialized) {
    return;
  }

  initialized = true;

  RayTraceGeometry::init_type();
  RayTraceTriangleMesh::init_type();
}
