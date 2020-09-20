#pragma once

#include "config_editor.h"
#include "pvector.h"
#include "solid.h"

class EXPCL_EDITOR CSolidCollection {
PUBLISHED:
  CSolidCollection() = default;

  size_t get_num_solids() const;
  CSolid *get_solid(size_t n) const;
  void add_solid(CSolid *solid);
  void remove_solid(CSolid *solid);
  bool has_solid(CSolid *solid) const;

private:
  pvector<PT(CSolid)> _solids;
};

INLINE size_t CSolidCollection::
get_num_solids() const {
  return _solids.size();
}

INLINE CSolid *CSolidCollection::
get_solid(size_t n) const {
  return _solids[n];
}

INLINE void CSolidCollection::
add_solid(CSolid *solid) {
  _solids.push_back(solid);
}

INLINE void CSolidCollection::
remove_solid(CSolid *solid) {
  auto itr = std::find(_solids.begin(), _solids.end(), solid);
  if (itr != _solids.end()) {
    _solids.erase(itr);
  }
}

INLINE bool CSolidCollection::
has_solid(CSolid *solid) const {
  return std::find(_solids.begin(), _solids.end(), solid) != _solids.end();
}
