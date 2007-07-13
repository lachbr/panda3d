// Filename: finiteBoundingVolume.h
// Created by:  drose (02Oct99)
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

#ifndef FINITEBOUNDINGVOLUME_H
#define FINITEBOUNDINGVOLUME_H

#include "pandabase.h"

#include "geometricBoundingVolume.h"


////////////////////////////////////////////////////////////////////
//       Class : FiniteBoundingVolume
// Description : A special kind of GeometricBoundingVolume that is
//               known to be finite.  It is possible to query this
//               kind of volume for its minimum and maximum extents.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA FiniteBoundingVolume : public GeometricBoundingVolume {
PUBLISHED:
  virtual LPoint3f get_min() const=0;
  virtual LPoint3f get_max() const=0;
  virtual float get_volume() const;

public:
  virtual const FiniteBoundingVolume *as_finite_bounding_volume() const;

protected:
  virtual bool around_lines(const BoundingVolume **first,
                            const BoundingVolume **last);
  virtual bool around_planes(const BoundingVolume **first,
                            const BoundingVolume **last);

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    GeometricBoundingVolume::init_type();
    register_type(_type_handle, "FiniteBoundingVolume",
                  GeometricBoundingVolume::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

#endif



















