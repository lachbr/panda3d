// Filename: drawCullHandler.h
// Created by:  drose (25Feb02)
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

#ifndef DRAWCULLHANDLER_H
#define DRAWCULLHANDLER_H

#include "pandabase.h"
#include "cullHandler.h"

class GraphicsStateGuardian;

////////////////////////////////////////////////////////////////////
//       Class : DrawCullHandler
// Description : This special kind of CullHandler immediately draws
//               its contents as soon as it receives them.  This draws
//               geometry immediately as it is encountered in the
//               scene graph by cull, mixing the draw and cull
//               traversals into one traversal, and prohibiting state
//               sorting.  However, it has somewhat lower overhead
//               than separating out draw and cull, if state sorting
//               and multiprocessing are not required.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA DrawCullHandler : public CullHandler {
public:
  INLINE DrawCullHandler(GraphicsStateGuardian *gsg);

  //  virtual void begin_decal();
  virtual void record_geom(Geom *geom, const TransformState *transform,
                           const RenderState *state);
  //  virtual void push_decal();
  //  virtual void pop_decal();

private:
  GraphicsStateGuardian *_gsg;
};

#include "drawCullHandler.I"

#endif


  
