#pragma once

#include "config_editor.h"
#include "luse.h"

class EXPCL_EDITOR CRay {
PUBLISHED:
  CRay(const LPoint3 &origin, const LVector3 &direction);

  INLINE void set_origin(const LPoint3 &origin);
  INLINE LPoint3 get_origin() const;
  MAKE_PROPERTY(origin, get_origin, set_origin);

  INLINE void set_direction(const LVector3 &direction);
  INLINE LVector3 get_direction() const;
  MAKE_PROPERTY(direction, get_direction, set_direction);

  INLINE void set_t(PN_stdfloat t);
  INLINE PN_stdfloat get_t() const;
  MAKE_PROPERTY(t, get_t, set_t);

  INLINE void xform(const LMatrix4 &mat);

private:
  LPoint3 _origin;
  LVector3 _direction;
  PN_stdfloat _t;
};

INLINE CRay::
CRay(const LPoint3 &origin, const LVector3 &direction) {
  _origin = origin;
  _direction = direction;
  _t = 0.0;
}

INLINE void CRay::
set_origin(const LPoint3 &origin) {
  _origin = origin;
}

INLINE LPoint3 CRay::
get_origin() const {
  return _origin;
}

INLINE void CRay::
set_direction(const LVector3 &dir) {
  _direction = dir;
}

INLINE LVector3 CRay::
get_direction() const {
  return _direction;
}

INLINE void CRay::
set_t(PN_stdfloat t) {
  _t = t;
}

INLINE PN_stdfloat CRay::
get_t() const {
  return _t;
}

INLINE void CRay::
xform(const LMatrix4 &mat) {
  _origin = mat.xform_point(_origin);
  _direction = mat.xform_vec(_direction);
}
