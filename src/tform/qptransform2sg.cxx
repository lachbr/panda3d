// Filename: qptransform2sg.cxx
// Created by:  drose (12Mar02)
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

#include "qptransform2sg.h"
#include "transformState.h"


TypeHandle qpTransform2SG::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: qpTransform2SG::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
qpTransform2SG::
qpTransform2SG(const string &name) :
  qpDataNode(name)
{
  _transform_input = define_input("Transform", EventStoreMat4::get_class_type());

  _node = NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: qpTransform2SG::set_node
//       Access: Public
//  Description: Sets the node that this node will adjust.
////////////////////////////////////////////////////////////////////
void qpTransform2SG::
set_node(PandaNode *node) {
  _node = node;
}

////////////////////////////////////////////////////////////////////
//     Function: qpTransform2SG::get_node
//       Access: Public
//  Description: Returns the node that this node will adjust, or NULL
//               if the node has not yet been set.
////////////////////////////////////////////////////////////////////
PandaNode *qpTransform2SG::
get_node() const {
  return _node;
}


////////////////////////////////////////////////////////////////////
//     Function: qpTransform2SG::do_transmit_data
//       Access: Protected, Virtual
//  Description: The virtual implementation of transmit_data().  This
//               function receives an array of input parameters and
//               should generate an array of output parameters.  The
//               input parameters may be accessed with the index
//               numbers returned by the define_input() calls that
//               were made earlier (presumably in the constructor);
//               likewise, the output parameters should be set with
//               the index numbers returned by the define_output()
//               calls.
////////////////////////////////////////////////////////////////////
void qpTransform2SG::
do_transmit_data(const DataNodeTransmit &input, DataNodeTransmit &) {
  if (input.has_data(_transform_input)) {
    const EventStoreMat4 *transform;
    DCAST_INTO_V(transform, input.get_data(_transform_input).get_ptr());
    const LMatrix4f &mat = transform->get_value();
    if (_node != (PandaNode *)NULL) {
      _node->set_transform(TransformState::make_mat(mat));
    }
  }
}
