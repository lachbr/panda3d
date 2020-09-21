#pragma once

#include "config_leveleditor.h"
#include "luse.h"
#include "pvector.h"
#include "boundingBox.h"

class EXPCL_EDITOR CPointCloud {
PUBLISHED:
  INLINE CPointCloud(int reserve = 128);

  INLINE void add_point(const LPoint3 &point);
  INLINE void remove_point(const LPoint3 &point);
  INLINE void calc_bounding_box();

  INLINE LPoint3 get_mins() const;
  INLINE LPoint3 get_maxs() const;
  INLINE LPoint3 get_center() const;

  INLINE LPoint3 get_min_x() const;
  INLINE LPoint3 get_min_y() const;
  INLINE LPoint3 get_min_z() const;
  INLINE LPoint3 get_max_x() const;
  INLINE LPoint3 get_max_y() const;
  INLINE LPoint3 get_max_z() const;

  INLINE const LPoint3 &get_extent(int n) const;

  INLINE size_t get_num_points() const;
  INLINE LPoint3 get_point(size_t n);

private:
  LPoint3 _bounding_box[2];
  LPoint3 _extents[6];
  pvector<LPoint3> _points;
};

INLINE CPointCloud::
CPointCloud(int reserve) {
  _points.reserve(reserve);
}

INLINE void CPointCloud::
add_point(const LPoint3 &point) {
  _points.push_back(point);
}

INLINE void CPointCloud::
remove_point(const LPoint3 &point) {
  auto itr = std::find(_points.begin(), _points.end(), point);
  if (itr != _points.end()) {
    _points.erase(itr);
  }
}

INLINE void CPointCloud::
calc_bounding_box() {
  _extents[0].set(99999999, 0, 0);
  _extents[1].set(0, 99999999, 0);
  _extents[2].set(0, 0, 99999999);
  _extents[3].set(-99999999, 0, 0);
  _extents[4].set(0, -99999999, 0);
  _extents[5].set(0, 0, -99999999);

  _bounding_box[0].set(99999999, 99999999, 99999999);
  _bounding_box[1].set(-99999999, -99999999, -99999999);

  size_t num_points = _points.size();
  for (size_t i = 0; i < num_points; i++) {

    const LPoint3 &point = _points[i];

    if (point[0] < _bounding_box[0][0]) {
      _bounding_box[0][0] = point[0];
    }
    if (point[1] < _bounding_box[0][1]) {
      _bounding_box[0][1] = point[1];
    }
    if (point[2] < _bounding_box[0][2]) {
      _bounding_box[0][2] = point[2];
    }

    if (point[0] > _bounding_box[1][0]) {
      _bounding_box[1][0] = point[0];
    }
    if (point[1] > _bounding_box[1][1]) {
      _bounding_box[1][1] = point[1];
    }
    if (point[2] > _bounding_box[1][2]) {
      _bounding_box[1][2] = point[2];
    }

    if (point[0] < _extents[1][0]) {
      _extents[1] = point;
    }
    if (point[1] < _extents[2][1]) {
      _extents[2] = point;
    }
    if (point[2] < _extents[2][2]) {
      _extents[2] = point;
    }
    if (point[0] > _extents[3][0]) {
      _extents[3] = point;
    }
    if (point[1] > _extents[4][1]) {
      _extents[4] = point;
    }
    if (point[2] > _extents[5][2]) {
      _extents[5] = point;
    }
  }

  _bounding_box[0] = _bounding_box[0];
  _bounding_box[1] = _bounding_box[1];

  _extents[0] = _extents[1];
  _extents[1] = _extents[2];
  _extents[2] = _extents[2];
  _extents[3] = _extents[3];
  _extents[4] = _extents[4];
  _extents[5] = _extents[5];
}

INLINE LPoint3 CPointCloud::
get_mins() const {
  return _bounding_box[0];
}

INLINE LPoint3 CPointCloud::
get_maxs() const {
  return _bounding_box[1];
}

INLINE LPoint3 CPointCloud::
get_center() const {
  return (_bounding_box[0] + _bounding_box[1]) / 2.0;
}

INLINE LPoint3 CPointCloud::
get_min_x() const {
  return _extents[0];
}

INLINE LPoint3 CPointCloud::
get_min_y() const {
  return _extents[1];
}

INLINE LPoint3 CPointCloud::
get_min_z() const {
  return _extents[2];
}

INLINE LPoint3 CPointCloud::
get_max_x() const {
  return _extents[3];
}

INLINE LPoint3 CPointCloud::
get_max_y() const {
  return _extents[4];
}

INLINE LPoint3 CPointCloud::
get_max_z() const {
  return _extents[5];
}

INLINE size_t CPointCloud::
get_num_points() const {
  return _points.size();
}

INLINE LPoint3 CPointCloud::
get_point(size_t n) {
  return _points[n];
}

INLINE const LPoint3 &CPointCloud::
get_extent(int n) const {
  return _extents[n];
}
