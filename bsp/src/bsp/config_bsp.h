/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file config_bsp.h
 * @author Brian Lach
 * @date March 27, 2018
 */

#ifndef CONFIG_BSP_H
#define CONFIG_BSP_H

#include "dconfig.h"
#include "renderAttrib.h"

#ifdef BUILDING_LIBPANDABSP
#define EXPCL_PANDABSP EXPORT_CLASS
#define EXPTP_PANDABSP EXPORT_TEMPL
#else
#define EXPCL_PANDABSP IMPORT_CLASS
#define EXPTP_PANDABSP IMPORT_TEMPL
#endif

// For some reason interrogate wigs out on this
#ifndef CPPPARSER
extern EXPCL_PANDABSP vector_string parse_cmd(const std::string &cmd);

ConfigureDecl(config_bsp, EXPCL_PANDABSP, EXPTP_PANDABSP);
#endif

#ifndef CPPPARSER
extern EXPCL_PANDABSP void init_libbsp();
#endif

#endif // CONFIG_BSP_H
