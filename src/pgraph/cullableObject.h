// Filename: cullableObject.h
// Created by:  drose (04Mar02)
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

#ifndef CULLABLEOBJECT_H
#define CULLABLEOBJECT_H

#include "pandabase.h"

#include "geom.h"
#include "renderState.h"
#include "transformState.h"
#include "pointerTo.h"
#include "referenceCount.h"

////////////////////////////////////////////////////////////////////
//       Class : CullableObject
// Description : The smallest atom of cull.  This is normally just a
//               Geom and its associated state, but it also represent
//               a number of Geoms to be drawn together, with a number
//               of Geoms decalled onto them.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA CullableObject {
public:
  INLINE CullableObject(CullableObject *next = NULL);

private:
  INLINE CullableObject(const CullableObject &copy);
  INLINE void operator = (const CullableObject &copy);

public:
  ~CullableObject();

  void output(ostream &out) const;
  
  PT(Geom) _geom;
  CPT(RenderState) _state;
  CPT(TransformState) _transform;
  CullableObject *_next;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    register_type(_type_handle, "CullableObject");
  }

private:
  static TypeHandle _type_handle;
};

INLINE ostream &operator << (ostream &out, const CullableObject &object) {
  object.output(out);
  return out;
}

#include "cullableObject.I"

#endif
