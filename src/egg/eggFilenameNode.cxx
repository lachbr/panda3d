// Filename: eggFilenameNode.cxx
// Created by:  drose (11Feb99)
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

#include "eggFilenameNode.h"

TypeHandle EggFilenameNode::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: EggFilenameNode::get_default_extension
//       Access: Public, Virtual
//  Description: Returns the default extension for this filename type.
////////////////////////////////////////////////////////////////////
string EggFilenameNode::
get_default_extension() const {
  return string();
}
