// Filename: eggMatrixTablePointer.cxx
// Created by:  drose (26Feb01)
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

#include "eggMatrixTablePointer.h"

#include "dcast.h"
#include "eggXfmAnimData.h"
#include "eggXfmSAnim.h"

TypeHandle EggMatrixTablePointer::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: EggMatrixTablePointer::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
EggMatrixTablePointer::
EggMatrixTablePointer(EggObject *object) {
  _table = DCAST(EggTable, object);

  if (_table != (EggTable *)NULL) {
    // Now search for the child named "xform".  This contains the
    // actual table data.
    EggGroupNode::iterator ci;
    bool found = false;
    for (ci = _table->begin(); ci != _table->end() && !found; ++ci) {
      EggNode *child = (*ci);
      if (child->get_name() == "xform") {
        if (child->is_of_type(EggXfmSAnim::get_class_type())) {
          _xform = DCAST(EggXfmSAnim, child);
          _xform->normalize();
          found = true;

        } else if (child->is_of_type(EggXfmAnimData::get_class_type())) {
          // Quietly replace old-style XfmAnim tables with new-style
          // XfmSAnim tables.
          PT(EggXfmAnimData) anim = DCAST(EggXfmAnimData, child);
          _xform = new EggXfmSAnim(*anim);
          _table->replace(ci, _xform.p());
          found = true;
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: EggMatrixTablePointer::get_num_frames
//       Access: Public, Virtual
//  Description: Returns the number of frames of animation for this
//               particular joint.
////////////////////////////////////////////////////////////////////
int EggMatrixTablePointer::
get_num_frames() const {
  if (_xform == (EggXfmSAnim *)NULL) {
    return 0;
  } else {
    return _xform->get_num_rows();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: EggMatrixTablePointer::get_frame
//       Access: Public, Virtual
//  Description: Returns the transform matrix corresponding to this
//               joint position in the nth frame.
////////////////////////////////////////////////////////////////////
LMatrix4d EggMatrixTablePointer::
get_frame(int n) const {
  if (get_num_frames() == 1) {
    // If we have exactly one frame, then we have as many frames as we
    // want; just repeat the first frame.
    n = 0;
  }

  nassertr(n >= 0 && n < get_num_frames(), LMatrix4d::ident_mat());
  LMatrix4d mat;
  _xform->get_value(n, mat);
  return mat;
}

////////////////////////////////////////////////////////////////////
//     Function: EggMatrixTablePointer::set_frame
//       Access: Public, Virtual
//  Description: Sets the transform matrix corresponding to this
//               joint position in the nth frame.
////////////////////////////////////////////////////////////////////
void EggMatrixTablePointer::
set_frame(int n, const LMatrix4d &mat) {
  nassertv(n >= 0 && n < get_num_frames());
  _xform->set_value(n, mat);
}

////////////////////////////////////////////////////////////////////
//     Function: EggMatrixTablePointer::add_frame
//       Access: Public, Virtual
//  Description: Appends a new frame onto the end of the data, if
//               possible; returns true if not possible, or false
//               otherwise (e.g. for a static joint).
////////////////////////////////////////////////////////////////////
bool EggMatrixTablePointer::
add_frame(const LMatrix4d &mat) {
  if (_xform == (EggXfmSAnim *)NULL) {
    return false;
  }

  return _xform->add_data(mat);
}

////////////////////////////////////////////////////////////////////
//     Function: EggMatrixTablePointer::do_rebuild
//       Access: Public, Virtual
//  Description: Rebuilds the entire table all at once, based on the
//               frames added by repeated calls to add_rebuild_frame()
//               since the last call to begin_rebuild().
//
//               Until do_rebuild() is called, the animation table is
//               not changed.
//
//               The return value is true if all frames are
//               acceptable, or false if there is some problem.
////////////////////////////////////////////////////////////////////
bool EggMatrixTablePointer::
do_rebuild() {
  if (_rebuild_frames.empty()) {
    return true;
  }

  if (_xform == (EggXfmSAnim *)NULL) {
    return false;
  }

  _xform->clear_data();
  RebuildFrames::const_iterator fi;
  for (fi = _rebuild_frames.begin(); fi != _rebuild_frames.end(); ++fi) {
    if (!_xform->add_data(*fi)) {
      return false;
    }
  }

  _rebuild_frames.clear();
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: EggMatrixTablePointer::optimize
//       Access: Public, Virtual
//  Description: Resets the table before writing to disk so that
//               redundant rows (e.g. i { 1 1 1 1 1 1 1 1 }) are
//               collapsed out.
////////////////////////////////////////////////////////////////////
void EggMatrixTablePointer::
optimize() {
  if (_xform != (EggXfmSAnim *)NULL) {
    _xform->optimize();
  }
}
