// Filename: config_interval.cxx
// Created by:  drose (27Aug02)
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

#include "config_interval.h"
#include "cInterval.h"
#include "cLerpInterval.h"
#include "cLerpNodePathInterval.h"
#include "cLerpAnimEffectInterval.h"
#include "cMetaInterval.h"
#include "showInterval.h"
#include "hideInterval.h"

#include "dconfig.h"

Configure(config_interval);
NotifyCategoryDef(interval, "");

ConfigureFn(config_interval) {
  init_libinterval();
}

// Set this to the default value for set_precision() for each
// CMetaInterval created.
const double interval_precision = config_interval.GetDouble("interval-precision", 1000.0);

// Set this true to generate an assertion failure if interval
// functions are called out-of-order.
const bool verify_intervals = config_interval.GetBool("verify-intervals", false);

////////////////////////////////////////////////////////////////////
//     Function: init_libinterval
//  Description: Initializes the library.  This must be called at
//               least once before any of the functions or classes in
//               this library can be used.  Normally it will be
//               called by the static initializers and need not be
//               called explicitly, but special cases exist.
////////////////////////////////////////////////////////////////////
void
init_libinterval() {
  static bool initialized = false;
  if (initialized) {
    return;
  }
  initialized = true;

  CInterval::init_type();
  CLerpInterval::init_type();
  CLerpNodePathInterval::init_type();
  CLerpAnimEffectInterval::init_type();
  CMetaInterval::init_type();
  ShowInterval::init_type();
  HideInterval::init_type();
}

