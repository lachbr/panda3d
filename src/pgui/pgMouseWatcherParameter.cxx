// Filename: pgMouseWatcherParameter.cxx
// Created by:  drose (05Jul01)
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

#include "pgMouseWatcherParameter.h"

TypeHandle PGMouseWatcherParameter::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: PGMouseWatcherParameter::Destructor
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
PGMouseWatcherParameter::
~PGMouseWatcherParameter() {
}

////////////////////////////////////////////////////////////////////
//     Function: PGMouseWatcherParameter::output
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
void PGMouseWatcherParameter::
output(ostream &out) const {
  MouseWatcherParameter::output(out);
}
