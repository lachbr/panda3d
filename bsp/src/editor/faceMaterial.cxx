#include "faceMaterial.h"
#include "solidFace.h"
#include "deg_2_rad.h"
#include "materialReference.h"

FaceOrientation CFaceMaterial::
set_v_axis_from_orientation(CSolidFace *face) {
  _uaxis = LVector3(0);
  _vaxis = LVector3(0);

  // Determine the genral orientation of this face(floor, ceiling, n wall, etc).
  FaceOrientation orientation = face->get_orientation();
  if (orientation == FaceOrientation::Invalid) {
    return orientation;
  }

  // Pick a world axis aligned V axis based on the face orientation.
  _vaxis = DownVectors[(int)orientation];

  return orientation;
}

void CFaceMaterial::
align_texture_to_face(CSolidFace *face) {
  // Set the U and V axes to match the plane's normal.
  // Need to start with the world alignment on the V axis so that we don't
  // align backwards.
  // Then we can calculate U based on that, and the real V afterwards

  FaceOrientation orientation = set_v_axis_from_orientation(face);
  if (orientation == FaceOrientation::Invalid) {
    return;
  }

  CPlane plane = face->get_world_plane();
  LNormal normal = plane.get_normal();

  //
  // Calculate the texture axes.
  //

  // Using the axis-aligned V axis, calculate the true U axis
  _uaxis = normal.cross(_vaxis).normalized();

  // Now use the true U axis to calculate the true V axis.
  _vaxis = _uaxis.cross(normal).normalized();

  _rotation = 0.0;
}

void CFaceMaterial::
align_texture_to_world(CSolidFace *face) {
  // Set the U and V axes to match the X, Y, or Z axes.
  // How they are calculated depends on which direction the plane is facing.

  FaceOrientation orientation = set_v_axis_from_orientation(face);
  if (orientation == FaceOrientation::Invalid) {
    return;
  }

  _uaxis = RightVectors[(int)orientation];
  _rotation = 0.0;
}

void CFaceMaterial::
set_texture_rotation(PN_stdfloat angle) {
  // Rotate texture around face normal
  PN_stdfloat rads = deg_2_rad(angle);
  LVector3 texnorm = _vaxis.cross(_uaxis).normalized();
  LQuaternion transform;
  transform.set_from_axis_angle_rad(rads, texnorm);
  _uaxis = transform.xform(_uaxis);
  _vaxis = transform.xform(_vaxis);
  _rotation = angle;
}

void CFaceMaterial::
fit_texture_to_point_cloud(const CPointCloud &cloud, int tile_x, int tile_y) {
  if (!_material) {
    return;
  }

  if (tile_x <= 0) {
    tile_x = 1;
  }
  if (tile_y <= 0) {
    tile_y = 1;
  }

  // Scale will change, no need to use it in the calculations
  vector_float xvals;
  vector_float yvals;
  for (int i = 0; i < 6; i++) {
    const LPoint3 &extent = cloud.get_extent(i);
    xvals.push_back(extent.dot(_uaxis));
    yvals.push_back(extent.dot(_vaxis));
  }

  auto minmax_u = std::minmax_element(xvals.begin(), xvals.end());
  PN_stdfloat min_u = *minmax_u.first;
  PN_stdfloat max_u = *minmax_u.second;

  auto minmax_v = std::minmax_element(yvals.begin(), yvals.end());
  PN_stdfloat min_v = *minmax_v.first;
  PN_stdfloat max_v = *minmax_v.second;

  _scale[0] = (max_u - min_u) / (_material->get_x_size() * tile_x);
  _scale[1] = (max_v - min_v) / (_material->get_y_size() * tile_y);
  _shift[0] = -min_u / _scale[0];
  _shift[1] = -min_v / _scale[1];
}

void CFaceMaterial::
align_texture_with_point_cloud(const CPointCloud &cloud, Align mode) {
  if (!_material) {
    return;
  }

  vector_float xvals;
  vector_float yvals;
  for (int i = 0; i < 6; i++) {
    const LPoint3 &extent = cloud.get_extent(i);
    xvals.push_back(extent.dot(_uaxis) / _scale[0]);
    yvals.push_back(extent.dot(_vaxis) / _scale[1]);
  }

  auto minmax_u = std::minmax_element(xvals.begin(), xvals.end());
  PN_stdfloat min_u = *minmax_u.first;
  PN_stdfloat max_u = *minmax_u.second;

  auto minmax_v = std::minmax_element(yvals.begin(), yvals.end());
  PN_stdfloat min_v = *minmax_v.first;
  PN_stdfloat max_v = *minmax_v.second;

  switch (mode) {
  case Align::Left:
    _shift[0] = -min_u;
    break;
  case Align::Right:
    _shift[0] = -max_u + _material->get_x_size();
    break;
  case Align::Center:
    {
      PN_stdfloat avg_u = (min_u + max_u) / 2.0;
      PN_stdfloat avg_v = (min_v + max_v) / 2.0;
      _shift[0] = -avg_u + _material->get_x_size() / 2.0;
      _shift[1] = -avg_v + _material->get_y_size() / 2.0;
    }
    break;
  case Align::Top:
    _shift[1] = -min_v;
    break;
  case Align::Bottom:
    _shift[1] = -max_v + _material->get_y_size();
    break;
  default:
    break;
  }
}

void CFaceMaterial::
minimize_texture_shift_values() {
  if (!_material) {
    return;
  }

  // Keep the shift values to a minimum
  _shift[0] %= _material->get_x_size();
  _shift[1] %= _material->get_y_size();

  if (_shift[0] < -_material->get_x_size() / 2) {
    _shift[0] += _material->get_x_size();
  }

  if (_shift[1] < -_material->get_y_size() / 2) {
    _shift[1] += _material->get_y_size();
  }
}
