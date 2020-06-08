#pragma once

#include "config_bsp.h"

#include "geomVertexData.h"
#include "geomVertexFormat.h"
#include "geomVertexWriter.h"

class EXPCL_PANDABSP DynamicVertexBuffer : public ReferenceCount {

PUBLISHED:
  DynamicVertexBuffer(const GeomVertexFormat *format, int num_vertices);

  bool lock(int num_vertices, int &first_index);
  void unlock();

  GeomVertexWriter get_writer(const std::string &column_name);

  GeomVertexData *get_vdata() const;

  int get_max_vertices() const;
  int get_position() const;

  void reset();

private:
  int _position;
  int _max_vertices;
  int _locked_num_verts;
  PT(GeomVertexData) _vdata;
};

INLINE void DynamicVertexBuffer::
reset() {
  _position = 0;
  _locked_num_verts = 0;
}

INLINE GeomVertexData *DynamicVertexBuffer::
get_vdata() const {
  return _vdata;
}

INLINE int DynamicVertexBuffer::
get_position() const {
  return _position;
}

INLINE int DynamicVertexBuffer::
get_max_vertices() const {
  return _max_vertices;
}