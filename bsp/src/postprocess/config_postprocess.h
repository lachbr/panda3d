/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file config_postprocess.h
 * @author Brian Lach
 * @date 2020-09-20
 */

#ifndef CONFIG_POSTPROCESS_H
#define CONFIG_POSTPROCESS_H

#include "dconfig.h"
#include "notifyCategoryProxy.h"

#ifdef BUILDING_BSP_POSTPROCESS
#define EXPCL_BSP_POSTPROCESS EXPORT_CLASS
#define EXPTP_BSP_POSTPROCESS EXPORT_TEMPL
#else
#define EXPCL_BSP_POSTPROCESS IMPORT_CLASS
#define EXPTP_BSP_POSTPROCESS IMPORT_TEMPL
#endif

ConfigureDecl(config_postprocess, EXPCL_BSP_POSTPROCESS, EXPTP_BSP_POSTPROCESS);

NotifyCategoryDecl(postprocess, EXPCL_BSP_POSTPROCESS, EXPTP_BSP_POSTPROCESS);

extern EXPCL_BSP_POSTPROCESS void init_libpostprocess();

#endif // CONFIG_POSTPROCESS_H
