// Filename: triangle.cxx
// Created by:  drose (16Nov99)
// 
////////////////////////////////////////////////////////////////////

#include "triangle.h"

inline int
is_right(const LVector2d &v1, const LVector2d &v2) {
  return (-v1[0] * v2[1] + v1[1] * v2[0]) < 0.0;
}

bool
triangle_contains_point(const LPoint2d &p, const LPoint2d &v0,
			const LPoint2d &v1, const LPoint2d &v2) {
  // In the case of a triangle defined with points in counterclockwise
  // order, a point is interior to the triangle iff the point is not
  // right of each of the edges.

  if (is_right(p - v0, v1 - v0)) {
    return false;
  }
  if (is_right(p - v1, v2 - v1)) {
    return false;
  }
  if (is_right(p - v2, v0 - v2)) {
    return false;
  }

  return true;
}

bool
triangle_contains_circle(const LPoint2d &p, double radius,
			 const LPoint2d &v0,
			 const LPoint2d &v1, const LPoint2d &v2) {
  // This is a cheesy hack.  Instead of performing an actual
  // triangle-circle intersection test, we simply move the point
  // radius units closer to the centroid of the triangle, and test
  // that point for intersection.

  LPoint2d centroid = (v0 + v1 + v2) / 3.0;

  LVector2d vec = centroid - p;
  double d = length(vec);
  if (d <= radius) {
    // We were already closer than radius distance from the centroid;
    // this is an automatic intersection.
    return true;
  }

  LPoint2d new_p = p + radius * (vec / d);
  return triangle_contains_point(new_p, v0, v1, v2);
}
