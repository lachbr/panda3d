// Filename: linearSinkForce.cxx
// Created by:  charles (21Jun00)
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

#include "linearSinkForce.h"

TypeHandle LinearSinkForce::_type_handle;

////////////////////////////////////////////////////////////////////
//    Function : LinearSinkForce
//      Access : Public
// Description : Simple constructor
////////////////////////////////////////////////////////////////////
LinearSinkForce::
LinearSinkForce(const LPoint3f& p, FalloffType f, float r, float a,
          bool mass) :
  LinearDistanceForce(p, f, r, a, mass) {
}

////////////////////////////////////////////////////////////////////
//    Function : LinearSinkForce
//      Access : Public
// Description : Simple constructor
////////////////////////////////////////////////////////////////////
LinearSinkForce::
LinearSinkForce(void) :
  LinearDistanceForce(LPoint3f(0.0f, 0.0f, 0.0f), FT_ONE_OVER_R_SQUARED,
                1.0f, 1.0f, true) {
}

////////////////////////////////////////////////////////////////////
//    Function : LinearSinkForce
//      Access : Public
// Description : copy constructor
////////////////////////////////////////////////////////////////////
LinearSinkForce::
LinearSinkForce(const LinearSinkForce &copy) :
  LinearDistanceForce(copy) {
}

////////////////////////////////////////////////////////////////////
//    Function : ~LinearSinkForce
//      Access : Public
// Description : Simple destructor
////////////////////////////////////////////////////////////////////
LinearSinkForce::
~LinearSinkForce(void) {
}

////////////////////////////////////////////////////////////////////
//    Function : make_copy
//      Access : Public
// Description : copier
////////////////////////////////////////////////////////////////////
LinearForce *LinearSinkForce::
make_copy(void) {
  return new LinearSinkForce(*this);
}

////////////////////////////////////////////////////////////////////
//    Function : get_child_vector
//      Access : Public
// Description : virtual force query
////////////////////////////////////////////////////////////////////
LVector3f LinearSinkForce::
get_child_vector(const PhysicsObject *po) {
  return (get_force_center() - po->get_position()) * get_scalar_term();
}
