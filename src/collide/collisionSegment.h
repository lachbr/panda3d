// Filename: collisionSegment.h
// Created by:  drose (30Jan01)
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

#ifndef COLLISIONSEGMENT_H
#define COLLISIONSEGMENT_H

#include <pandabase.h>

#include "collisionSolid.h"

class ProjectionNode;

///////////////////////////////////////////////////////////////////
//       Class : CollisionSegment
// Description : A finite line segment, with two specific endpoints
//               but no thickness.  It's similar to a CollisionRay,
//               except it does not continue to infinity.
//
//               It does have an ordering, from point A to point B.
//               If more than a single point of the segment is
//               intersecting a solid, the reported intersection point
//               is generally the closest on the segment to point A.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA CollisionSegment : public CollisionSolid {
PUBLISHED:
  INLINE CollisionSegment();
  INLINE CollisionSegment(const LPoint3f &a, const LPoint3f &db);
  INLINE CollisionSegment(float ax, float ay, float az,
                          float bx, float by, float bz);

public:
  INLINE CollisionSegment(const CollisionSegment &copy);
  virtual CollisionSolid *make_copy();

  virtual int
  test_intersection(CollisionHandler *record,
                    const CollisionEntry &entry,
                    const CollisionSolid *into) const;

  virtual void xform(const LMatrix4f &mat);
  virtual LPoint3f get_collision_origin() const;

  virtual void output(ostream &out) const;

PUBLISHED:
  INLINE void set_point_a(const LPoint3f &a);
  INLINE void set_point_a(float x, float y, float z);
  INLINE const LPoint3f &get_point_a() const;

  INLINE void set_point_b(const LPoint3f &b);
  INLINE void set_point_b(float x, float y, float z);
  INLINE const LPoint3f &get_point_b() const;

  bool set_projection(ProjectionNode *camera, const LPoint2f &point);
  INLINE bool set_projection(ProjectionNode *camera, float px, float py);

protected:
  virtual void recompute_bound();

protected:
  virtual void recompute_viz(Node *parent);

private:
  LPoint3f _a, _b;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    CollisionSolid::init_type();
    register_type(_type_handle, "CollisionSegment",
                  CollisionSolid::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

#include "collisionSegment.I"

#endif


