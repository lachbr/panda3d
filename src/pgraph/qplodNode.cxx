// Filename: qplodNode.cxx
// Created by:  drose (06Mar02)
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

#include "qplodNode.h"
#include "cullTraverserData.h"
#include "qpcullTraverser.h"

TypeHandle qpLODNode::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: qpLODNode::CData::make_copy
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
CycleData *qpLODNode::CData::
make_copy() const {
  return new CData(*this);
}

////////////////////////////////////////////////////////////////////
//     Function: qpLODNode::CData::write_datagram
//       Access: Public, Virtual
//  Description: Writes the contents of this object to the datagram
//               for shipping out to a Bam file.
////////////////////////////////////////////////////////////////////
void qpLODNode::CData::
write_datagram(BamWriter *manager, Datagram &dg) const {
  _lod.write_datagram(dg);
}

////////////////////////////////////////////////////////////////////
//     Function: qpLODNode::CData::fillin
//       Access: Public, Virtual
//  Description: This internal function is called by make_from_bam to
//               read in all of the relevant data from the BamFile for
//               the new qpLODNode.
////////////////////////////////////////////////////////////////////
void qpLODNode::CData::
fillin(DatagramIterator &scan, BamReader *manager) {
  _lod.read_datagram(scan);
}

////////////////////////////////////////////////////////////////////
//     Function: qpLODNode::make_copy
//       Access: Public, Virtual
//  Description: Returns a newly-allocated Node that is a shallow copy
//               of this one.  It will be a different Node pointer,
//               but its internal data may or may not be shared with
//               that of the original Node.
////////////////////////////////////////////////////////////////////
PandaNode *qpLODNode::
make_copy() const {
  return new qpLODNode(*this);
}

////////////////////////////////////////////////////////////////////
//     Function: qpLODNode::safe_to_combine
//       Access: Public, Virtual
//  Description: Returns true if it is generally safe to combine this
//               particular kind of PandaNode with other kinds of
//               PandaNodes, adding children or whatever.  For
//               instance, an LODNode should not be combined with any
//               other PandaNode, because its set of children is
//               meaningful.
////////////////////////////////////////////////////////////////////
bool qpLODNode::
safe_to_combine() const {
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: qpLODNode::xform
//       Access: Public, Virtual
//  Description: Transforms the contents of this PandaNode by the
//               indicated matrix, if it means anything to do so.  For
//               most kinds of PandaNodes, this does nothing.
////////////////////////////////////////////////////////////////////
void qpLODNode::
xform(const LMatrix4f &mat) {
  CDWriter cdata(_cycler);
  cdata->_lod.xform(mat);
}

////////////////////////////////////////////////////////////////////
//     Function: qpLODNode::has_cull_callback
//       Access: Public, Virtual
//  Description: Should be overridden by derived classes to return
//               true if cull_callback() has been defined.  Otherwise,
//               returns false to indicate cull_callback() does not
//               need to be called for this node during the cull
//               traversal.
////////////////////////////////////////////////////////////////////
bool qpLODNode::
has_cull_callback() const {
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: qpLODNode::cull_callback
//       Access: Public, Virtual
//  Description: If has_cull_callback() returns true, this function
//               will be called during the cull traversal to perform
//               any additional operations that should be performed at
//               cull time.  This may include additional manipulation
//               of render state or additional visible/invisible
//               decisions, or any other arbitrary operation.
//
//               By the time this function is called, the node has
//               already passed the bounding-volume test for the
//               viewing frustum, and the node's transform and state
//               have already been applied to the indicated
//               CullTraverserData object.
//
//               The return value is true if this node should be
//               visible, or false if it should be culled.
////////////////////////////////////////////////////////////////////
bool qpLODNode::
cull_callback(qpCullTraverser *trav, CullTraverserData &data) {
  if (data._net_transform->is_singular()) {
    // If we're under a singular transform, we can't compute the LOD;
    // select none of them instead.
    select_child(get_num_children());

  } else { 
    CDReader cdata(_cycler);
    LPoint3f camera_pos(0, 0, 0);

    // Get the LOD center in camera space
    CPT(TransformState) rel_transform =
      data._net_transform->invert_compose(trav->get_camera_transform());
    LPoint3f center = cdata->_lod._center * rel_transform->get_mat();
    
    // Determine which child to traverse
    int index = cdata->_lod.compute_child(camera_pos, center);
    select_child(index);
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: qpLODNode::output
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
void qpLODNode::
output(ostream &out) const {
  SelectiveChildNode::output(out);
  CDReader cdata(_cycler);
  out << " ";
  cdata->_lod.output(out);
}

////////////////////////////////////////////////////////////////////
//     Function: qpLODNode::register_with_read_factory
//       Access: Public, Static
//  Description: Tells the BamReader how to create objects of type
//               qpLODNode.
////////////////////////////////////////////////////////////////////
void qpLODNode::
register_with_read_factory() {
  BamReader::get_factory()->register_factory(get_class_type(), make_from_bam);
}

////////////////////////////////////////////////////////////////////
//     Function: qpLODNode::write_datagram
//       Access: Public, Virtual
//  Description: Writes the contents of this object to the datagram
//               for shipping out to a Bam file.
////////////////////////////////////////////////////////////////////
void qpLODNode::
write_datagram(BamWriter *manager, Datagram &dg) {
  SelectiveChildNode::write_datagram(manager, dg);
  manager->write_cdata(dg, _cycler);
}

////////////////////////////////////////////////////////////////////
//     Function: qpLODNode::make_from_bam
//       Access: Protected, Static
//  Description: This function is called by the BamReader's factory
//               when a new object of type qpLODNode is encountered
//               in the Bam file.  It should create the qpLODNode
//               and extract its information from the file.
////////////////////////////////////////////////////////////////////
TypedWritable *qpLODNode::
make_from_bam(const FactoryParams &params) {
  qpLODNode *node = new qpLODNode("");
  DatagramIterator scan;
  BamReader *manager;

  parse_params(params, scan, manager);
  node->fillin(scan, manager);

  return node;
}

////////////////////////////////////////////////////////////////////
//     Function: qpLODNode::fillin
//       Access: Protected
//  Description: This internal function is called by make_from_bam to
//               read in all of the relevant data from the BamFile for
//               the new qpLODNode.
////////////////////////////////////////////////////////////////////
void qpLODNode::
fillin(DatagramIterator &scan, BamReader *manager) {
  SelectiveChildNode::fillin(scan, manager);
  manager->read_cdata(scan, _cycler);
}
