// Filename: collisionSphere.cxx
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


#include "collisionSphere.h"
#include "collisionRay.h"
#include "collisionSegment.h"
#include "collisionHandler.h"
#include "collisionEntry.h"
#include "config_collide.h"

#include <boundingSphere.h>
#include <geomNode.h>
#include <datagram.h>
#include <datagramIterator.h>
#include <bamReader.h>
#include <bamWriter.h>

#include <geomSphere.h>
#include <nearly_zero.h>

TypeHandle CollisionSphere::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: CollisionSphere::make_copy
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
CollisionSolid *CollisionSphere::
make_copy() {
  return new CollisionSphere(*this);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSphere::test_intersection
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
int CollisionSphere::
test_intersection(CollisionHandler *record, const CollisionEntry &entry,
                  const CollisionSolid *into) const {
  return into->test_intersection_from_sphere(record, entry);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSphere::xform
//       Access: Public, Virtual
//  Description: Transforms the solid by the indicated matrix.
////////////////////////////////////////////////////////////////////
void CollisionSphere::
xform(const LMatrix4f &mat) {
  _center = _center * mat;

  // This is a little cheesy and fails miserably in the presence of a
  // non-proportionate scale.
  LVector3f radius_v = LVector3f(_radius, 0.0, 0.0) * mat;
  _radius = length(radius_v);

  clear_viz_arcs();
  mark_bound_stale();
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSphere::get_collision_origin
//       Access: Public, Virtual
//  Description: Returns the point in space deemed to be the "origin"
//               of the solid for collision purposes.  The closest
//               intersection point to this origin point is considered
//               to be the most significant.
////////////////////////////////////////////////////////////////////
LPoint3f CollisionSphere::
get_collision_origin() const {
  return get_center();
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSphere::output
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
void CollisionSphere::
output(ostream &out) const {
  out << "csphere, c (" << _center << "), r " << _radius;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSphere::recompute_bound
//       Access: Protected, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
void CollisionSphere::
recompute_bound() {
  BoundedObject::recompute_bound();
  nassertv(_bound != (BoundingVolume*)0L);
  nassertv(!_center.is_nan() && !cnan(_radius));
  BoundingSphere sphere(_center, _radius);
  _bound->extend_by(&sphere);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSphere::test_intersection_from_sphere
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
int CollisionSphere::
test_intersection_from_sphere(CollisionHandler *record,
                              const CollisionEntry &entry) const {
  const CollisionSphere *sphere;
  DCAST_INTO_R(sphere, entry.get_from(), 0);

 //from_radius_v = LVector3f(sphere->get_radius(), 0.0, 0.0) * entry.get_wrt_space();
 //float from_radius = length(from_radius_v);

  const LMatrix4f *pMat = &entry.get_wrt_space();
  float from_radius = sphere->get_radius() *
                      sqrtf((*pMat)(0,0)*(*pMat)(0,0) + 
                            (*pMat)(0,1)*(*pMat)(0,1) + 
                            (*pMat)(0,2)*(*pMat)(0,2));

  LPoint3f from_center = sphere->get_center() * entry.get_wrt_space();
  LVector3f vec = from_center - _center;

  float dist2 = dot(vec, vec);
  float total_radius = _radius + from_radius;

  if (dist2 > total_radius * total_radius) {
    // No intersection.
    return 0;
  }

  if (collide_cat.is_debug()) {
    collide_cat.debug()
      << "intersection detected from " << *entry.get_from_node() << " into "
      << entry.get_into_node_path() << "\n";
  }
  PT(CollisionEntry) new_entry = new CollisionEntry(entry);

  float dist = sqrtf(dist2);
  LVector3f into_normal = normalize(vec);
  LPoint3f into_intersection_point = into_normal * (dist - from_radius);
  float into_depth = total_radius - dist;

  new_entry->set_into_surface_normal(into_normal * entry.get_inv_wrt_space());
  new_entry->set_into_intersection_point(into_intersection_point * entry.get_inv_wrt_space());
  new_entry->set_into_depth(into_depth);

  record->add_entry(new_entry);
  return 1;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSphere::test_intersection_from_ray
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
int CollisionSphere::
test_intersection_from_ray(CollisionHandler *record,
                           const CollisionEntry &entry) const {
  const CollisionRay *ray;
  DCAST_INTO_R(ray, entry.get_from(), 0);

  LPoint3f from_origin = ray->get_origin() * entry.get_wrt_space();
  LVector3f from_direction = ray->get_direction() * entry.get_wrt_space();

  double t1, t2;
  if (!intersects_line(t1, t2, from_origin, from_direction)) {
    // No intersection.
    return 0;
  }

  if (t2 < 0.0) {
    // Both intersection points are before the start of the ray.
    return 0;
  }

  if (collide_cat.is_debug()) {
    collide_cat.debug()
      << "intersection detected from " << *entry.get_from_node() << " into "
      << entry.get_into_node_path() << "\n";
  }
  PT(CollisionEntry) new_entry = new CollisionEntry(entry);

  LPoint3f into_intersection_point;
  if (t1 < 0.0) {
    into_intersection_point = from_origin + t2 * from_direction;
  } else {
    into_intersection_point = from_origin + t1 * from_direction;
  }
  new_entry->set_into_intersection_point(into_intersection_point);

  record->add_entry(new_entry);
  return 1;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSphere::test_intersection_from_segment
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
int CollisionSphere::
test_intersection_from_segment(CollisionHandler *record,
                               const CollisionEntry &entry) const {
  const CollisionSegment *segment;
  DCAST_INTO_R(segment, entry.get_from(), 0);

  LPoint3f from_a = segment->get_point_a() * entry.get_wrt_space();
  LPoint3f from_b = segment->get_point_b() * entry.get_wrt_space();
  LVector3f from_direction = from_b - from_a;

  double t1, t2;
  if (!intersects_line(t1, t2, from_a, from_direction)) {
    // No intersection.
    return 0;
  }

  if (t2 < 0.0 || t1 > 1.0) {
    // Both intersection points are before the start of the segment or
    // after the end of the segment.
    return 0;
  }

  if (collide_cat.is_debug()) {
    collide_cat.debug()
      << "intersection detected from " << *entry.get_from_node() << " into "
      << entry.get_into_node_path() << "\n";
  }
  PT(CollisionEntry) new_entry = new CollisionEntry(entry);

  LPoint3f into_intersection_point;
  if (t1 < 0.0) {
    // Point a is within the sphere.  The first intersection point is
    // point a itself.
    into_intersection_point = from_a;
  } else {
    // Point a is outside the sphere, and point b is either inside the
    // sphere or beyond it.  The first intersection point is at t1.
    into_intersection_point = from_a + t1 * from_direction;
  }
  new_entry->set_into_intersection_point(into_intersection_point);

  record->add_entry(new_entry);
  return 1;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSphere::recompute_viz
//       Access: Public, Virtual
//  Description: Rebuilds the geometry that will be used to render a
//               visible representation of the collision solid.
////////////////////////////////////////////////////////////////////
void CollisionSphere::
recompute_viz(Node *parent) {
  if (collide_cat.is_debug()) {
    collide_cat.debug()
      << "Recomputing viz for " << *this << " on " << *parent << "\n";
  }

  GeomSphere *sphere = new GeomSphere;
  PTA_Vertexf verts;
  verts.push_back(_center);
  verts.push_back(_center + LVector3f(_radius, 0.0, 0.0));
  sphere->set_coords(verts, G_PER_VERTEX);
  sphere->set_num_prims(1);

  GeomNode *viz = new GeomNode("viz-sphere");
  viz->add_geom(sphere);
  add_solid_viz(parent, viz);
  //  add_wireframe_viz(parent, viz);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSphere::intersects_line
//       Access: Protected
//  Description: Determine the point(s) of intersect of a parametric
//               line with the sphere.  The line is infinite in both
//               directions, and passes through "from" and from+delta.
//               If the line does not intersect the sphere, the
//               function returns false, and t1 and t2 are undefined.
//               If it does intersect the sphere, it returns true, and
//               t1 and t2 are set to the points along the equation
//               from+t*delta that correspond to the two points of
//               intersection.
////////////////////////////////////////////////////////////////////
bool CollisionSphere::
intersects_line(double &t1, double &t2,
                const LPoint3f &from, const LVector3f &delta) const {
  // Solve the equation for the intersection of a line with a sphere
  // using the quadratic formula.

  // A line segment from f to f+d is defined as all P such that
  // P = f + td for 0 <= t <= 1.

  // A sphere with radius r about point c is defined as all P such
  // that r^2 = (P - c)^2.

  // Subsituting P in the above we have:

  // r^2 = (f + td - c)^2 =
  // (f^2 + ftd - fc + ftd + t^2d^2 - tdc - fc - tdc + c^2) =
  // t^2(d^2) + t(fd + fd - dc - dc) + (f^2 - fc - fc + c^2) =
  // t^2(d^2) + t(2d(f - c)) + (f - c)^2

  // Thus, the equation is quadratic in t, and we have
  // at^2 + bt + c = 0

  // Where  a = d^2
  //        b = 2d(f - c)
  //        c = (f - c)^2 - r^2

  // Solving for t using the quadratic equation gives us the point of
  // intersection along the line segment.  Actually, there are two
  // solutions (since it is quadratic): one for the front of the
  // sphere, and one for the back.  In the case there the line is
  // tangent to the sphere, there is only one solution (and the
  // radical is zero).

  double A = dot(delta, delta);

  nassertr(A != 0.0, false);

  LVector3f fc = from - _center;
  double B = 2.0 * dot(delta, fc);
  double fc_d2 = dot(fc, fc);
  double C = fc_d2 - _radius * _radius;

  double radical = B*B - 4.0*A*C;

  if (IS_NEARLY_ZERO(radical)) {
    // Tangent.
    t1 = t2 = -B /(2.0*A);
    return true;
  } else

  if (radical < 0.0) {
    // No real roots: no intersection with the line.
    return false;
  }

  double reciprocal_2A = 1.0/(2.0*A);
  double sqrt_radical = sqrtf(radical);
  t1 = ( -B - sqrt_radical ) * reciprocal_2A;
  t2 = ( -B + sqrt_radical ) * reciprocal_2A;

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSphere::register_with_read_factory
//       Access: Public, Static
//  Description: Factory method to generate a CollisionSphere object
////////////////////////////////////////////////////////////////////
void CollisionSphere::
register_with_read_factory() {
  BamReader::get_factory()->register_factory(get_class_type(), make_CollisionSphere);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSphere::write_datagram
//       Access: Public
//  Description: Function to write the important information in
//               the particular object to a Datagram
////////////////////////////////////////////////////////////////////
void CollisionSphere::
write_datagram(BamWriter *manager, Datagram &me) {
  CollisionSolid::write_datagram(manager, me);
  _center.write_datagram(me);
  me.add_float32(_radius);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSphere::make_CollisionSphere
//       Access: Protected
//  Description: Factory method to generate a CollisionSphere object
////////////////////////////////////////////////////////////////////
TypedWritable *CollisionSphere::
make_CollisionSphere(const FactoryParams &params) {
  CollisionSphere *me = new CollisionSphere;
  DatagramIterator scan;
  BamReader *manager;

  parse_params(params, scan, manager);
  me->fillin(scan, manager);
  return me;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSphere::fillin
//       Access: Protected
//  Description: Function that reads out of the datagram (or asks
//               manager to read) all of the data that is needed to
//               re-create this object and stores it in the appropiate
//               place
////////////////////////////////////////////////////////////////////
void CollisionSphere::
fillin(DatagramIterator& scan, BamReader* manager) {
  CollisionSolid::fillin(scan, manager);
  _center.read_datagram(scan);
  _radius = scan.get_float32();
}
