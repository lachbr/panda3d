#pragma once

#include "config_editor.h"
#include "cPlane.h"
#include "luse.h"
#include "pvector.h"
#include "planeClassification.h"
#include "referenceCount.h"

class EXPCL_EDITOR CPolygon : public ReferenceCount {
PUBLISHED:
  CPolygon() = default;
  INLINE CPolygon(const pvector<LPoint3> &vertices, const CPlane &plane);
  INLINE CPolygon(const pvector<LPoint3> &vertices);
  CPolygon(const CPlane &plane, PN_stdfloat radius = 10000);

  INLINE bool is_valid() const;

  void simplify();

  INLINE void xform(const LMatrix4 &mat);
  INLINE void flip();
  INLINE bool is_convex(PN_stdfloat epsilon = 0.001) const;
  INLINE LPoint3 get_origin() const;
  INLINE void expand(PN_stdfloat radius);
  INLINE PlaneClassification classify_against_plane(const CPlane &plane) const;

  INLINE bool split(const CPlane &clip_plane);

  size_t get_num_vertices() const;
  const LPoint3 &get_vertex(size_t n) const;

public:
  bool split(const CPlane &clip_plane, PT(CPolygon) *front, PT(CPolygon) *back,
             PT(CPolygon) *cfront, PT(CPolygon) *cback);

private:
  pvector<LPoint3> _vertices;
  CPlane _plane;
};

INLINE CPolygon::
CPolygon(const pvector<LPoint3> &vertices, const CPlane &plane) {
  _vertices = vertices;
  _plane = plane;
}

INLINE CPolygon::
CPolygon(const pvector<LPoint3> &vertices) {
  _vertices = vertices;
  _plane = CPlane::from_vertices(vertices[0], vertices[1], vertices[2]);
  simplify();
}

INLINE size_t CPolygon::
get_num_vertices() const {
  return _vertices.size();
}

INLINE const LPoint3 &CPolygon::
get_vertex(size_t n) const {
  return _vertices[n];
}

INLINE bool CPolygon::
is_valid() const {
  size_t num_verts = _vertices.size();
  for (size_t i = 0; i < num_verts; i++) {
    if (_plane.on_plane(_vertices[i]) != 0) {
      // Vert doesn't lie within the plane.
      return false;
    }
  }

  return true;
}

INLINE void CPolygon::
xform(const LMatrix4 &mat) {
  size_t num_verts = _vertices.size();
  for (size_t i = 0; i < num_verts; i++) {
    _vertices[i] = mat.xform_point(_vertices[i]);
  }

  _plane = CPlane::from_vertices(_vertices[0], _vertices[1], _vertices[2]);
}

INLINE void CPolygon::
flip() {
  std::reverse(_vertices.begin(), _vertices.end());
  _plane.flip();
}

INLINE bool CPolygon::
is_convex(PN_stdfloat epsilon) const {
  size_t count = _vertices.size();
  for (size_t i = 0; i < count; i++) {
    LPoint3 v1 = _vertices[i];
    LPoint3 v2 = _vertices[(i + 1) % count];
    LPoint3 v3 = _vertices[(i + 2) % count];
    LVector3 l1 = (v1 - v2).normalized();
    LVector3 l2 = (v3 - v2).normalized();
    LVector3 cross = l1.cross(l2);
    if (fabs(_plane.dist_to_plane(v2 + cross)) > epsilon) {
      return false;
    }
  }

  return true;
}

INLINE LPoint3 CPolygon::
get_origin() const {
  LPoint3 origin(0);
  size_t count = _vertices.size();
  for (size_t i = 0; i < count; i++) {
    origin += _vertices[i];
  }
  return origin / count;
}

INLINE void CPolygon::
expand(PN_stdfloat radius) {
  LPoint3 origin = get_origin();
  size_t count = _vertices.size();
  for (size_t i = 0; i < count; i++) {
    _vertices[i] = (_vertices[i] - origin).normalized() * radius + origin;
  }

  _plane = CPlane::from_vertices(_vertices[0], _vertices[1], _vertices[2]);
}

INLINE PlaneClassification CPolygon::
classify_against_plane(const CPlane &plane) const {
  size_t front = 0;
  size_t back = 0;
  size_t onplane = 0;
  size_t count = _vertices.size();

  for (size_t i = 0; i < count; i++) {
    int test = plane.on_plane(_vertices[i]);
    if (test <= 0) {
      back++;
    }
    if (test >= 0) {
      front++;
    }
    if (test == 0) {
      onplane++;
    }
  }

  if (onplane == count) {
    return PlaneClassification::OnPlane;

  } else if (front == count) {
    return PlaneClassification::Front;

  } else if (back == count) {
    return PlaneClassification::Back;

  } else {
    return PlaneClassification::Spanning;
  }
}

INLINE bool CPolygon::
split(const CPlane &plane) {
  PT(CPolygon) front, back, cfront, cback;
  bool ret = split(plane, &front, &back, &cfront, &cback);
  if (ret) {
    _vertices = back->_vertices;
    _plane = back->_plane;
  }

  return ret;
}
