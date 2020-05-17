#ifndef CONFIG_RECASTNAVIGATION_H
#define CONFIG_RECASTNAVIGATION_H

#pragma once

#include "pandabase.h"
#include "notifyCategoryProxy.h"
#include "configVariableDouble.h"
#include "configVariableString.h"
#include "configVariableInt.h"

using namespace std;

NotifyCategoryDeclNoExport(recastnavigation);

extern void init_librecastnavigation();

#endif
