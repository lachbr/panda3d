// Filename: linearJitterForce.cxx
// Created by:  charles (16Jun00)
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

#include "linearJitterForce.h"

TypeHandle LinearJitterForce::_type_handle;

////////////////////////////////////////////////////////////////////
//    Function : LinearJitterForce
//      Access : Public
// Description : constructor
////////////////////////////////////////////////////////////////////
LinearJitterForce::
LinearJitterForce(float a, bool mass) :
  LinearRandomForce(a, mass) {
}

////////////////////////////////////////////////////////////////////
//    Function : LinearJitterForce
//      Access : Public
// Description : copy constructor
////////////////////////////////////////////////////////////////////
LinearJitterForce::
LinearJitterForce(const LinearJitterForce &copy) :
  LinearRandomForce(copy) {
}

////////////////////////////////////////////////////////////////////
//    Function : LinearJitterForce
//      Access : Public
// Description : constructor
////////////////////////////////////////////////////////////////////
LinearJitterForce::
~LinearJitterForce(void) {
}

////////////////////////////////////////////////////////////////////
//    Function : make_copy
//      Access : Public
// Description : copier
////////////////////////////////////////////////////////////////////
LinearForce *LinearJitterForce::
make_copy(void) {
  return new LinearJitterForce(*this);
}

////////////////////////////////////////////////////////////////////
//    Function : get_child_vector
//      Access : Public
// Description : random value
////////////////////////////////////////////////////////////////////
LVector3f LinearJitterForce::
get_child_vector(const PhysicsObject *) {
  return random_unit_vector();
}
