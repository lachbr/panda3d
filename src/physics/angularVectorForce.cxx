// Filename: angularVectorForce.cxx
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

#include "angularVectorForce.h"

TypeHandle AngularVectorForce::_type_handle;

////////////////////////////////////////////////////////////////////
//    Function : AngularVectorForce
//      Access : public
// Description : constructor
////////////////////////////////////////////////////////////////////
AngularVectorForce::
AngularVectorForce(const LVector3f &vec) :
  AngularForce(), _fvec(vec) {
}

////////////////////////////////////////////////////////////////////
//    Function : AngularVectorForce
//      Access : public
// Description : constructor
////////////////////////////////////////////////////////////////////
AngularVectorForce::
AngularVectorForce(float x, float y, float z) :
  AngularForce() {
  _fvec.set(x, y, z);
}

////////////////////////////////////////////////////////////////////
//    Function : AngularVectorForce
//      Access : public
// Description : copy constructor
////////////////////////////////////////////////////////////////////
AngularVectorForce::
AngularVectorForce(const AngularVectorForce &copy) :
  AngularForce(copy) {
  _fvec = copy._fvec;
}

////////////////////////////////////////////////////////////////////
//    Function : ~AngularVectorForce
//      Access : public, virtual
// Description : destructor
////////////////////////////////////////////////////////////////////
AngularVectorForce::
~AngularVectorForce(void) {
}

////////////////////////////////////////////////////////////////////
//    Function : make_copy
//      Access : private, virtual
// Description : dynamic copier
////////////////////////////////////////////////////////////////////
AngularForce *AngularVectorForce::
make_copy(void) const {
  return new AngularVectorForce(*this);
}

////////////////////////////////////////////////////////////////////
//    Function : get_child_vector
//      Access : private, virtual
// Description : query
////////////////////////////////////////////////////////////////////
LVector3f AngularVectorForce::
get_child_vector(const PhysicsObject *) {
  return _fvec;
}
