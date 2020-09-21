#pragma once

#include "config_leveleditor.h"
#include "plane.h"

class EXPCL_EDITOR CPlane : public LPlane {
PUBLISHED:
  CPlane() = default;
  CPlane(PN_stdfloat a, PN_stdfloat b, PN_stdfloat c, PN_stdfloat d);

  static CPlane from_vertices(const LPoint3 &p1, const LPoint3 &p2, const LPoint3 &p3);
  static bool intersect(LPoint3 &intersection_point, const CPlane &p1,
                        const CPlane &p2, const CPlane &p3);

  bool get_intersection_point(LPoint3 &intersection_point, const LPoint3 &start,
                              const LPoint3 &end, bool ignore_direction = false,
                              bool ignore_segment = false) const;

  int on_plane(const LPoint3 &point, PN_stdfloat epsilon = 0.5) const;

  LVector3 get_closest_axis_to_normal() const;


};

INLINE CPlane::
CPlane(PN_stdfloat a, PN_stdfloat b, PN_stdfloat c, PN_stdfloat d) :
  LPlane(a, b, c, d) {
}

INLINE CPlane CPlane::
from_vertices(const LPoint3 &p1, const LPoint3 &p2, const LPoint3 &p3) {
  LVector3 ab = p2 - p1;
  LVector3 ac = p3 - p1;
  LVector3 normal = ac.cross(ab).normalized();
  PN_stdfloat dist = normal.dot(p1);

  return CPlane(normal[0], normal[1], normal[2], -dist);
}

INLINE bool CPlane::
intersect(LPoint3 &intersection_point, const CPlane &p1,
          const CPlane &p2, const CPlane &p3) {
  // Get the intersection point of 3 planes

  LVector3 n1 = p1.get_normal();
  LVector3 n2 = p2.get_normal();
  LVector3 n3 = p3.get_normal();

  LVector3 c1 = n2.cross(n3);
  LVector3 c2 = n3.cross(n1);
  LVector3 c3 = n1.cross(n2);

  PN_stdfloat denom = n1.dot(c1);
  if (denom < 0.00001) {
    return false;
  }

  LVector3 numer = (-p1[3] * c1) + (-p2[3] * c2) + (-p3[3] * c3);
  intersection_point = numer / denom;
  return true;
}

INLINE bool CPlane::
get_intersection_point(LPoint3 &intersection_point, const LPoint3 &start,
                       const LPoint3 &end, bool ignore_direction,
                       bool ignore_segment) const {
  LVector3 direction = end - start;
  LVector3 normal = get_normal();
  PN_stdfloat denom = -normal.dot(direction);
  PN_stdfloat num = normal.dot(start - normal * -get_w());

  if (fabs(denom) < 0.00001 || (!ignore_direction && denom < 0)) {
    return false;
  }

  PN_stdfloat u = num / denom;
  if (!ignore_segment && (u < 0 || u > 1)) {
    return false;
  }

  intersection_point = start + (direction * u);
  return true;
}

INLINE int CPlane::
on_plane(const LPoint3 &point, PN_stdfloat epsilon) const {
  PN_stdfloat res = dist_to_plane(point);
  if (fabs(res) < epsilon) {
    return 0;
  }

  if (res < 0) {
    return -1;
  }

  return 1;
}

INLINE LVector3 CPlane::
get_closest_axis_to_normal() const {
  // VHE prioritizes the axes in order of X, Y, Z

  LVector3 norm = get_normal();
  norm[0] = fabs(norm[0]);
  norm[1] = fabs(norm[1]);
  norm[2] = fabs(norm[2]);

  if (norm[0] >= norm[1] && norm[0] >= norm[2]) {
    return LVector3::unit_x();

  } else if (norm[1] >= norm[2]) {
    return LVector3::unit_y();

  } else {
    return LVector3::unit_z();
  }
}
