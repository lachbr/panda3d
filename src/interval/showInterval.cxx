// Filename: showInterval.cxx
// Created by:  drose (27Aug02)
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

#include "showInterval.h"

int ShowInterval::_unique_index;
TypeHandle ShowInterval::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: ShowInterval::Constructor
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
ShowInterval::
ShowInterval(const NodePath &node, const string &name) :
  CInterval(name, 0.0, true),
  _node(node)
{
  nassertv(!node.is_empty());
  if (_name.empty()) {
    ostringstream name_strm;
    name_strm
      << "ShowInterval-" << node.node()->get_name() << "-" << ++_unique_index;
    _name = name_strm.str();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: ShowInterval::instant
//       Access: Published, Virtual
//  Description: This is called in lieu of priv_initialize() .. priv_step()
//               .. priv_finalize(), when everything is to happen within
//               one frame.  The interval should initialize itself,
//               then leave itself in the final state.
////////////////////////////////////////////////////////////////////
void ShowInterval::
priv_instant() {
  check_stopped("instant");
  _node.show();
  _state = S_final;
}

////////////////////////////////////////////////////////////////////
//     Function: ShowInterval::reverse_instant
//       Access: Published, Virtual
//  Description: This is called in lieu of priv_reverse_initialize()
//               .. priv_step() .. priv_reverse_finalize(), when everything is
//               to happen within one frame.  The interval should
//               initialize itself, then leave itself in the initial
//               state.
////////////////////////////////////////////////////////////////////
void ShowInterval::
priv_reverse_instant() {
  check_stopped("instant");
  _node.hide();
  _state = S_initial;
}
