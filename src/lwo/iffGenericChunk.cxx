// Filename: iffGenericChunk.cxx
// Created by:  drose (23Apr01)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://www.panda3d.org/license.txt .
//
// To contact the maintainers of this program write to
// panda3d@yahoogroups.com .
//
////////////////////////////////////////////////////////////////////

#include "iffGenericChunk.h"
#include "iffInputFile.h"

#include <indent.h>

TypeHandle IffGenericChunk::_type_handle;


////////////////////////////////////////////////////////////////////
//     Function: IffGenericChunk::read_iff
//       Access: Public, Virtual
//  Description: Reads the data of the chunk in from the given input
//               file, if possible.  The ID and length of the chunk
//               have already been read.  stop_at is the byte position
//               of the file to stop at (based on the current position
//               at in->get_bytes_read()).  Returns true on success,
//               false otherwise.
////////////////////////////////////////////////////////////////////
bool IffGenericChunk::
read_iff(IffInputFile *in, size_t stop_at) {
  size_t length = stop_at - in->get_bytes_read();
  bool result = in->read_bytes(_data, length);
  in->align();
  return result;
}

////////////////////////////////////////////////////////////////////
//     Function: IffGenericChunk::write
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
void IffGenericChunk::
write(ostream &out, int indent_level) const {
  indent(out, indent_level)
    << get_id() << " { " << _data.get_length() << " bytes }\n";
}

