// Filename: collisionPlane.h
// Created by:  drose (25Apr00)
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

#ifndef COLLISIONPLANE_H
#define COLLISIONPLANE_H

#include <pandabase.h>

#include "collisionSolid.h"

#include <luse.h>
#include <plane.h>

///////////////////////////////////////////////////////////////////
//       Class : CollisionPlane
// Description :
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA CollisionPlane : public CollisionSolid {
protected:
  INLINE CollisionPlane();

PUBLISHED:
  INLINE CollisionPlane(const Planef &plane);
  INLINE CollisionPlane(const CollisionPlane &copy);

public:
  virtual CollisionSolid *make_copy();

  virtual int
  test_intersection(CollisionHandler *record,
                    const CollisionEntry &entry,
                    const CollisionSolid *into) const;

  virtual int
  test_intersection(qpCollisionHandler *record,
                    const qpCollisionEntry &entry,
                    const CollisionSolid *into) const;

  virtual void xform(const LMatrix4f &mat);
  virtual LPoint3f get_collision_origin() const;

  virtual void output(ostream &out) const;

PUBLISHED:
  INLINE LVector3f get_normal() const;
  INLINE float dist_to_plane(const LPoint3f &point) const;

  INLINE void set_plane(const Planef &plane);
  INLINE const Planef &get_plane() const;

protected:
  virtual BoundingVolume *recompute_bound();

protected:
  virtual int
  test_intersection_from_sphere(CollisionHandler *record,
                                const CollisionEntry &entry) const;
  virtual int
  test_intersection_from_ray(CollisionHandler *record,
                             const CollisionEntry &entry) const;
  virtual int
  test_intersection_from_sphere(qpCollisionHandler *record,
                                const qpCollisionEntry &entry) const;
  virtual int
  test_intersection_from_ray(qpCollisionHandler *record,
                             const qpCollisionEntry &entry) const;

  virtual void recompute_viz(Node *parent);
  virtual void fill_viz_geom();

private:
  Planef _plane;

public:
  static void register_with_read_factory(void);
  virtual void write_datagram(BamWriter* manager, Datagram &me);

  static TypedWritable *make_CollisionPlane(const FactoryParams &params);

protected:
  void fillin(DatagramIterator& scan, BamReader* manager);

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    CollisionSolid::init_type();
    register_type(_type_handle, "CollisionPlane",
                  CollisionSolid::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

#include "collisionPlane.I"

#endif


