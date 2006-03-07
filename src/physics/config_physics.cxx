// Filename: config_physics.cxx
// Created by:  charles (17Jul00)
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

#include "config_physics.h"
#include "physicsCollisionHandler.h"
#include "physicsObject.h"
#include "physicalNode.h"
#include "linearIntegrator.h"
#include "linearControlForce.h"
#include "angularIntegrator.h"
#include "forceNode.h"
#include "forces.h"
#include "actorNode.h"
#include "angularForce.h"
#include "angularVectorForce.h"

#include "dconfig.h"

ConfigureDef(config_physics);
NotifyCategoryDef(physics, "");

ConfigureFn(config_physics) {
  init_libphysics();
}


////////////////////////////////////////////////////////////////////
//     Function: init_libphysics
//  Description: Initializes the library.  This must be called at
//               least once before any of the functions or classes in
//               this library can be used.  Normally it will be
//               called by the static initializers and need not be
//               called explicitly, but special cases exist.
////////////////////////////////////////////////////////////////////
void
init_libphysics() {
  static bool initialized = false;
  if (initialized) {
    return;
  }
  initialized = true;

  ActorNode::init_type();
  AngularForce::init_type();
  AngularVectorForce::init_type();
  BaseForce::init_type();
  ForceNode::init_type();
  LinearControlForce::init_type();
  LinearCylinderVortexForce::init_type();
  LinearDistanceForce::init_type();
  LinearForce::init_type();
  LinearFrictionForce::init_type();
  LinearJitterForce::init_type();
  LinearNoiseForce::init_type();
  LinearRandomForce::init_type();
  LinearSinkForce::init_type();
  LinearSourceForce::init_type();
  LinearUserDefinedForce::init_type();
  LinearVectorForce::init_type();
  Physical::init_type();
  PhysicsCollisionHandler::init_type();
  PhysicalNode::init_type();
  PhysicsObject::init_type();
}
