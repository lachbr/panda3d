// Filename: loaderFileType.cxx
// Created by:  drose (20Jun00)
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

#include "loaderFileType.h"

TypeHandle LoaderFileType::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: LoaderFileType::Constructor
//       Access: Protected
//  Description:
////////////////////////////////////////////////////////////////////
LoaderFileType::
LoaderFileType() {
}

////////////////////////////////////////////////////////////////////
//     Function: LoaderFileType::Destructor
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
LoaderFileType::
~LoaderFileType() {
}

////////////////////////////////////////////////////////////////////
//     Function: LoaderFileType::load_file
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
PT(PandaNode) LoaderFileType::
load_file(const Filename &path, bool report_errors) const {
  loader_cat.error()
    << get_type() << " cannot read PandaNode objects.\n";
  return NULL;
}
