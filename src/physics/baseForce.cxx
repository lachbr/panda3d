// Filename: baseForce.cxx
// Created by:  charles (08Aug00)
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

#include "baseForce.h"

TypeHandle BaseForce::_type_handle;

////////////////////////////////////////////////////////////////////
//    Function : BaseForce
//      Access : protected
// Description : constructor
////////////////////////////////////////////////////////////////////
BaseForce::
BaseForce(bool active) :
  _force_node(NULL), _active(active) {
}

////////////////////////////////////////////////////////////////////
//    Function : BaseForce
//      Access : protected
// Description : copy constructor
////////////////////////////////////////////////////////////////////
BaseForce::
BaseForce(const BaseForce &copy) :
  TypedReferenceCount(copy) {
  _active = copy._active;
  _force_node = (ForceNode *) NULL;
}

////////////////////////////////////////////////////////////////////
//    Function : ~BaseForce
//      Access : public, virtual
// Description : destructor
////////////////////////////////////////////////////////////////////
BaseForce::
~BaseForce(void) {
}
