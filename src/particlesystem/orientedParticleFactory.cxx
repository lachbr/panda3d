// Filename: orientedParticleFactory.cxx
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

#include "orientedParticleFactory.h"
#include "orientedParticle.h"

////////////////////////////////////////////////////////////////////
//    Function : OrientedParticleFactory
//      Access : Public
// Description : Constructor
////////////////////////////////////////////////////////////////////
OrientedParticleFactory::
OrientedParticleFactory(void) :
  BaseParticleFactory() {
}

////////////////////////////////////////////////////////////////////
//    Function : OrientedParticleFactory
//      Access : Public
// Description : copy Constructor
////////////////////////////////////////////////////////////////////
OrientedParticleFactory::
OrientedParticleFactory(const OrientedParticleFactory &copy) :
  BaseParticleFactory(copy) {
  _initial_orientation = copy._initial_orientation;
  _final_orientation = copy._final_orientation;
}

////////////////////////////////////////////////////////////////////
//    Function : ~OrientedParticleFactory
//      Access : public
// Description : destructor
////////////////////////////////////////////////////////////////////
OrientedParticleFactory::
~OrientedParticleFactory(void) {
}

////////////////////////////////////////////////////////////////////
//    Function : populate_child_particle
//      Access : private
// Description : child spawn
////////////////////////////////////////////////////////////////////
void OrientedParticleFactory::
populate_child_particle(BaseParticle *bp) const {
  bp->set_orientation(_initial_orientation);
}

////////////////////////////////////////////////////////////////////
//    Function : alloc_particle
//      Access : public
// Description : child particle generation function
////////////////////////////////////////////////////////////////////
BaseParticle *OrientedParticleFactory::
alloc_particle(void) const {
  return new OrientedParticle;
}
