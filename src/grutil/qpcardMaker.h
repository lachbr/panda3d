// Filename: qpcardMaker.h
// Created by:  drose (16Mar02)
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

#ifndef qpCARDMAKER_H
#define qpCARDMAKER_H

#include "pandabase.h"

#include "luse.h"
#include "pandaNode.h"
#include "pointerTo.h"
#include "namable.h"

////////////////////////////////////////////////////////////////////
//       Class : qpCardMaker
// Description : This class generates 2-d "cards", that is,
//               rectangular polygons, particularly useful for showing
//               textures etc. in the 2-d scene graph.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA qpCardMaker : public Namable {
PUBLISHED:
  INLINE qpCardMaker(const string &name);
  INLINE ~qpCardMaker();

  void reset();
  INLINE void set_uv_range(const TexCoordf &ll, const TexCoordf &ur);
  INLINE void set_has_uvs(bool flag);

  INLINE void set_frame(float left, float right, float bottom, float top);
  INLINE void set_frame(const LVecBase4f &frame);

  INLINE void set_color(float r, float g, float b, float a);
  INLINE void set_color(const Colorf &color);

  INLINE void set_source_geometry(PandaNode *node, const LVecBase4f &frame);
  INLINE void clear_source_geometry();

  PT(PandaNode) generate();

private:
  PT(PandaNode) rescale_source_geometry();

  bool _has_uvs;
  TexCoordf _ll, _ur;
  LVecBase4f _frame;

  bool _has_color;
  Colorf _color;

  PT(PandaNode) _source_geometry;
  LVecBase4f _source_frame;
};

#include "qpcardMaker.I"

#endif

