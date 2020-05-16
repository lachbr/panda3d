#ifndef CONFIG_RECASTNAVIGATION_H
#define CONFIG_RECASTNAVIGATION_H

#pragma once

#include "pandabase.h"
#include "notifyCategoryProxy.h"
#include "configVariableDouble.h"
#include "configVariableString.h"
#include "configVariableInt.h"

#ifdef BUILDING_P3RECAST
#define EXPCL_P3RECAST EXPORT_CLASS
#define EXPTP_P3RECAST EXPORT_TEMPL
#else
#define EXPCL_P3RECAST IMPORT_CLASS
#define EXPTP_P3RECAST IMPORT_TEMPL
#endif

using namespace std;

NotifyCategoryDecl(recastnavigation, EXPORT_CLASS, EXPORT_TEMPL);

extern void init_librecastnavigation();

#endif
