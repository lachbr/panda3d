// Filename: collisionEntry.h
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

#ifndef COLLISIONENTRY_H
#define COLLISIONENTRY_H

#include "pandabase.h"

#include "collisionSolid.h"
#include "collisionNode.h"

#include "typedReferenceCount.h"
#include "luse.h"
#include "pointerTo.h"
#include "pandaNode.h"
#include "nodePath.h"

///////////////////////////////////////////////////////////////////
//       Class : CollisionEntry
// Description : Defines a single collision event.  One of these is
//               created for each collision detected by a
//               CollisionTraverser, to be dealt with by the
//               CollisionHandler.
//
//               A CollisionEntry provides slots for a number of data
//               values (such as intersection point and normal) that
//               might or might not be known for each collision.  It
//               is up to the handler to determine what information is
//               known and to do the right thing with it.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA CollisionEntry : public TypedReferenceCount {
public:
  INLINE CollisionEntry();
  CollisionEntry(const CollisionEntry &copy);
  void operator = (const CollisionEntry &copy);

PUBLISHED:
  INLINE const CollisionSolid *get_from() const;
  INLINE bool has_into() const;
  INLINE const CollisionSolid *get_into() const;

  INLINE CollisionNode *get_from_node() const;
  INLINE PandaNode *get_into_node() const;
  INLINE const NodePath &get_into_node_path() const;

  INLINE const LMatrix4f &get_from_space() const;
  INLINE const LMatrix4f &get_into_space() const;
  INLINE const LMatrix4f &get_wrt_space() const;
  INLINE const LMatrix4f &get_inv_wrt_space() const;

  INLINE void set_into_intersection_point(const LPoint3f &point);
  INLINE bool has_into_intersection_point() const;
  INLINE const LPoint3f &get_into_intersection_point() const;

  INLINE bool has_from_intersection_point() const;
  INLINE LPoint3f get_from_intersection_point() const;

  INLINE void set_into_surface_normal(const LVector3f &normal);
  INLINE bool has_into_surface_normal() const;
  INLINE const LVector3f &get_into_surface_normal() const;

  INLINE void set_into_depth(float depth);
  INLINE bool has_into_depth() const;
  INLINE float get_into_depth() const;

private:
  CPT(CollisionSolid) _from;
  CPT(CollisionSolid) _into;

  PT(CollisionNode) _from_node;
  PT(PandaNode) _into_node;
  NodePath _into_node_path;
  LMatrix4f _from_space;
  LMatrix4f _into_space;
  LMatrix4f _wrt_space;
  LMatrix4f _inv_wrt_space;

  enum Flags {
    F_has_into_intersection_point = 0x0001,
    F_has_into_surface_normal     = 0x0002,
    F_has_into_depth              = 0x0004,
  };

  int _flags;

  LPoint3f _into_intersection_point;
  LVector3f _into_surface_normal;
  float _into_depth;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    TypedReferenceCount::init_type();
    register_type(_type_handle, "CollisionEntry",
                  TypedReferenceCount::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;

  friend class CollisionTraverser;
};

#include "collisionEntry.I"

#endif



