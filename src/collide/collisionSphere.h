// Filename: collisionSphere.h
// Created by:  drose (24Apr00)
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

#ifndef COLLISIONSPHERE_H
#define COLLISIONSPHERE_H

#include <pandabase.h>

#include "collisionSolid.h"

///////////////////////////////////////////////////////////////////
//       Class : CollisionSphere
// Description : A spherical collision volume or object.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA CollisionSphere : public CollisionSolid {
PUBLISHED:
  INLINE CollisionSphere(const LPoint3f &center, float radius);
  INLINE CollisionSphere(float cx, float cy, float cz, float radius);

public:
  INLINE CollisionSphere(const CollisionSphere &copy);
  virtual CollisionSolid *make_copy();

  virtual int
  test_intersection(qpCollisionHandler *record,
                    const qpCollisionEntry &entry,
                    const CollisionSolid *into) const;

  virtual void xform(const LMatrix4f &mat);
  virtual LPoint3f get_collision_origin() const;

  virtual void output(ostream &out) const;

PUBLISHED:
  INLINE void set_center(const LPoint3f &center);
  INLINE void set_center(float x, float y, float z);
  INLINE const LPoint3f &get_center() const;

  INLINE void set_radius(float radius);
  INLINE float get_radius() const;

protected:
  INLINE CollisionSphere(void);
  virtual BoundingVolume *recompute_bound();

protected:
  virtual int
  test_intersection_from_sphere(qpCollisionHandler *record,
                                const qpCollisionEntry &entry) const;
  virtual int
  test_intersection_from_ray(qpCollisionHandler *record,
                             const qpCollisionEntry &entry) const;
  virtual int
  test_intersection_from_segment(qpCollisionHandler *record,
                                 const qpCollisionEntry &entry) const;

  virtual void fill_viz_geom();

  bool intersects_line(double &t1, double &t2,
                       const LPoint3f &from, const LVector3f &delta) const;

private:
  LPoint3f _center;
  float _radius;

public:
  static void register_with_read_factory();
  virtual void write_datagram(BamWriter *manager, Datagram &me);

protected:
  static TypedWritable *make_CollisionSphere(const FactoryParams &params);
  void fillin(DatagramIterator &scan, BamReader *manager);

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    CollisionSolid::init_type();
    register_type(_type_handle, "CollisionSphere",
                  CollisionSolid::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

#include "collisionSphere.I"

#endif


