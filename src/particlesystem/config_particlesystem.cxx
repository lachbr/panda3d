// Filename: config_particlesystem.cxx
// Created by:  charles (05Jul00)
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

#include "config_particlesystem.h"
#include "particleSystem.h"
#include "geomParticleRenderer.h"

ConfigureDef(config_particlesystem);
NotifyCategoryDef(particlesystem, "");

ConfigureFn(config_particlesystem) {
  init_libparticlesystem();
}

////////////////////////////////////////////////////////////////////
//     Function: init_libparticlesystem
//  Description: Initializes the library.  This must be called at
//               least once before any of the functions or classes in
//               this library can be used.  Normally it will be
//               called by the static initializers and need not be
//               called explicitly, but special cases exist.
////////////////////////////////////////////////////////////////////
void
init_libparticlesystem() {
  static bool initialized = false;
  if (initialized) {
    return;
  }
  initialized = true;

}

