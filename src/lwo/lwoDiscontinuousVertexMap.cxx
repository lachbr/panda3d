// Filename: lwoDiscontinuousVertexMap.cxx
// Created by:  drose (24Apr01)
// 
////////////////////////////////////////////////////////////////////

#include "lwoDiscontinuousVertexMap.h"
#include "lwoInputFile.h"

#include <indent.h>

TypeHandle LwoDiscontinuousVertexMap::_type_handle;


////////////////////////////////////////////////////////////////////
//     Function: LwoDiscontinuousVertexMap::has_value
//       Access: Public
//  Description: Returns true if the map has a value associated with
//               the given index, false otherwise.
////////////////////////////////////////////////////////////////////
bool LwoDiscontinuousVertexMap::
has_value(int polygon_index, int vertex_index) const {
  VMad::const_iterator di;
  di = _vmad.find(polygon_index);
  if (di != _vmad.end()) {
    const VMap &vmap = (*di).second;
    return (vmap.count(vertex_index) != 0);
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: LwoDiscontinuousVertexMap::get_value
//       Access: Public
//  Description: Returns the mapping value associated with the given
//               index, or an empty PTA_float if there is no mapping
//               value associated.
////////////////////////////////////////////////////////////////////
PTA_float LwoDiscontinuousVertexMap::
get_value(int polygon_index, int vertex_index) const {
  VMad::const_iterator di;
  di = _vmad.find(polygon_index);
  if (di != _vmad.end()) {
    const VMap &vmap = (*di).second;
    VMap::const_iterator vi;
    vi = vmap.find(vertex_index);
    if (vi != vmap.end()) {
      return (*vi).second;
    }
  }

  return PTA_float();
}

////////////////////////////////////////////////////////////////////
//     Function: LwoDiscontinuousVertexMap::read_iff
//       Access: Public, Virtual
//  Description: Reads the data of the chunk in from the given input
//               file, if possible.  The ID and length of the chunk
//               have already been read.  stop_at is the byte position
//               of the file to stop at (based on the current position
//               at in->get_bytes_read()).  Returns true on success,
//               false otherwise.
////////////////////////////////////////////////////////////////////
bool LwoDiscontinuousVertexMap::
read_iff(IffInputFile *in, size_t stop_at) {
  LwoInputFile *lin = DCAST(LwoInputFile, in);

  _map_type = lin->get_id();
  _dimension = lin->get_be_uint16();
  _name = lin->get_string();

  while (lin->get_bytes_read() < stop_at && !lin->is_eof()) {
    int vertex_index = lin->get_vx();
    int polygon_index = lin->get_vx();

    PTA_float value;
    for (int i = 0; i < _dimension; i++) {
      value.push_back(lin->get_be_float32());
    }

    bool inserted = _vmad[polygon_index].insert(VMap::value_type(vertex_index, value)).second;
    if (!inserted) {
      nout << "Duplicate pair " << polygon_index << ", " << vertex_index
	   << " in map.\n";
    }
  }

  return (lin->get_bytes_read() == stop_at);
}

////////////////////////////////////////////////////////////////////
//     Function: LwoDiscontinuousVertexMap::write
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void LwoDiscontinuousVertexMap::
write(ostream &out, int indent_level) const {
  indent(out, indent_level)
    << get_id() << " { map_type = " << _map_type 
    << ", dimension = " << _dimension
    << ", name = \"" << _name << "\", "
    << _vmad.size() << " polygons }\n";
}
