// Filename: geomVertexWriter.cxx
// Created by:  drose (25Mar05)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001 - 2004, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://etc.cmu.edu/panda3d/docs/license/ .
//
// To contact the maintainers of this program write to
// panda3d-general@lists.sourceforge.net .
//
////////////////////////////////////////////////////////////////////

#include "geomVertexWriter.h"


#ifndef NDEBUG
  // This is defined just for the benefit of having something non-NULL
  // to return from a nassertr() call.
unsigned char GeomVertexWriter::empty_buffer[100] = { 0 };
#endif

////////////////////////////////////////////////////////////////////
//     Function: GeomVertexWriter::set_column
//       Access: Published
//  Description: Sets up the writer to use the indicated column
//               description on the given array.
//
//               This also resets the current write row number to the
//               start row (the same value passed to a previous call
//               to set_row(), or 0 if set_row() was never called.)
//
//               The return value is true if the data type is valid,
//               false otherwise.
////////////////////////////////////////////////////////////////////
bool GeomVertexWriter::
set_column(int array, const GeomVertexColumn *column) {
  if (_vertex_data == (GeomVertexData *)NULL &&
      _array_data == (GeomVertexArrayData *)NULL) {
    return false;
  }

  if (column == (const GeomVertexColumn *)NULL) {
    // Clear the data type.
    _array = -1;
    _packer = NULL;
    _stride = 0;
    _pointer = NULL;
    _pointer_end = NULL;

    return false;
  }

  if (_vertex_data != (GeomVertexData *)NULL) {
#ifndef NDEBUG
    _array = -1;
    _packer = NULL;
    nassertr(array >= 0 && array < _vertex_data->get_num_arrays(), false);
#endif
    _array = array;
    const GeomVertexArrayData *array_data =_vertex_data->get_array(_array);
    _stride = array_data->get_array_format()->get_stride();

  } else {
    _stride = _array_data->get_array_format()->get_stride();
  }

  _packer = column->_packer;
  
  set_pointer(_start_row);
  
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomVertexWriter::output
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
void GeomVertexWriter::
output(ostream &out) const {
  const GeomVertexColumn *column = get_column();
  if (column == (GeomVertexColumn *)NULL) {
    out << "GeomVertexWriter()";
    
  } else {
    out << "GeomVertexWriter, array = " << get_array_data()
        << ", column = " << column->get_name()
        << " (" << get_packer()->get_name()
        << "), write row " << get_write_row();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GeomVertexWriter::initialize
//       Access: Private
//  Description: Called only by the constructor.
////////////////////////////////////////////////////////////////////
void GeomVertexWriter::
initialize() {
  _array = 0;
  _packer = NULL;
  _pointer_begin = NULL;
  _pointer_end = NULL;
  _pointer = NULL;
  _start_row = 0;
}
