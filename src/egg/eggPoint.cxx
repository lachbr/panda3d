// Filename: eggPoint.cxx
// Created by:  drose (15Dec99)
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

#include "eggPoint.h"

#include <indent.h>

#include <algorithm>

TypeHandle EggPoint::_type_handle;


////////////////////////////////////////////////////////////////////
//     Function: EggPoint::cleanup
//       Access: Public, Virtual
//  Description: Cleans up modeling errors in whatever context this
//               makes sense.  For instance, for a polygon, this calls
//               remove_doubled_verts(true).  For a point, it calls
//               remove_nonunique_verts().  Returns true if the
//               primitive is valid, or false if it is degenerate.
////////////////////////////////////////////////////////////////////
bool EggPoint::
cleanup() {
  remove_nonunique_verts();
  return !empty();
}

////////////////////////////////////////////////////////////////////
//     Function: EggPoint::write
//       Access: Public, Virtual
//  Description: Writes the point to the indicated output stream in
//               Egg format.
////////////////////////////////////////////////////////////////////
void EggPoint::
write(ostream &out, int indent_level) const {
  write_header(out, indent_level, "<PointLight>");
  write_body(out, indent_level+2);
  indent(out, indent_level) << "}\n";
}
