#pragma once

#include "config_leveleditor.h"
#include "luse.h"

#include "planeClassification.h"
#include "cPlane.h"

class EXPCL_EDITOR CLine {
PUBLISHED:
  INLINE CLine(const LPoint3 &start, const LPoint3 &end);

  INLINE void reverse();
  INLINE void xform(const LMatrix4 &mat);
  INLINE bool equals(const CLine &other) const;
  INLINE bool almost_equal(const CLine &other, PN_stdfloat delta = 0.0001) const;
  INLINE PlaneClassification classify_against_plane(const CPlane &plane) const;
  INLINE LPoint3 closest_point(const LPoint3 &point) const;

private:
  LPoint3 _start;
  LPoint3 _end;
};

INLINE CLine::
CLine(const LPoint3 &start, const LPoint3 &end) {
  _start = start;
  _end = end;
}

INLINE void CLine::
reverse() {
  LPoint3 temp = _start;
  _start = _end;
  _end = temp;
}

INLINE void CLine::
xform(const LMatrix4 &mat) {
  _start = mat.xform_point(_start);
  _end = mat.xform_point(_end);
}

INLINE bool CLine::
equals(const CLine &other) const {
  return (_start == other._start && _end == other._end) ||
    (_start == other._end && _end == other._start);
}

INLINE bool CLine::
almost_equal(const CLine &other, PN_stdfloat delta) const {
  return (_start.almost_equal(other._start, delta) && _end.almost_equal(other._end, delta)) ||
    (_start.almost_equal(other._end, delta) && _end.almost_equal(other._start, delta));
}

INLINE PlaneClassification CLine::
classify_against_plane(const CPlane &plane) const {
  PN_stdfloat start = plane.dist_to_plane(_start);
  PN_stdfloat end = plane.dist_to_plane(_end);

  if (start == 0 && end == 0) {
    return PlaneClassification::OnPlane;

  } else if (start <= 0 && end <= 0) {
    return PlaneClassification::Back;

  } else if (start >= 0 && end >= 0) {
    return PlaneClassification::Front;

  } else {
    return PlaneClassification::Spanning;
  }
}

INLINE LPoint3 CLine::
closest_point(const LPoint3 &point) const {
  LVector3 delta = _end - _start;
  PN_stdfloat den = delta.length_squared();
  if (den == 0) {
    return _start; // start and end are the same
  }

  LVector3 num_point = point - _start;
  num_point.componentwise_mult(delta);
  PN_stdfloat num = num_point[0] + num_point[1] + num_point[2];
  PN_stdfloat u = num / den;

  if (u < 0) {
    return _start; // point is before the segment start

  } else if (u > 1) {
    return _end; // point is after the segment end

  } else {
    return _start + (delta * u);
  }
}
