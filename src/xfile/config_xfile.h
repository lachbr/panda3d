// Filename: config_xfile.h
// Created by:  drose (22Jun01)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://www.panda3d.org/license.txt .
//
// To contact the maintainers of this program write to
// panda3d@yahoogroups.com .
//
////////////////////////////////////////////////////////////////////

#ifndef CONFIG_XFILE_H
#define CONFIG_XFILE_H

#include "pandatoolbase.h"

#include "notifyCategoryProxy.h"

NotifyCategoryDeclNoExport(xfile);

extern bool xfile_one_mesh;

extern void init_libxfile();

#endif
