// Filename: angularIntegrator.cxx
// Created by:  charles (09Aug00)
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

#include "angularIntegrator.h"

////////////////////////////////////////////////////////////////////
//    Function : AngularIntegrator
//      Access : protected
// Description : constructor
////////////////////////////////////////////////////////////////////
AngularIntegrator::
AngularIntegrator(void) {
}

////////////////////////////////////////////////////////////////////
//    Function : ~AngularIntegrator
//      Access : public, virtual
// Description : destructor
////////////////////////////////////////////////////////////////////
AngularIntegrator::
~AngularIntegrator(void) {
}

////////////////////////////////////////////////////////////////////
//    Function : Integrate
//      Access : public
// Description : high-level integration.  API.
////////////////////////////////////////////////////////////////////
void AngularIntegrator::
integrate(Physical *physical, pvector< PT(AngularForce) >& forces,
          float dt) {
  // intercept in case we want to censor/adjust values
  if (dt > _max_angular_dt)
    dt = _max_angular_dt;

  // this actually does the integration.
  child_integrate(physical, forces, dt);
}
