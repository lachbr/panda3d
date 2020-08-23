#include "cPolygon.h"
#include "cLine.h"

CPolygon::
CPolygon(const CPlane &plane, PN_stdfloat radius) {
  LVector3 norm = plane.get_normal();
  LPoint3 point = plane.get_point();
  LVector3 direction = plane.get_closest_axis_to_normal();
  LVector3 tempv = direction == LVector3::unit_z() ? -LVector3::unit_y() : -LVector3::unit_z();
  LVector3 up = tempv.cross(norm).normalized();
  LVector3 right = norm.cross(up).normalized();

  _vertices = {
    point + right + up, // top right
    point - right + up, // top left
    point - right - up, // bottom left
    point + right - up, // bottom right
  };

  _plane = plane;
  expand(radius);
}

void CPolygon::
simplify() {
  size_t i = 0;
  while (1) {
    size_t count = _vertices.size() - 2;
    if (i >= count) {
      break;
    }

    const LPoint3 &v1 = _vertices[i];
    const LPoint3 &v2 = _vertices[i + 2];
    const LPoint3 &p = _vertices[i + 1];
    CLine line(v1, v2);
    // If the midpoint is on the line, remove it
    if (line.closest_point(p).almost_equal(p)) {
      _vertices.erase(_vertices.begin() + (i + 1));
    }

    i++;
  }
}

bool CPolygon::
split(const CPlane &plane, PT(CPolygon) *front, PT(CPolygon) *back,
             PT(CPolygon) *cfront, PT(CPolygon) *cback) {
  *front = *back = *cfront = *cback = nullptr;

  PlaneClassification classify = classify_against_plane(plane);
  if (classify != PlaneClassification::Spanning) {
    if (classify == PlaneClassification::Back) {
      *back = this;

    } else if (classify == PlaneClassification::Front) {
      *front = this;

    } else if (_plane.get_normal().dot(plane.get_normal()) > 0) {
      *cfront = this;

    } else {
      *cback = this;
    }
  }

  // Get the new front and back vertices
  pvector<LPoint3> back_verts;
  pvector<LPoint3> front_verts;
  int prev = 0;

  size_t count = _vertices.size();
  for (size_t i = 0; i < count + 1; i++) {
    const LPoint3 &end = _vertices[i % count];
    int cls = plane.on_plane(end);

    // Check plane crossing
    if (i > 0 && cls != 0 && prev != 0 && prev != cls) {
      // This line end point has crossed the plane
      const LPoint3 &start = _vertices[i - 1];

      LPoint3 isect;
      bool ret = plane.get_intersection_point(isect, start, end, true);
      nassertr(ret, false);

      back_verts.push_back(isect);
      front_verts.push_back(isect);
    }

    // Add original points
    if (i < count) {
      // Points on the plane get put in both polygons.
      if (cls >= 0) {
        front_verts.push_back(end);
      }
      if (cls <= 0) {
        back_verts.push_back(end);
      }
    }

    prev = cls;
  }

  *back = new CPolygon(back_verts);
  *front = new CPolygon(front_verts);
  *cback = *cfront = nullptr;

  return true;
}
