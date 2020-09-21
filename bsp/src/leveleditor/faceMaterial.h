#pragma once

#include "config_leveleditor.h"
#include "dtoolbase.h"
#include "faceOrientation.h"
#include "luse.h"
#include "cPointCloud.h"

class CSolidFace;
class CMaterialReference;

class EXPCL_EDITOR CFaceMaterial {
PUBLISHED:
  CFaceMaterial();
  CFaceMaterial(const CFaceMaterial &) = default;
  CFaceMaterial(CFaceMaterial &&) = default;
  void operator =(const CFaceMaterial &);

  FaceOrientation set_v_axis_from_orientation(CSolidFace *face);
  void align_texture_to_face(CSolidFace *face);
  void align_texture_to_world(CSolidFace *face);
  void set_texture_rotation(PN_stdfloat angle);
  void fit_texture_to_point_cloud(const CPointCloud &cloud, int tile_x, int tile_y);
  void align_texture_with_point_cloud(const CPointCloud &cloud, Align mode);
  void minimize_texture_shift_values();

  CFaceMaterial clone();

  void set_scale(const LVector2 &scale);
  LVector2 get_scale() const;
  MAKE_PROPERTY(scale, get_scale, set_scale);

  void set_shift(const LVector2i &shift);
  LVector2i get_shift() const;
  MAKE_PROPERTY(shift, get_shift, set_shift);

  void set_u_axis(const LVector3 &axis);
  LVector3 get_u_axis() const;
  MAKE_PROPERTY(u_axis, get_u_axis, set_u_axis);

  void set_v_axis(const LVector3 &axis);
  LVector3 get_v_axis() const;
  MAKE_PROPERTY(v_axis, get_v_axis, set_v_axis);

  void set_rotation(PN_stdfloat rotation);
  PN_stdfloat get_rotation() const;
  MAKE_PROPERTY(rotation, get_rotation, set_rotation);

  void set_material(CMaterialReference *mat);
  CMaterialReference *get_material() const;
  MAKE_PROPERTY(material, get_material, set_material);

private:
  LVector2 _scale;
  LVector2i _shift;
  LVector3 _uaxis;
  LVector3 _vaxis;
  PN_stdfloat _rotation;
  CMaterialReference *_material;

  friend class CSolidFace;
};

INLINE void CFaceMaterial::
set_scale(const LVector2 &scale) {
  _scale = scale;
}

INLINE LVector2 CFaceMaterial::
get_scale() const {
  return _scale;
}

INLINE void CFaceMaterial::
set_shift(const LVector2i &shift) {
  _shift = shift;
}

INLINE LVector2i CFaceMaterial::
get_shift() const {
  return _shift;
}

INLINE void CFaceMaterial::
set_u_axis(const LVector3 &axis) {
  _uaxis = axis;
}

INLINE LVector3 CFaceMaterial::
get_u_axis() const {
  return _uaxis;
}

INLINE void CFaceMaterial::
set_v_axis(const LVector3 &axis) {
  _vaxis = axis;
}

INLINE LVector3 CFaceMaterial::
get_v_axis() const {
  return _vaxis;
}

INLINE void CFaceMaterial::
set_rotation(PN_stdfloat rotation) {
  _rotation = rotation;
}

INLINE PN_stdfloat CFaceMaterial::
get_rotation() const {
  return _rotation;
}

INLINE void CFaceMaterial::
set_material(CMaterialReference *mat) {
  _material = mat;
}

INLINE CMaterialReference *CFaceMaterial::
get_material() const {
  return _material;
}

INLINE void CFaceMaterial::
operator =(const CFaceMaterial &other) {
  _scale = other._scale;
  _shift = other._shift;
  _uaxis = other._uaxis;
  _vaxis = other._vaxis;
  _rotation = other._rotation;
  _material = other._material;
}

INLINE CFaceMaterial::
CFaceMaterial() {
  _material = nullptr;
  _scale = LVector2(1);
  _rotation = 0.0;
}

INLINE CFaceMaterial CFaceMaterial::
clone() {
  return CFaceMaterial(*this);
}
