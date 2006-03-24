// Filename: cullBinFrontToBack.h
// Created by:  drose (29May02)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001 - 2004, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://etc.cmu.edu/panda3d/docs/license/ .
//
// To contact the maintainers of this program write to
// panda3d-general@lists.sourceforge.net .
//
////////////////////////////////////////////////////////////////////

#ifndef CULLBINFRONTTOBACK_H
#define CULLBINFRONTTOBACK_H

#include "pandabase.h"

#include "cullBin.h"
#include "geom.h"
#include "transformState.h"
#include "renderState.h"
#include "pointerTo.h"

////////////////////////////////////////////////////////////////////
//       Class : CullBinFrontToBack
// Description : A specific kind of CullBin that sorts geometry in
//               order from nearest to furthest based on the center of
//               its bounding volume.
//
//               This is useful for rendering opaque geometry, taking
//               optimal advantage of a hierarchical Z-buffer.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA CullBinFrontToBack : public CullBin {
public:
  INLINE CullBinFrontToBack(const string &name, GraphicsStateGuardianBase *gsg);
  virtual ~CullBinFrontToBack();

  static CullBin *make_bin(const string &name, GraphicsStateGuardianBase *gsg);

  virtual void add_object(CullableObject *object);
  virtual void finish_cull();
  virtual void draw();

private:
  class ObjectData {
  public:
    INLINE ObjectData(CullableObject *object, float dist);
    INLINE bool operator < (const ObjectData &other) const;
    
    CullableObject *_object;
    float _dist;
  };

  typedef pvector<ObjectData> Objects;
  Objects _objects;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    CullBin::init_type();
    register_type(_type_handle, "CullBinFrontToBack",
                  CullBin::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

#include "cullBinFrontToBack.I"

#endif


  
