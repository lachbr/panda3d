#pragma once

#include "config_editor.h"
#include "referenceCount.h"
#include "bspMaterial.h"
#include "filename.h"
#include "luse.h"

class EXPCL_EDITOR CMaterialReference : public ReferenceCount {
PUBLISHED:
  CMaterialReference(const Filename &filename);

  const BSPMaterial *get_material() const;
  MAKE_PROPERTY(material, get_material);

  const Filename &get_filename() const;
  MAKE_PROPERTY(filename, get_filename);

  void set_size(const LVector2i &size);
  const LVector2i &get_size() const;
  int get_x_size() const;
  int get_y_size() const;
  MAKE_PROPERTY(size, get_size, set_size);

private:
  const BSPMaterial *_material;
  Filename _filename;
  LVector2i _size;
};

INLINE CMaterialReference::
CMaterialReference(const Filename &filename) {
  _filename = filename;
  _material = BSPMaterial::get_from_file(filename);
  _size = LVector2i(0, 0);
}

INLINE const BSPMaterial *CMaterialReference::
get_material() const {
  return _material;
}

INLINE const Filename &CMaterialReference::
get_filename() const {
  return _filename;
}

INLINE void CMaterialReference::
set_size(const LVector2i &size) {
  _size = size;
}

INLINE int CMaterialReference::
get_x_size() const {
  return _size[0];
}

INLINE int CMaterialReference::
get_y_size() const {
  return _size[1];
}

INLINE const LVector2i &CMaterialReference::
get_size() const {
  return _size;
}
