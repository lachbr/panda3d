// Filename: config_pgui.cxx
// Created by:  drose (02Jul01)
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

#include "config_pgui.h"
#include "pgButton.h"
#include "pgEntry.h"
#include "pgMouseWatcherParameter.h"
#include "pgMouseWatcherGroup.h"
#include "pgItem.h"
#include "pgMouseWatcherRegion.h"
#include "pgTop.h"
#include "pgWaitBar.h"

#include "dconfig.h"

Configure(config_pgui);
NotifyCategoryDef(pgui, "");

ConfigureFn(config_pgui) {
  init_libpgui();
}

const bool pgui_quick = config_pgui.GetBool("pgui-quick", true);


////////////////////////////////////////////////////////////////////
//     Function: init_libpgui
//  Description: Initializes the library.  This must be called at
//               least once before any of the functions or classes in
//               this library can be used.  Normally it will be
//               called by the static initializers and need not be
//               called explicitly, but special cases exist.
////////////////////////////////////////////////////////////////////
void
init_libpgui() {
  static bool initialized = false;
  if (initialized) {
    return;
  }
  initialized = true;

  PGButton::init_type();
  PGEntry::init_type();
  PGMouseWatcherParameter::init_type();
  PGMouseWatcherGroup::init_type();
  PGItem::init_type();
  PGMouseWatcherRegion::init_type();
  PGTop::init_type();
  PGWaitBar::init_type();
}
