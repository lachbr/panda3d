// Filename: loaderFileTypeBam.cxx
// Created by:  jason (21Jun00)
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

#include "loaderFileTypeBam.h"
#include "config_pgraph.h"
#include "bamFile.h"
#include "loaderOptions.h"

#include "dcast.h"

TypeHandle LoaderFileTypeBam::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: LoaderFileTypeBam::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
LoaderFileTypeBam::
LoaderFileTypeBam() {
}

////////////////////////////////////////////////////////////////////
//     Function: LoaderFileTypeBam::get_name
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
string LoaderFileTypeBam::
get_name() const {
  return "Bam";
}

////////////////////////////////////////////////////////////////////
//     Function: LoaderFileTypeBam::get_extension
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
string LoaderFileTypeBam::
get_extension() const {
  return "bam";
}

////////////////////////////////////////////////////////////////////
//     Function: LoaderFileTypeBam::supports_compressed
//       Access: Published, Virtual
//  Description: Returns true if this file type can transparently load
//               compressed files (with a .pz extension), false
//               otherwise.
////////////////////////////////////////////////////////////////////
bool LoaderFileTypeBam::
supports_compressed() const {
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: LoaderFileTypeBam::load_file
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
PT(PandaNode) LoaderFileTypeBam::
load_file(const Filename &path, const LoaderOptions &options) const {
  bool report_errors = (options.get_flags() & LoaderOptions::LF_report_errors) != 0;
  BamFile bam_file;
  if (!bam_file.open_read(path, report_errors)) {
    return NULL;
  }

  return bam_file.read_node(report_errors);
}

