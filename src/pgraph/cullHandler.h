// Filename: cullHandler.h
// Created by:  drose (23Feb02)
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

#ifndef CULLHANDLER_H
#define CULLHANDLER_H

#include "pandabase.h"

class Geom;
class TransformState;
class RenderState;

////////////////////////////////////////////////////////////////////
//       Class : CullHandler
// Description : This defines the abstract interface for an object
//               that receives Geoms identified by the CullTraverser.
//               By itself, it's not a particularly useful class; to
//               use it, derive from it and redefine record_geom().
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA CullHandler {
public:
  //  virtual void begin_decal();
  virtual void record_geom(Geom *geom, const TransformState *transform,
                           const RenderState *state);
  //  virtual void push_decal();
  //  virtual void pop_decal();
};

#endif


  
