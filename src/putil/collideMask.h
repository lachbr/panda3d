// Filename: collideMask.h
// Created by:  drose (03Jul00)
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

#ifndef COLLIDEMASK_H
#define COLLIDEMASK_H

#include <pandabase.h>

#include "bitMask.h"

// This is the data type of the collision mask: the set of bits that
// every CollisionNode has, and that any two nodes must have some in
// common in order to be tested for a mutual intersection.

// This file used to live in the collide directory, but since it's
// such a trivial definition that a few other directories (like egg)
// need without necessarily having to pull in all of collide, it
// seemed better to move it to putil.

typedef BitMask32 CollideMask;

#endif

