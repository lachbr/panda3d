// Filename: collisionPolygon.cxx
// Created by:  drose (25Apr00)
// 
////////////////////////////////////////////////////////////////////

#include "collisionPolygon.h"
#include "collisionHandler.h"
#include "collisionEntry.h"
#include "collisionSphere.h"
#include "collisionRay.h"
#include "config_collide.h"

#include <boundingSphere.h>
#include <pointerToArray.h>
#include <geomNode.h>
#include <geom.h>
#include <geomPolygon.h>
#include <algorithm>
#include <datagram.h>
#include <datagramIterator.h>
#include <bamReader.h>
#include <bamWriter.h>

TypeHandle CollisionPolygon::_type_handle;



////////////////////////////////////////////////////////////////////
//     Function: is_right
//  Description: Returns true if the 2-d v1 is to the right of v2.
////////////////////////////////////////////////////////////////////
INLINE bool
is_right(const LVector2f &v1, const LVector2f &v2) {
  return (-v1[0] * v2[1] + v1[1] * v2[0]) > 0;
}


////////////////////////////////////////////////////////////////////
//     Function: CollisionPolygon::Copy Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
CollisionPolygon::
CollisionPolygon(const CollisionPolygon &copy) :
  CollisionPlane(copy),
  _points(copy._points),
  _median(copy._median),
  _axis(copy._axis),
  _reversed(copy._reversed)
{
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPolygon::make_copy
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
CollisionSolid *CollisionPolygon::
make_copy() {
  return new CollisionPolygon(*this);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPolygon::test_intersection
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
int CollisionPolygon::
test_intersection(CollisionHandler *, const CollisionEntry &, 
		  const CollisionSolid *into) const {
  // Polygons cannot currently be intersected from, only into.  Do not
  // add a CollisionPolygon to a CollisionTraverser.
  nassertr(false, 0);
  return 0;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPolygon::xform
//       Access: Public, Virtual
//  Description: Transforms the solid by the indicated matrix.
////////////////////////////////////////////////////////////////////
void CollisionPolygon::
xform(const LMatrix4f &mat) {
  // We need to convert all the vertices to 3-d for this operation,
  // and then convert them back.  Hopefully we won't lose too much
  // precision during all of this.

#ifndef NDEBUG
  if (collide_cat.is_debug()) {
    collide_cat.debug()
      << "CollisionPolygon transformed by:\n";
    mat.write(collide_cat.debug(false), 2);
    if (_points.empty()) {
      collide_cat.debug(false)
	<< "  (no points)\n";
    }
  }
#endif

  if (!_points.empty()) {
    vector<LPoint3f> verts;
    Points::const_iterator pi;
    for (pi = _points.begin(); pi != _points.end(); ++pi) {
      verts.push_back(to_3d(*pi) * mat);
    }
    if (_reversed) {
      reverse(verts.begin(), verts.end());
    }
    setup_points(verts.begin(), verts.end());
  }
    
  clear_viz_arcs();
  mark_bound_stale();
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPolygon::output
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void CollisionPolygon::
output(ostream &out) const {
  out << "cpolygon";
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPolygon::recompute_bound
//       Access: Protected, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void CollisionPolygon::
recompute_bound() {
  // First, get ourselves a fresh, empty bounding volume.
  BoundedObject::recompute_bound();
  assert(_bound != (BoundingVolume*)0L);

  GeometricBoundingVolume *gbv = DCAST(GeometricBoundingVolume, _bound);

  // Now actually compute the bounding volume by putting it around all
  // of our vertices.
  vector<LPoint3f> vertices;
  Points::const_iterator pi;
  for (pi = _points.begin(); pi != _points.end(); ++pi) {
    vertices.push_back(to_3d(*pi));
  }

  gbv->around(vertices.begin(), vertices.end());
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPolygon::test_intersection_from_sphere
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
int CollisionPolygon::
test_intersection_from_sphere(CollisionHandler *record,
			      const CollisionEntry &entry) const {
  if (_points.size() < 3) {
    return 0;
  }

  const CollisionSphere *sphere;
  DCAST_INTO_R(sphere, entry.get_from(), 0);

  LPoint3f from_center = sphere->get_center() * entry.get_wrt_space();
  LVector3f from_radius_v = 
    LVector3f(sphere->get_radius(), 0.0, 0.0) * entry.get_wrt_space();
  float from_radius = length(from_radius_v);

  float dist = dist_to_plane(from_center);
  if (dist > from_radius || dist < -from_radius) {
    // No intersection.
    return 0;
  }

  // Ok, we intersected the plane, but did we intersect the polygon?

  // The nearest point within the plane to our center is the
  // intersection of the line (center, center+normal) with the plane.
  LPoint3f plane_point;
  bool really_intersects = 
    get_plane().intersects_line(plane_point,
				from_center, from_center + get_normal());
  nassertr(really_intersects, 0);

  LPoint2f p = to_2d(plane_point);

  // Now we have found a point on the polygon's plane that corresponds
  // to the point tangent to our collision sphere where it first
  // touches the plane.  We want to decide whether the sphere itself
  // will intersect the polygon.  We can approximate this by testing
  // whether a circle of the given radius centered around this tangent
  // point, in the plane of the polygon, would intersect.
  
  // But even this approximate test is too difficult.  To approximate
  // the approximation, we'll test two points: (1) the center itself.
  // If this is inside the polygon, then certainly the circle
  // intersects the polygon, and the sphere collides.  (2) a point on
  // the outside of the circle, nearest to the center of the polygon.
  // If _this_ point is inside the polygon, then again the circle, and
  // hence the sphere, intersects.  If neither point is inside the
  // polygon, chances are reasonably good the sphere doesn't intersect
  // the polygon after all.

  if (is_inside(p)) {
    // The circle's center is inside the polygon; we have a collision!

  } else {

    if (from_radius > 0.0) {
      // Now find the point on the rim of the circle nearest the
      // polygon's center.

      // First, get a vector from the center of the circle to the center
      // of the polygon.
      LVector2f rim = _median - p;
      float rim_length = length(rim);

      if (rim_length <= from_radius) {
	// Here's a surprise: the center of the polygon is within the
	// circle!  Since the center is guaranteed to be interior to the
	// polygon (the polygon is convex), it follows that the circle
	// intersects the polygon.

      } else {
	// Now scale this vector to length radius, and get the new point.
	rim = (rim * from_radius / rim_length) + p;

	// Is the new point within the polygon?
	if (is_inside(rim)) {
	  // It sure is!  The circle intersects!

	} else {
	  // No intersection.
	  return 0;
	}
      }
    }
  }

  if (collide_cat.is_debug()) {
    collide_cat.debug()
      << "intersection detected from " << *entry.get_from_node() << " into " 
      << *entry.get_into_node() << "\n";
  }
  PT(CollisionEntry) new_entry = new CollisionEntry(entry);

  LVector3f into_normal = get_normal() * entry.get_inv_wrt_space();
  float into_depth = from_radius - dist;

  new_entry->set_into_surface_normal(into_normal);
  new_entry->set_into_depth(into_depth);

  record->add_entry(new_entry);
  return 1;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPolygon::test_intersection_from_ray
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
int CollisionPolygon::
test_intersection_from_ray(CollisionHandler *record,
			   const CollisionEntry &entry) const {
  if (_points.size() < 3) {
    return 0;
  }

  const CollisionRay *ray;
  DCAST_INTO_R(ray, entry.get_from(), 0);

  LPoint3f from_origin = ray->get_origin() * entry.get_wrt_space();
  LVector3f from_direction = ray->get_direction() * entry.get_wrt_space();

  float t;
  if (!get_plane().intersects_line(t, from_origin, from_direction)) {
    // No intersection.
    return 0;
  }

  if (t < 0.0) { 
    // The intersection point is before the start of the ray.
    return 0;
  }

  LPoint3f plane_point = from_origin + t * from_direction;
  if (!is_inside(to_2d(plane_point))) {
    // Outside the polygon's perimeter.
    return 0;
  }

  if (collide_cat.is_debug()) {
    collide_cat.debug()
      << "intersection detected from " << *entry.get_from_node() << " into " 
      << *entry.get_into_node() << "\n";
  }
  PT(CollisionEntry) new_entry = new CollisionEntry(entry);

  new_entry->set_into_intersection_point(plane_point);

  record->add_entry(new_entry);
  return 1;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPolygon::recompute_viz
//       Access: Public, Virtual
//  Description: Rebuilds the geometry that will be used to render a
//               visible representation of the collision solid.
////////////////////////////////////////////////////////////////////
void CollisionPolygon::
recompute_viz(Node *parent) {
  if (collide_cat.is_debug()) {
    collide_cat.debug()
      << "Recomputing viz for " << *this << " on " << *parent << "\n";
  }

  if (_points.size() < 3) {
    if (collide_cat.is_debug()) {
      collide_cat.debug()
	<< "(Degenerate poly, ignoring.)\n";
    }
    return;
  }

  PTA_Vertexf verts;
  Points::const_iterator pi;
  for (pi = _points.begin(); pi != _points.end(); ++pi) {
    verts.push_back(to_3d(*pi));
  }
  if (_reversed) {
    reverse(verts.begin(), verts.end());
  }
  
  PTA_int lengths;
  lengths.push_back(_points.size());

  GeomPolygon *polygon = new GeomPolygon;
  polygon->set_coords(verts, G_PER_VERTEX);
  polygon->set_num_prims(1);
  polygon->set_lengths(lengths);

  GeomNode *viz = new GeomNode("viz-polygon");
  viz->add_geom(polygon);
  add_solid_viz(parent, viz);
  add_wireframe_viz(parent, viz);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPolygon::is_inside
//       Access: Private
//  Description: 
////////////////////////////////////////////////////////////////////
bool CollisionPolygon::
is_inside(const LPoint2f &p) const {
  // We insist that the polygon be convex.  This makes things a bit simpler.

  // In the case of a convex polygon, defined with points in counterclockwise
  // order, a point is interior to the polygon iff the point is not right of
  // each of the edges.

  for (int i = 0; i < _points.size() - 1; i++) {
    if (is_right(p - _points[i], _points[i+1] - _points[i])) {
      return false;
    }
  }
  if (is_right(p - _points[_points.size() - 1], 
	       _points[0] - _points[_points.size() - 1])) {
    return false; 
  }

  return true;

}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPolygon::setup_points
//       Access: Private
//  Description: 
////////////////////////////////////////////////////////////////////
void CollisionPolygon::
setup_points(const LPoint3f *begin, const LPoint3f *end) {
  int num_points = end - begin;
  nassertv(num_points >= 3);

  _points.clear();

  // Tell the base CollisionPlane class what its plane will be.  We
  // can determine this from the first three 3-d points.
  Planef plane(begin[0], begin[1], begin[2]);
  set_plane(plane);

  LVector3f normal = get_normal();

#ifndef NDEBUG
  // Make sure all the source points are good.
  {
    float normal_length = normal.length();
    bool all_ok = IS_THRESHOLD_EQUAL(normal_length, 1.0, 0.001);

    const LPoint3f *pi;
    for (pi = begin; pi != end && all_ok; ++pi) {
      if ((*pi).is_nan()) {
	all_ok = false;
      } else {
	// Make sure no points are repeated.
	const LPoint3f *pj;
	for (pj = begin; pj != pi && all_ok; ++pj) {
	  if ((*pj).almost_equal(*pi)) {
	    all_ok = false;
	  }
	}
      }
    }

    if (!all_ok) {
      collide_cat.error() << "Invalid points passed to CollisionPolygon:\n";
      for (pi = begin; pi != end; ++pi) {
	collide_cat.error(false) << "  " << (*pi) << "\n";
      }
      collide_cat.error(false) 
	<< "  normal " << normal << " with length " << normal_length << "\n";
      
      nassertv(false);
    }
  }

  if (collide_cat.is_debug()) {
    collide_cat.debug()
      << "CollisionPolygon defined with " << num_points << " vertices:\n";
    const LPoint3f *pi;
    for (pi = begin; pi != end; ++pi) {
      collide_cat.debug(false) << "  " << (*pi) << "\n";
    }
  }
#endif

  // First determine the largest of |normal[0]|, |normal[1]|, and
  // |normal[2]|.  This will tell us which axis-aligned plane the
  // polygon is most nearly aligned with, and therefore which plane we
  // should project onto for determining interiorness of the
  // intersection point.

  if (fabs(normal[0]) >= fabs(normal[1])) {
    if (fabs(normal[0]) >= fabs(normal[2])) {
      _axis = AT_x;
    } else {
      _axis = AT_z;
    }
  } else {
    if (fabs(normal[1]) >= fabs(normal[2])) {
      _axis = AT_y;
    } else {
      _axis = AT_z;
    }
  }

  // Now project all of the points onto the 2-d plane.

  const LPoint3f *pi;
  switch (_axis) {
  case AT_x:
    for (pi = begin; pi != end; ++pi) {
      const LPoint3f &point = (*pi);
      _points.push_back(LPoint2f(point[1], point[2]));
    }
    break;

  case AT_y:
    for (pi = begin; pi != end; ++pi) {
      const LPoint3f &point = (*pi);
      _points.push_back(LPoint2f(point[0], point[2]));
    }
    break;

  case AT_z:
    for (pi = begin; pi != end; ++pi) {
      const LPoint3f &point = (*pi);
      _points.push_back(LPoint2f(point[0], point[1]));
    }
    break;
  }

  nassertv(_points.size() >= 3);

  // One other complication: In projecting the polygon onto the plane,
  // we might have lost its counterclockwise-vertex orientation.  If
  // this is the case, we must reverse the order of the vertices.
  _reversed = is_right(_points[2] - _points[0], _points[1] - _points[0]);
  if (_reversed) {
    reverse(_points.begin(), _points.end());
  }

  // Finally, average up all the points to get the median.  This is
  // the geometric center of the polygon, and (since the polygon must
  // be convex) is also a point within the polygon.

  _median = _points[0];
  for (int n = 1; n < _points.size(); n++) {
    _median += _points[n];
  }
  _median /= _points.size();
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPolygon::to_2d
//       Access: Private
//  Description: Assuming the indicated point in 3-d space lies within
//               the polygon's plane, returns the corresponding point
//               in the polygon's 2-d definition space.
////////////////////////////////////////////////////////////////////
LPoint2f CollisionPolygon::
to_2d(const LPoint3f &point3d) const {
  nassertr(!point3d.is_nan(), LPoint2f(0.0, 0.0));

  // Project the point of intersection with the plane onto the
  // axis-aligned plane we projected the polygon onto, and see if the
  // point is interior to the polygon.
  switch (_axis) {
  case AT_x:
    return LPoint2f(point3d[1], point3d[2]);
    
  case AT_y:
    return LPoint2f(point3d[0], point3d[2]);
    
  case AT_z:
    return LPoint2f(point3d[0], point3d[1]);
  }

  nassertr(false, LPoint2f(0.0, 0.0));
  return LPoint2f(0.0, 0.0);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPolygon::to_3d
//       Access: Private
//  Description: Extrude the indicated point in the polygon's 2-d
//               definition space back into 3-d coordinates.
////////////////////////////////////////////////////////////////////
LPoint3f CollisionPolygon::
to_3d(const LPoint2f &point2d) const {
  nassertr(!point2d.is_nan(), LPoint3f(0.0, 0.0, 0.0));

  LVector3f normal = get_normal();
  float D = get_plane()._d;

  nassertr(!normal.is_nan(), LPoint3f(0.0, 0.0, 0.0));
  nassertr(!cnan(D), LPoint3f(0.0, 0.0, 0.0));

  switch (_axis) {
  case AT_x:
    return LPoint3f(-(normal[1]*point2d[0] + normal[2]*point2d[1] + D)/normal[0],
		    point2d[0], point2d[1]);

  case AT_y:
    return LPoint3f(point2d[0],
		    -(normal[0]*point2d[0] + normal[2]*point2d[1] + D)/normal[1],
		    point2d[1]);

  case AT_z:
    return LPoint3f(point2d[0], point2d[1],
		    -(normal[0]*point2d[0] + normal[1]*point2d[1] + D)/normal[2]);
  }

  nassertr(false, LPoint3f(0.0, 0.0, 0.0));
  return LPoint3f(0.0, 0.0, 0.0);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPolygon::write_datagram
//       Access: Public
//  Description: Function to write the important information in
//               the particular object to a Datagram
////////////////////////////////////////////////////////////////////
void CollisionPolygon::
write_datagram(BamWriter *manager, Datagram &me)
{
  int i;

  CollisionPlane::write_datagram(manager, me);
  me.add_uint16(_points.size());
  for(i = 0; i < _points.size(); i++)
  {
    _points[i].write_datagram(me);
  }
  _median.write_datagram(me);
  me.add_uint8(_axis);
  me.add_uint8(_reversed);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPolygon::fillin
//       Access: Protected
//  Description: Function that reads out of the datagram (or asks
//               manager to read) all of the data that is needed to
//               re-create this object and stores it in the appropiate
//               place
////////////////////////////////////////////////////////////////////
void CollisionPolygon::
fillin(DatagramIterator& scan, BamReader* manager)
{
  int i;
  LPoint2f temp;
  CollisionPlane::fillin(scan, manager);
  int size = scan.get_uint16();
  for(i = 0; i < size; i++)
  {
    temp.read_datagram(scan);
    _points.push_back(temp);
  }
  _median.read_datagram(scan);
  _axis = (enum AxisType)scan.get_uint8();

  // It seems that Windows wants this expression to prevent a
  // 'performance warning'.  Whatever.
  _reversed = (scan.get_uint8() != 0);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPolygon::make_CollisionPolygon
//       Access: Protected
//  Description: Factory method to generate a CollisionPolygon object
////////////////////////////////////////////////////////////////////
TypedWriteable* CollisionPolygon::
make_CollisionPolygon(const FactoryParams &params)
{
  CollisionPolygon *me = new CollisionPolygon;
  BamReader *manager;
  Datagram packet;

  parse_params(params, manager, packet);
  DatagramIterator scan(packet);

  me->fillin(scan, manager);
  return me;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPolygon::register_with_factory
//       Access: Public, Static
//  Description: Factory method to generate a CollisionPolygon object
////////////////////////////////////////////////////////////////////
void CollisionPolygon::
register_with_read_factory(void)
{
  BamReader::get_factory()->register_factory(get_class_type(), make_CollisionPolygon);
}


