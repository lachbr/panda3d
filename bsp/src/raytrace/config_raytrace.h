/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file config_raytrace.h
 * @author Brian Lach
 * @date 2020-09-21
 */

#ifndef CONFIG_RAYTRACE_H
#define CONFIG_RAYTRACE_H

#include "dconfig.h"
#include "dtoolbase.h"
#include "notifyCategoryProxy.h"

#ifdef BUILDING_BSP_RAYTRACE
#define EXPCL_BSP_RAYTRACE EXPORT_CLASS
#define EXPTP_BSP_RAYTRACE EXPORT_TEMPL
#else
#define EXPCL_BSP_RAYTRACE IMPORT_CLASS
#define EXPTP_BSP_RAYTRACE IMPORT_CLASS
#endif

NotifyCategoryDecl(raytrace, EXPCL_BSP_RAYTRACE, EXPTP_BSP_RAYTRACE);

ConfigureDecl(config_raytrace, EXPCL_BSP_RAYTRACE, EXPTP_BSP_RAYTRACE);

// embree forward decls
struct RTCDeviceTy;
typedef struct RTCDeviceTy* RTCDevice;
struct RTCSceneTy;
typedef struct RTCSceneTy* RTCScene;
struct RTCGeometryTy;
typedef struct RTCGeometryTy* RTCGeometry;

extern EXPCL_BSP_RAYTRACE void init_libraytrace();

#endif // CONFIG_RAYTRACE_H
