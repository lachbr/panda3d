// Filename: lwoSurfaceSmoothingAngle.cxx
// Created by:  drose (24Apr01)
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

#include "lwoSurfaceSmoothingAngle.h"
#include "lwoInputFile.h"

#include <indent.h>
#include <deg_2_rad.h>

TypeHandle LwoSurfaceSmoothingAngle::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: LwoSurfaceSmoothingAngle::read_iff
//       Access: Public, Virtual
//  Description: Reads the data of the chunk in from the given input
//               file, if possible.  The ID and length of the chunk
//               have already been read.  stop_at is the byte position
//               of the file to stop at (based on the current position
//               at in->get_bytes_read()).  Returns true on success,
//               false otherwise.
////////////////////////////////////////////////////////////////////
bool LwoSurfaceSmoothingAngle::
read_iff(IffInputFile *in, size_t stop_at) {
  LwoInputFile *lin = DCAST(LwoInputFile, in);

  _angle = lin->get_be_float32();

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: LwoSurfaceSmoothingAngle::write
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
void LwoSurfaceSmoothingAngle::
write(ostream &out, int indent_level) const {
  indent(out, indent_level)
    << get_id() << " { angle = " << rad_2_deg(_angle) << " degrees }\n";
}
