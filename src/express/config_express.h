// Filename: config_express.h
// Created by:  cary (04Jan00)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001 - 2004, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://etc.cmu.edu/panda3d/docs/license/ .
//
// To contact the maintainers of this program write to
// panda3d-general@lists.sourceforge.net .
//
////////////////////////////////////////////////////////////////////

#ifndef __CONFIG_EXPRESS_H__
#define __CONFIG_EXPRESS_H__

#include "pandabase.h"
#include "notifyCategoryProxy.h"
#include "dconfig.h"

// We include these files to force them to be instrumented by
// interrogate.
#include "pandaSystem.h"
#include "globPattern.h"
#include "configFlags.h"
#include "configPage.h"
#include "configPageManager.h"
#include "configVariable.h"
#include "configVariableBase.h"
#include "configVariableBool.h"
#include "configVariableDouble.h"
#include "configVariableFilename.h"
#include "configVariableInt.h"
#include "configVariableList.h"
#include "configVariableManager.h"
#include "configVariableSearchPath.h"
#include "configVariableString.h"

ConfigureDecl(config_express, EXPCL_PANDAEXPRESS, EXPTP_PANDAEXPRESS);
NotifyCategoryDecl(express, EXPCL_PANDAEXPRESS, EXPTP_PANDAEXPRESS);

// Actually, we can't determine this config variable the normal way,
// because we must be able to access it at static init time.  Instead
// of declaring it a global constant, we'll make it a member of
// MemoryUsage.

//extern EXPCL_PANDAEXPRESS const bool track_memory_usage;

EXPCL_PANDAEXPRESS bool get_use_high_res_clock();
EXPCL_PANDAEXPRESS bool get_paranoid_clock();
EXPCL_PANDAEXPRESS bool get_paranoid_inheritance();
EXPCL_PANDAEXPRESS bool get_verify_dcast();

extern ConfigVariableInt patchfile_window_size;
extern ConfigVariableInt patchfile_increment_size;
extern ConfigVariableInt patchfile_buffer_size;
extern ConfigVariableInt patchfile_zone_size;

extern ConfigVariableBool keep_temporary_files;

extern ConfigVariableBool lock_to_one_cpu;

extern ConfigVariableString encryption_algorithm;
extern ConfigVariableInt encryption_key_length;
extern ConfigVariableInt encryption_iteration_count;
extern ConfigVariableInt multifile_encryption_iteration_count;

extern ConfigVariableBool vfs_case_sensitive;
extern ConfigVariableBool vfs_implicit_pz;
extern ConfigVariableBool vfs_auto_unwrap_pz;

extern EXPCL_PANDAEXPRESS ConfigVariableBool use_vfs;

extern EXPCL_PANDAEXPRESS ConfigVariableBool collect_tcp;
extern EXPCL_PANDAEXPRESS ConfigVariableDouble collect_tcp_interval;

// Expose the Config variable for Python access.
BEGIN_PUBLISH
typedef Config::Config<ConfigureGetConfig_config_express> ConfigExpress;
EXPCL_PANDAEXPRESS ConfigExpress &get_config_express();
END_PUBLISH

extern EXPCL_PANDAEXPRESS void init_libexpress();

#endif /* __CONFIG_UTIL_H__ */
