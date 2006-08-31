// Filename: config_glxdisplay.h
// Created by:  cary (07Oct99)
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

#ifndef __CONFIG_GLXDISPLAY_H__
#define __CONFIG_GLXDISPLAY_H__

#include "pandabase.h"
#include "notifyCategoryProxy.h"
#include "configVariableString.h"
#include "configVariableBool.h"
#include "configVariableInt.h"

NotifyCategoryDecl(glxdisplay, EXPCL_PANDAGL, EXPTP_PANDAGL);

extern EXPCL_PANDAGL void init_libglxdisplay();

extern ConfigVariableString display_cfg;
extern ConfigVariableBool glx_error_abort;
extern ConfigVariableBool glx_get_proc_address;
extern ConfigVariableBool glx_get_os_address;

extern ConfigVariableInt glx_wheel_up_button;
extern ConfigVariableInt glx_wheel_down_button;

#endif /* __CONFIG_GLXDISPLAY_H__ */
