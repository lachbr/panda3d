#include "dynamic_vertex_buffer.h"
#include "geomEnums.h"

DynamicVertexBuffer::
DynamicVertexBuffer(const GeomVertexFormat *format, int num_vertices) :
  _vdata(new GeomVertexData("dynamic", format, GeomEnums::UH_dynamic)),
  _position(0),
  _locked_num_verts(0),
  _max_vertices(num_vertices) {
  
  _vdata->set_num_rows(_max_vertices);
}

GeomVertexWriter DynamicVertexBuffer::
get_writer(const std::string &name) {
  GeomVertexWriter writer(_vdata, name);
  writer.set_row(_position);
  return writer;
}

bool DynamicVertexBuffer::
lock(int num_vertices, int &first_index) {
  if (_position + num_vertices > _max_vertices) {
    return false; // can't write more vertices than we have allocated
  }
  _locked_num_verts = num_vertices;
  first_index = _position;
  return true;
}

void DynamicVertexBuffer::
unlock() {
  _position += _locked_num_verts;
  _locked_num_verts = 0;
}