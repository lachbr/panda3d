// Filename: qpnodePath.cxx
// Created by:  drose (25Feb02)
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

#include "qpnodePath.h"
#include "qpnodePathCollection.h"
#include "qpfindApproxPath.h"
#include "qpfindApproxLevelEntry.h"
#include "qpfindApproxLevel.h"
#include "config_pgraph.h"
#include "colorAttrib.h"
#include "colorScaleAttrib.h"
#include "cullBinAttrib.h"
#include "textureAttrib.h"
#include "materialAttrib.h"
#include "fogAttrib.h"
#include "renderModeAttrib.h"
#include "cullFaceAttrib.h"
#include "depthTestAttrib.h"
#include "depthWriteAttrib.h"
#include "billboardEffect.h"
#include "transparencyAttrib.h"
#include "materialPool.h"
#include "look_at.h"
#include "compose_matrix.h"
#include "plist.h"
#include "boundingSphere.h"
#include "qpgeomNode.h"
#include "qpsceneGraphReducer.h"
#include "textureCollection.h"
#include "globPattern.h"
#include "config_gobj.h"

// stack seems to overflow on Intel C++ at 7000.  If we need more than 
// 7000, need to increase stack size.
int qpNodePath::_max_search_depth = 7000; 
TypeHandle qpNodePath::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_num_nodes
//       Access: Published
//  Description: Returns the number of nodes in the path.
////////////////////////////////////////////////////////////////////
int qpNodePath::
get_num_nodes() const {
  if (is_empty()) {
    return 0;
  }
  uncollapse_head();
  return _head->get_length();
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_node
//       Access: Published
//  Description: Returns the nth node of the path, where 0 is the
//               referenced (bottom) node and get_num_nodes() - 1 is
//               the top node.  This requires iterating through the
//               path.
////////////////////////////////////////////////////////////////////
PandaNode *qpNodePath::
get_node(int index) const {
  nassertr(index >= 0 && index < get_num_nodes(), NULL);

  uncollapse_head();
  qpNodePathComponent *comp = _head;
  while (index > 0) {
    // If this assertion fails, the index was out of range; the
    // component's length must have been invalid.
    nassertr(comp != (qpNodePathComponent *)NULL, NULL);
    comp = comp->get_next();
    index--;
  }

  // If this assertion fails, the index was out of range; the
  // component's length must have been invalid.
  nassertr(comp != (qpNodePathComponent *)NULL, NULL);
  return comp->get_node();
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_top_node
//       Access: Published
//  Description: Returns the top node of the path, or NULL if the path
//               is empty.  This requires iterating through the path.
////////////////////////////////////////////////////////////////////
PandaNode *qpNodePath::
get_top_node() const {
  if (is_empty()) {
    return (PandaNode *)NULL;
  }

  uncollapse_head();
  qpNodePathComponent *comp = _head;
  while (!comp->is_top_node()) {
    comp = comp->get_next();
    nassertr(comp != (qpNodePathComponent *)NULL, NULL);
  }

  return comp->get_node();
}


////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_children
//       Access: Published
//  Description: Returns the set of all child nodes of the referenced
//               node.
////////////////////////////////////////////////////////////////////
qpNodePathCollection qpNodePath::
get_children() const {
  qpNodePathCollection result;
  nassertr_always(!is_empty(), result);

  PandaNode *bottom_node = node();

  PandaNode::Children cr = bottom_node->get_children();
  int num_children = cr.get_num_children();
  for (int i = 0; i < num_children; i++) {
    qpNodePath child;
    child._head = PandaNode::get_component(_head, cr.get_child(i));
    result.add_path(child);
  }

  return result;
}

////////////////////////////////////////////////////////////////////
//     Function: qoNodePath::find
//       Access: Published
//  Description: Searches for a node below the referenced node that
//               matches the indicated string.  Returns the shortest
//               match found, if any, or an empty NodePath if no match
//               can be found.
////////////////////////////////////////////////////////////////////
qpNodePath qpNodePath::
find(const string &path) const {
  nassertr_always(!is_empty(), fail());

  qpNodePathCollection col;
  find_matches(col, path, 1);

  if (col.is_empty()) {
    return qpNodePath::not_found();
  }

  return col.get_path(0);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::find_all_matches
//       Access: Published
//  Description: Returns the complete set of all NodePaths that begin
//               with this NodePath and can be extended by
//               path.  The shortest paths will be listed
//               first.
////////////////////////////////////////////////////////////////////
qpNodePathCollection qpNodePath::
find_all_matches(const string &path) const {
  qpNodePathCollection col;
  nassertr_always(!is_empty(), col);
  nassertr(verify_complete(), col);
  find_matches(col, path, -1);
  return col;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::find_all_paths_to
//       Access: Published
//  Description: Returns the set of all NodePaths that extend from
//               this NodePath down to the indicated node.  The
//               shortest paths will be listed first.
////////////////////////////////////////////////////////////////////
qpNodePathCollection qpNodePath::
find_all_paths_to(PandaNode *node) const {
  qpNodePathCollection col;
  nassertr_always(!is_empty(), col);
  nassertr(verify_complete(), col);
  nassertr(node != (PandaNode *)NULL, col);
  qpFindApproxPath approx_path;
  approx_path.add_match_many(0);
  approx_path.add_match_pointer(node, 0);
  find_matches(col, approx_path, -1);
  return col;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::reparent_to
//       Access: Published
//  Description: Removes the referenced node of the qpNodePath from its
//               current parent and attaches it to the referenced node of
//               the indicated qpNodePath.
////////////////////////////////////////////////////////////////////
void qpNodePath::
reparent_to(const qpNodePath &other, int sort) {
  nassertv(other.verify_complete());
  nassertv_always(!is_empty());
  nassertv_always(!other.is_empty());

  uncollapse_head();
  other.uncollapse_head();
  PandaNode::reparent(other._head, _head, sort);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::wrt_reparent_to
//       Access: Published
//  Description: This functions identically to reparent_to(), except
//               the transform on this node is also adjusted so that
//               the node remains in the same place in world
//               coordinates, even if it is reparented into a
//               different coordinate system.
////////////////////////////////////////////////////////////////////
void qpNodePath::
wrt_reparent_to(const qpNodePath &other, int sort) {
  nassertv(other.verify_complete());
  nassertv_always(!is_empty());
  nassertv_always(!other.is_empty());

  set_transform(get_transform(other));
  reparent_to(other, sort);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::instance_to
//       Access: Published
//  Description: Adds the referenced node of the NodePath as a child
//               of the referenced node of the indicated other
//               NodePath.  Any other parent-child relations of the
//               node are unchanged; in particular, the node is not
//               removed from its existing parent, if any.
//
//               If the node already had an existing parent, this
//               method will create a new instance of the node within
//               the scene graph.
//
//               This does not change the NodePath itself, but does
//               return a new NodePath that reflects the new instance
//               node.
////////////////////////////////////////////////////////////////////
qpNodePath qpNodePath::
instance_to(const qpNodePath &other, int sort) const {
  nassertr(verify_complete(), qpNodePath::fail());
  nassertr_always(!is_empty(), qpNodePath::fail());
  nassertr(!other.is_empty(), qpNodePath::fail());

  uncollapse_head();
  other.uncollapse_head();

  qpNodePath new_instance;
  new_instance._head = PandaNode::attach(other._head, node(), sort);

  return new_instance;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::instance_under_node
//       Access: Published
//  Description: Behaves like instance_to(), but implicitly creates a
//               new node to instance the geometry under, and returns a
//               NodePath to that new node.  This allows the
//               programmer to set a unique state and/or transform on
//               this instance.
////////////////////////////////////////////////////////////////////
qpNodePath qpNodePath::
instance_under_node(const qpNodePath &other, const string &name, int sort) const {
  qpNodePath new_node = other.attach_new_node(name, sort);
  instance_to(new_node);
  return new_node;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::copy_to
//       Access: Published
//  Description: Functions exactly like instance_to(), except a deep
//               copy is made of the referenced node and all of its
//               descendents, which is then parented to the indicated
//               node.  A qpNodePath to the newly created copy is
//               returned.
////////////////////////////////////////////////////////////////////
qpNodePath qpNodePath::
copy_to(const qpNodePath &other, int sort) const {
  nassertr(verify_complete(), fail());
  nassertr_always(!is_empty(), fail());
  nassertr(!other.is_empty(), fail());

  PandaNode *source_node = node();
  PT(PandaNode) copy_node = source_node->copy_subgraph();
  nassertr(copy_node != (PandaNode *)NULL, fail());

  return other.attach_new_node(copy_node, sort);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::attach_new_node
//       Access: Published
//  Description: Attaches a new node, with or without existing
//               parents, to the scene graph below the referenced node
//               of this NodePath.  This is the preferred way to add
//               nodes to the graph.
//
//               This does *not* automatically extend the current
//               NodePath to reflect the attachment; however, a
//               NodePath that does reflect this extension is
//               returned.
////////////////////////////////////////////////////////////////////
qpNodePath qpNodePath::
attach_new_node(PandaNode *node, int sort) const {
  nassertr(verify_complete(), qpNodePath::fail());
  nassertr_always(!is_empty(), qpNodePath());
  nassertr(node != (PandaNode *)NULL, qpNodePath());

  uncollapse_head();
  qpNodePath new_path(*this);
  new_path._head = PandaNode::attach(_head, node, sort);
  return new_path;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::remove_node
//       Access: Published
//  Description: Disconnects the referenced node from the scene graph.
//               This will also delete the node if there are no other
//               pointers to it.
//
//               Normally, this should be called only when you are
//               really done with the node.  If you want to remove a
//               node from the scene graph but keep it around for
//               later, you should probably use reparent_to() and put
//               it under a holding node instead.
//
//               After the node is removed, the qpNodePath will have
//               been cleared.
////////////////////////////////////////////////////////////////////
void qpNodePath::
remove_node() {
  nassertv(_error_type != ET_not_found);
  if (is_empty() || is_singleton()) {
    // If we have no parents, remove_node() is just a do-nothing
    // operation; if we have no nodes, maybe we were already removed.
    // In either case, quietly do nothing except to ensure the
    // qpNodePath is clear.
    (*this) = qpNodePath::removed();
    return;
  }

  uncollapse_head();
  PandaNode::detach(_head);

  (*this) = qpNodePath::removed();
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::output
//       Access: Published
//  Description: Writes a sensible description of the qpNodePath to the
//               indicated output stream.
////////////////////////////////////////////////////////////////////
void qpNodePath::
output(ostream &out) const {
  uncollapse_head();

  switch (_error_type) {
  case ET_not_found:
    out << "**not found**";
    return;
  case ET_removed:
    out << "**removed**";
    return;
  case ET_fail:
    out << "**error**";
    return;
  default:
    break;
  }

  if (_head == (qpNodePathComponent *)NULL) {
    out << "(empty)";
  } else {
    r_output(out, _head);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_state
//       Access: Published
//  Description: Returns the state changes that must be made to
//               transition from the render state of this node to the
//               render state of the other node.
////////////////////////////////////////////////////////////////////
CPT(RenderState) qpNodePath::
get_state(const qpNodePath &other) const {
  if (is_empty()) {
    return other.get_net_state();
  }
  if (other.is_empty()) {
    return get_net_state()->invert_compose(RenderState::make_empty());
  }
    
  nassertr(verify_complete(), RenderState::make_empty());
  nassertr(other.verify_complete(), RenderState::make_empty());

  int a_count, b_count;
  find_common_ancestor(*this, other, a_count, b_count);

  CPT(RenderState) a_state = r_get_partial_state(_head, a_count);
  CPT(RenderState) b_state = r_get_partial_state(other._head, b_count);
  return a_state->invert_compose(b_state);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_state
//       Access: Published
//  Description: Sets the state object on this node, relative to
//               the other node.  This computes a new state object
//               that has the indicated value when seen relative to
//               the other node.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_state(const qpNodePath &other, const RenderState *state) const {
  nassertv(_error_type == ET_ok && other._error_type == ET_ok);
  nassertv_always(!is_empty());

  // First, we perform a wrt to the parent, to get the conversion.
  qpNodePath parent = get_parent();
  CPT(RenderState) rel_state = parent.get_state(other);

  CPT(RenderState) new_state = rel_state->compose(state);
  set_state(new_state);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_transform
//       Access: Published
//  Description: Returns the relative transform to this node from the
//               other node; i.e. the transformation of this node
//               as seen from the other node.
////////////////////////////////////////////////////////////////////
CPT(TransformState) qpNodePath::
get_transform(const qpNodePath &other) const {
  if (other.is_empty()) {
    return get_net_transform();
  }
  if (is_empty()) {
    return other.get_net_transform()->invert_compose(TransformState::make_identity());
  }
    
  nassertr(verify_complete(), TransformState::make_identity());
  nassertr(other.verify_complete(), TransformState::make_identity());

  int a_count, b_count;
  find_common_ancestor(*this, other, a_count, b_count);

  CPT(TransformState) a_transform = r_get_partial_transform(_head, a_count);
  CPT(TransformState) b_transform = r_get_partial_transform(other._head, b_count);
  return b_transform->invert_compose(a_transform);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_transform
//       Access: Published
//  Description: Sets the transform object on this node, relative to
//               the other node.  This computes a new transform object
//               that will have the indicated value when seen from the
//               other node.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_transform(const qpNodePath &other, const TransformState *transform) const {
  nassertv(_error_type == ET_ok && other._error_type == ET_ok);
  nassertv_always(!is_empty());

  // First, we perform a wrt to the parent, to get the conversion.
  qpNodePath parent = get_parent();
  CPT(TransformState) rel_trans = other.get_transform(parent);

  CPT(TransformState) new_trans = rel_trans->compose(transform);
  set_transform(new_trans);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_pos
//       Access: Published
//  Description: Sets the translation component of the transform,
//               leaving rotation and scale untouched.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_pos(const LVecBase3f &pos) {
  nassertv_always(!is_empty());
  set_transform(get_transform()->set_pos(pos));
}

void qpNodePath::
set_x(float x) {
  nassertv_always(!is_empty());
  LPoint3f pos = get_pos();
  pos[0] = x;
  set_pos(pos);
}

void qpNodePath::
set_y(float y) {
  nassertv_always(!is_empty());
  LPoint3f pos = get_pos();
  pos[1] = y;
  set_pos(pos);
}

void qpNodePath::
set_z(float z) {
  nassertv_always(!is_empty());
  LPoint3f pos = get_pos();
  pos[2] = z;
  set_pos(pos);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_pos
//       Access: Published
//  Description: Retrieves the translation component of the transform.
////////////////////////////////////////////////////////////////////
LPoint3f qpNodePath::
get_pos() const {
  nassertr_always(!is_empty(), LPoint3f(0.0f, 0.0f, 0.0f));
  return get_transform()->get_pos();
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_hpr
//       Access: Published
//  Description: Sets the rotation component of the transform,
//               leaving translation and scale untouched.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_hpr(const LVecBase3f &hpr) {
  nassertv_always(!is_empty());
  CPT(TransformState) transform = get_transform();
  nassertv(transform->has_components());
  set_transform(transform->set_hpr(hpr));
}

void qpNodePath::
set_h(float h) {
  nassertv_always(!is_empty());
  CPT(TransformState) transform = get_transform();
  nassertv(transform->has_components());
  LVecBase3f hpr = transform->get_hpr();
  hpr[0] = h;
  set_transform(transform->set_hpr(hpr));
}

void qpNodePath::
set_p(float p) {
  nassertv_always(!is_empty());
  CPT(TransformState) transform = get_transform();
  nassertv(transform->has_components());
  LVecBase3f hpr = transform->get_hpr();
  hpr[1] = p;
  set_transform(transform->set_hpr(hpr));
}

void qpNodePath::
set_r(float r) {
  nassertv_always(!is_empty());
  CPT(TransformState) transform = get_transform();
  nassertv(transform->has_components());
  LVecBase3f hpr = transform->get_hpr();
  hpr[2] = r;
  set_transform(transform->set_hpr(hpr));
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_hpr
//       Access: Published
//  Description: Retrieves the rotation component of the transform.
////////////////////////////////////////////////////////////////////
LVecBase3f qpNodePath::
get_hpr() const {
  nassertr_always(!is_empty(), LVecBase3f(0.0f, 0.0f, 0.0f));
  CPT(TransformState) transform = get_transform();
  nassertr(transform->has_components(), LVecBase3f(0.0f, 0.0f, 0.0f));
  return transform->get_hpr();
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_hpr
//       Access: Published
//  Description: Retrieves the rotation component of the transform.
////////////////////////////////////////////////////////////////////
LVecBase3f qpNodePath::
get_hpr(float roll) const {
  // This function is deprecated.  It used to be a hack to work around
  // a problem with decomposing Euler angles, but since we no longer
  // depend on decomposing these, we shouldn't need this any more.
  return get_hpr();
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_scale
//       Access: Published
//  Description: Sets the scale component of the transform,
//               leaving translation and rotation untouched.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_scale(const LVecBase3f &scale) {
  nassertv_always(!is_empty());
  CPT(TransformState) transform = get_transform();
  nassertv(transform->has_components());
  set_transform(transform->set_scale(scale));
}

void qpNodePath::
set_sx(float sx) {
  nassertv_always(!is_empty());
  CPT(TransformState) transform = get_transform();
  nassertv(transform->has_components());
  LVecBase3f scale = transform->get_scale();
  scale[0] = sx;
  set_transform(transform->set_scale(scale));
}

void qpNodePath::
set_sy(float sy) {
  nassertv_always(!is_empty());
  CPT(TransformState) transform = get_transform();
  nassertv(transform->has_components());
  LVecBase3f scale = transform->get_scale();
  scale[1] = sy;
  set_transform(transform->set_scale(scale));
}

void qpNodePath::
set_sz(float sz) {
  nassertv_always(!is_empty());
  CPT(TransformState) transform = get_transform();
  nassertv(transform->has_components());
  LVecBase3f scale = transform->get_scale();
  scale[2] = sz;
  set_transform(transform->set_scale(scale));
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_scale
//       Access: Published
//  Description: Retrieves the scale component of the transform.
////////////////////////////////////////////////////////////////////
LVecBase3f qpNodePath::
get_scale() const {
  nassertr_always(!is_empty(), LVecBase3f(0.0f, 0.0f, 0.0f));
  CPT(TransformState) transform = get_transform();
  nassertr(transform->has_components(), LVecBase3f(0.0f, 0.0f, 0.0f));
  return transform->get_scale();
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_pos_hpr
//       Access: Published
//  Description: Sets the translation and rotation component of the
//               transform, leaving scale untouched.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_pos_hpr(const LVecBase3f &pos, const LVecBase3f &hpr) {
  nassertv_always(!is_empty());
  CPT(TransformState) transform = get_transform();
  nassertv(transform->has_components());
  transform = TransformState::make_pos_hpr_scale
    (pos, hpr, transform->get_scale());
  set_transform(transform);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_hpr_scale
//       Access: Published
//  Description: Sets the rotation and scale components of the
//               transform, leaving translation untouched.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_hpr_scale(const LVecBase3f &hpr, const LVecBase3f &scale) {
  nassertv_always(!is_empty());
  CPT(TransformState) transform = get_transform();
  transform = TransformState::make_pos_hpr_scale
    (transform->get_pos(), hpr, scale);
  set_transform(transform);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_pos_hpr_scale
//       Access: Published
//  Description: Completely replaces the transform with new
//               translation, rotation, and scale components.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_pos_hpr_scale(const LVecBase3f &pos, const LVecBase3f &hpr,
                  const LVecBase3f &scale) {
  nassertv_always(!is_empty());
  set_transform(TransformState::make_pos_hpr_scale
                (pos, hpr, scale));
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_mat
//       Access: Published
//  Description: Directly sets an arbitrary 4x4 transform matrix.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_mat(const LMatrix4f &mat) {
  nassertv_always(!is_empty());
  set_transform(TransformState::make_mat(mat));
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_color_scale
//       Access: Published
//  Description: Sets the color scale component of the transform,
//               leaving translation and rotation untouched.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_color_scale(const LVecBase4f &scale) {
  nassertv_always(!is_empty());
  node()->set_attrib(ColorScaleAttrib::make(scale));
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_color_scale
//       Access: Published
//  Description: Returns the complete color scale vector that has been
//               applied to the bottom node, or all 1's (identity) if
//               no scale has been applied.
////////////////////////////////////////////////////////////////////
const LVecBase4f &qpNodePath::
get_color_scale() const {
  static const LVecBase4f ident_scale(1.0f, 1.0f, 1.0f, 1.0f);
  nassertr_always(!is_empty(), ident_scale);
  const RenderAttrib *attrib =
    node()->get_attrib(ColorScaleAttrib::get_class_type());
  if (attrib != (const RenderAttrib *)NULL) {
    const ColorScaleAttrib *csa = DCAST(ColorScaleAttrib, attrib);
    return csa->get_scale();
  }

  return ident_scale;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::look_at
//       Access: Published
//  Description: Sets the hpr on this qpNodePath so that it
//               rotates to face the indicated point in space.
////////////////////////////////////////////////////////////////////
void qpNodePath::
look_at(const LPoint3f &point, const LVector3f &up) {
  nassertv_always(!is_empty());

  LPoint3f pos = get_pos();

  LMatrix3f mat;
  ::look_at(mat, point - pos, up);
  LVecBase3f scale, hpr;
  decompose_matrix(mat, scale, hpr);
 
  set_hpr(hpr);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::heads_up
//       Access: Published
//  Description: Behaves like look_at(), but with a strong preference
//               to keeping the up vector oriented in the indicated
//               "up" direction.
////////////////////////////////////////////////////////////////////
void qpNodePath::
heads_up(const LPoint3f &point, const LVector3f &up) {
  nassertv_always(!is_empty());

  LPoint3f pos = get_pos();

  LMatrix3f mat;
  ::heads_up(mat, point - pos, up);
  LVecBase3f scale, hpr;
  decompose_matrix(mat, scale, hpr);
 
  set_hpr(hpr);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_pos
//       Access: Published
//  Description: Sets the translation component of the transform,
//               relative to the other node.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_pos(const qpNodePath &other, const LVecBase3f &pos) {
  nassertv_always(!is_empty());
  set_transform(other, get_transform(other)->set_pos(pos));
}

void qpNodePath::
set_x(const qpNodePath &other, float x) {
  nassertv_always(!is_empty());
  LPoint3f pos = get_pos(other);
  pos[0] = x;
  set_pos(other, pos);
}

void qpNodePath::
set_y(const qpNodePath &other, float y) {
  nassertv_always(!is_empty());
  LPoint3f pos = get_pos(other);
  pos[1] = y;
  set_pos(other, pos);
}

void qpNodePath::
set_z(const qpNodePath &other, float z) {
  nassertv_always(!is_empty());
  LPoint3f pos = get_pos(other);
  pos[2] = z;
  set_pos(other, pos);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_pos
//       Access: Published
//  Description: Returns the relative position of the referenced node
//               as seen from the other node.
////////////////////////////////////////////////////////////////////
LPoint3f qpNodePath::
get_pos(const qpNodePath &other) const {
  nassertr_always(!is_empty(), LPoint3f(0.0f, 0.0f, 0.0f));
  return get_transform(other)->get_pos();
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_hpr
//       Access: Published
//  Description: Sets the rotation component of the transform,
//               relative to the other node.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_hpr(const qpNodePath &other, const LVecBase3f &hpr) {
  nassertv_always(!is_empty());
  CPT(TransformState) transform = get_transform(other);
  nassertv(transform->has_components());
  set_transform(other, transform->set_hpr(hpr));
}

void qpNodePath::
set_h(const qpNodePath &other, float h) {
  nassertv_always(!is_empty());
  CPT(TransformState) transform = get_transform(other);
  nassertv(transform->has_components());
  LVecBase3f hpr = transform->get_hpr();
  hpr[0] = h;
  set_transform(other, transform->set_hpr(hpr));
}

void qpNodePath::
set_p(const qpNodePath &other, float p) {
  nassertv_always(!is_empty());
  CPT(TransformState) transform = get_transform(other);
  nassertv(transform->has_components());
  LVecBase3f hpr = transform->get_hpr();
  hpr[1] = p;
  set_transform(other, transform->set_hpr(hpr));
}

void qpNodePath::
set_r(const qpNodePath &other, float r) {
  nassertv_always(!is_empty());
  CPT(TransformState) transform = get_transform(other);
  nassertv(transform->has_components());
  LVecBase3f hpr = transform->get_hpr();
  hpr[2] = r;
  set_transform(other, transform->set_hpr(hpr));
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_hpr
//       Access: Published
//  Description: Returns the relative orientation of the bottom node
//               as seen from the other node.
////////////////////////////////////////////////////////////////////
LVecBase3f qpNodePath::
get_hpr(const qpNodePath &other) const {
  nassertr_always(!is_empty(), LVecBase3f(0.0f, 0.0f, 0.0f));
  CPT(TransformState) transform = get_transform(other);
  nassertr(transform->has_components(), LVecBase3f(0.0f, 0.0f, 0.0f));
  return transform->get_hpr();
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_hpr
//       Access: Published
//  Description: Returns the relative orientation of the bottom node
//               as seen from the other node.
////////////////////////////////////////////////////////////////////
LVecBase3f qpNodePath::
get_hpr(const qpNodePath &other, float roll) const {
  // This is still doing it the dumb way, with a decomposition.  This
  // function is deprecated anyway.
  LMatrix4f mat = get_mat(other);
  LVector3f scale, hpr, pos;
  decompose_matrix(mat, scale, hpr, pos, roll);
  return hpr;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_scale
//       Access: Published
//  Description: Sets the scale component of the transform,
//               relative to the other node.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_scale(const qpNodePath &other, const LVecBase3f &scale) {
  nassertv_always(!is_empty());
  CPT(TransformState) transform = get_transform(other);
  nassertv(transform->has_components());
  set_transform(other, transform->set_scale(scale));
}

void qpNodePath::
set_sx(const qpNodePath &other, float sx) {
  nassertv_always(!is_empty());
  CPT(TransformState) transform = get_transform(other);
  nassertv(transform->has_components());
  LVecBase3f scale = transform->get_scale();
  scale[0] = sx;
  set_transform(other, transform->set_scale(scale));
}

void qpNodePath::
set_sy(const qpNodePath &other, float sy) {
  nassertv_always(!is_empty());
  CPT(TransformState) transform = get_transform(other);
  nassertv(transform->has_components());
  LVecBase3f scale = transform->get_scale();
  scale[1] = sy;
  set_transform(other, transform->set_scale(scale));
}

void qpNodePath::
set_sz(const qpNodePath &other, float sz) {
  nassertv_always(!is_empty());
  CPT(TransformState) transform = get_transform(other);
  nassertv(transform->has_components());
  LVecBase3f scale = transform->get_scale();
  scale[2] = sz;
  set_transform(other, transform->set_scale(scale));
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_scale
//       Access: Published
//  Description: Returns the relative scale of the bottom node
//               as seen from the other node.
////////////////////////////////////////////////////////////////////
LVecBase3f qpNodePath::
get_scale(const qpNodePath &other) const {
  nassertr_always(!is_empty(), LVecBase3f(0.0f, 0.0f, 0.0f));
  CPT(TransformState) transform = get_transform(other);
  nassertr(transform->has_components(), LVecBase3f(0.0f, 0.0f, 0.0f));
  return transform->get_scale();
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_pos_hpr
//       Access: Published
//  Description: Sets the translation and rotation component of the
//               transform, relative to the other node.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_pos_hpr(const qpNodePath &other, const LVecBase3f &pos,
            const LVecBase3f &hpr) {
  nassertv_always(!is_empty());
  CPT(TransformState) transform = get_transform(other);
  nassertv(transform->has_components());
  transform = TransformState::make_pos_hpr_scale
    (pos, hpr, transform->get_scale());
  set_transform(other, transform);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_hpr_scale
//       Access: Published
//  Description: Sets the rotation and scale components of the
//               transform, leaving translation untouched.  This, or
//               set_pos_hpr_scale, is the preferred way to update a
//               transform when both hpr and scale are to be changed.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_hpr_scale(const qpNodePath &other, const LVecBase3f &hpr, const LVecBase3f &scale) {
  nassertv_always(!is_empty());
  CPT(TransformState) transform = get_transform(other);
  transform = TransformState::make_pos_hpr_scale
    (transform->get_pos(), hpr, scale);
  set_transform(other, transform);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_pos_hpr_scale
//       Access: Published
//  Description: Completely replaces the transform with new
//               translation, rotation, and scale components, relative
//               to the other node.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_pos_hpr_scale(const qpNodePath &other,
                  const LVecBase3f &pos, const LVecBase3f &hpr,
                  const LVecBase3f &scale) {
  nassertv_always(!is_empty());
  set_transform(other, TransformState::make_pos_hpr_scale
                (pos, hpr, scale));
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_mat
//       Access: Published
//  Description: Returns the matrix that describes the coordinate
//               space of the bottom node, relative to the other
//               path's bottom node's coordinate space.
////////////////////////////////////////////////////////////////////
const LMatrix4f &qpNodePath::
get_mat(const qpNodePath &other) const {
  CPT(TransformState) transform = get_transform(other);
  // We can safely assume the transform won't go away when the
  // function returns, since its reference count is also held in the
  // cache.  This assumption allows us to return a reference to the
  // matrix, instead of having to return a matrix on the stack.
  nassertr(transform->get_ref_count() > 1, LMatrix4f::ident_mat());
  return transform->get_mat();
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_mat
//       Access: Published
//  Description: Converts the indicated matrix from the other's
//               coordinate space to the local coordinate space, and
//               applies it to the node.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_mat(const qpNodePath &other, const LMatrix4f &mat) {
  nassertv_always(!is_empty());
  set_transform(other, TransformState::make_mat(mat));
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_relative_point
//       Access: Published
//  Description: Given that the indicated point is in the coordinate
//               system of the other node, returns the same point in
//               this node's coordinate system.
////////////////////////////////////////////////////////////////////
LPoint3f qpNodePath::
get_relative_point(const qpNodePath &other, const LVecBase3f &point) {
  LPoint3f rel_point = LPoint3f(point) * other.get_mat(*this);
  return rel_point;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::look_at
//       Access: Published
//  Description: Sets the transform on this qpNodePath so that it
//               rotates to face the indicated point in space, which
//               is relative to the other qpNodePath.  This
//               will overwrite any previously existing scale on the
//               node, although it will preserve any translation.
////////////////////////////////////////////////////////////////////
void qpNodePath::
look_at(const qpNodePath &other, const LPoint3f &point, const LVector3f &up) {
  nassertv_always(!is_empty());

  qpNodePath parent = get_parent();
  LPoint3f rel_point = point * other.get_mat(parent);

  LPoint3f pos = get_pos();

  LMatrix3f mat;
  ::look_at(mat, rel_point - pos, up);
  LVecBase3f scale, hpr;
  decompose_matrix(mat, scale, hpr);
 
  set_hpr(hpr);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::heads_up
//       Access: Published
//  Description: Behaves like look_at(), but with a strong preference
//               to keeping the up vector oriented in the indicated
//               "up" direction.
////////////////////////////////////////////////////////////////////
void qpNodePath::
heads_up(const qpNodePath &other, const LPoint3f &point, const LVector3f &up) {
  nassertv_always(!is_empty());

  qpNodePath parent = get_parent();
  LPoint3f rel_point = point * other.get_mat(parent);

  LPoint3f pos = get_pos();

  LMatrix3f mat;
  ::heads_up(mat, rel_point - pos, up);
  LVecBase3f scale, hpr;
  decompose_matrix(mat, scale, hpr);
 
  set_hpr(hpr);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_color
//       Access: Published
//  Description: Applies a scene-graph color to the referenced node.
//               This color will apply to all geometry at this level
//               and below (that does not specify a new color or a
//               set_color_off()).
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_color(float r, float g, float b, float a,
          int priority) {
  set_color(Colorf(r, g, b, a), priority);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_color
//       Access: Published
//  Description: Applies a scene-graph color to the referenced node.
//               This color will apply to all geometry at this level
//               and below (that does not specify a new color or a
//               set_color_off()).
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_color(const Colorf &color, int priority) {
  nassertv_always(!is_empty());
  node()->set_attrib(ColorAttrib::make_flat(color), priority);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_color_off
//       Access: Published
//  Description: Sets the geometry at this level and below to render
//               using the geometry color.  This is normally the
//               default, but it may be useful to use this to
//               contradict set_color() at a higher node level (or,
//               with a priority, to override a set_color() at a lower
//               level).
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_color_off(int priority) {
  nassertv_always(!is_empty());
  node()->set_attrib(ColorAttrib::make_vertex(), priority);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::clear_color
//       Access: Published
//  Description: Completely removes any color adjustment from the node.
//               This allows the natural color of the geometry, or
//               whatever color transitions might be otherwise
//               affecting the geometry, to show instead.
////////////////////////////////////////////////////////////////////
void qpNodePath::
clear_color() {
  nassertv_always(!is_empty());
  node()->clear_attrib(ColorAttrib::get_class_type());
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::has_color
//       Access: Published
//  Description: Returns true if a color has been applied to the given
//               node, false otherwise.
////////////////////////////////////////////////////////////////////
bool qpNodePath::
has_color() const {
  nassertr_always(!is_empty(), false);
  return node()->has_attrib(ColorAttrib::get_class_type());
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_color
//       Access: Published
//  Description: Returns the color that has been assigned to the node,
//               or black if no color has been assigned.
////////////////////////////////////////////////////////////////////
Colorf qpNodePath::
get_color() const {
  nassertr_always(!is_empty(), false);
  const RenderAttrib *attrib =
    node()->get_attrib(ColorAttrib::get_class_type());
  if (attrib != (const RenderAttrib *)NULL) {
    const ColorAttrib *ca = DCAST(ColorAttrib, attrib);
    if (ca->get_color_type() == ColorAttrib::T_flat) {
      return ca->get_color();
    }
  }

  pgraph_cat.warning()
    << "get_color() called on " << *this << " which has no color set.\n";

  return Colorf(0.0f, 0.0f, 0.0f, 0.0f);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_bin
//       Access: Published
//  Description: Assigns the geometry at this level and below to the
//               named rendering bin.  It is the user's responsibility
//               to ensure that such a bin already exists, either via
//               the cull-bin Configrc variable, or by explicitly
//               creating a GeomBin of the appropriate type at
//               runtime.
//
//               There are two default bins created when Panda is
//               started: "default" and "fixed".  Normally, all
//               geometry is assigned to "default" unless specified
//               otherwise.  This bin renders opaque geometry in
//               state-sorted order, followed by transparent geometry
//               sorted back-to-front.  If any geometry is assigned to
//               "fixed", this will be rendered following all the
//               geometry in "default", in the order specified by
//               draw_order for each piece of geometry so assigned.
//
//               The draw_order parameter is meaningful only for
//               GeomBinFixed type bins, e.g. "fixed".  Other kinds of
//               bins ignore it.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_bin(const string &bin_name, int draw_order, int priority) {
  nassertv_always(!is_empty());
  node()->set_attrib(CullBinAttrib::make(bin_name, draw_order), priority);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::clear_bin
//       Access: Published
//  Description: Completely removes any bin adjustment that may have
//               been set via set_bin() from this particular node.
////////////////////////////////////////////////////////////////////
void qpNodePath::
clear_bin() {
  nassertv_always(!is_empty());
  node()->clear_attrib(CullBinAttrib::get_class_type());
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::has_bin
//       Access: Published
//  Description: Returns true if the node has been assigned to the a
//               particular rendering bin via set_bin(), false
//               otherwise.
////////////////////////////////////////////////////////////////////
bool qpNodePath::
has_bin() const {
  nassertr_always(!is_empty(), false);
  return node()->has_attrib(CullBinAttrib::get_class_type());
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_bin_name
//       Access: Published
//  Description: Returns the name of the bin that this particular node
//               was assigned to via set_bin(), or the empty string if
//               no bin was assigned.  See set_bin() and has_bin().
////////////////////////////////////////////////////////////////////
string qpNodePath::
get_bin_name() const {
  nassertr_always(!is_empty(), string());
  const RenderAttrib *attrib =
    node()->get_attrib(ColorAttrib::get_class_type());
  if (attrib != (const RenderAttrib *)NULL) {
    const CullBinAttrib *ba = DCAST(CullBinAttrib, attrib);
    return ba->get_bin_name();
  }

  return string();
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_bin_draw_order
//       Access: Published
//  Description: Returns the drawing order associated with the bin
//               that this particular node was assigned to via
//               set_bin(), or 0 if no bin was assigned.  See
//               set_bin() and has_bin().
////////////////////////////////////////////////////////////////////
int qpNodePath::
get_bin_draw_order() const {
  nassertr_always(!is_empty(), false);
  const RenderAttrib *attrib =
    node()->get_attrib(ColorAttrib::get_class_type());
  if (attrib != (const RenderAttrib *)NULL) {
    const CullBinAttrib *ba = DCAST(CullBinAttrib, attrib);
    return ba->get_draw_order();
  }

  return 0;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_texture
//       Access: Published
//  Description: Sets the geometry at this level and below to render
//               using the indicated texture.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_texture(Texture *tex, int priority) {
  nassertv_always(!is_empty());
  node()->set_attrib(TextureAttrib::make(tex), priority);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_texture_off
//       Access: Published
//  Description: Sets the geometry at this level and below to render
//               using no texture.  This is normally the default, but
//               it may be useful to use this to contradict
//               set_texture() at a higher node level (or, with a
//               priority, to override a set_texture() at a lower
//               level).
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_texture_off(int priority) {
  nassertv_always(!is_empty());
  node()->set_attrib(TextureAttrib::make_off(), priority);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::clear_texture
//       Access: Published
//  Description: Completely removes any texture adjustment that may
//               have been set via set_texture() or set_texture_off()
//               from this particular node.  This allows whatever
//               textures might be otherwise affecting the geometry to
//               show instead.
////////////////////////////////////////////////////////////////////
void qpNodePath::
clear_texture() {
  nassertv_always(!is_empty());
  node()->clear_attrib(TextureAttrib::get_class_type());
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::has_texture
//       Access: Published
//  Description: Returns true if a texture has been applied to this
//               particular node via set_texture(), false otherwise.
//               This is not the same thing as asking whether the
//               geometry at this node will be rendered with
//               texturing, as there may be a texture in effect from a
//               higher or lower level.
////////////////////////////////////////////////////////////////////
bool qpNodePath::
has_texture() const {
  nassertr_always(!is_empty(), false);
  const RenderAttrib *attrib =
    node()->get_attrib(TextureAttrib::get_class_type());
  if (attrib != (const RenderAttrib *)NULL) {
    const TextureAttrib *ta = DCAST(TextureAttrib, attrib);
    return !ta->is_off();
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::has_texture_off
//       Access: Published
//  Description: Returns true if a texture has been specifically
//               disabled on this particular node via
//               set_texture_off(), false otherwise.  This is not the
//               same thing as asking whether the geometry at this
//               node will be rendered untextured, as there may be a
//               texture in effect from a higher or lower level.
////////////////////////////////////////////////////////////////////
bool qpNodePath::
has_texture_off() const {
  nassertr_always(!is_empty(), false);
  const RenderAttrib *attrib =
    node()->get_attrib(ColorAttrib::get_class_type());
  if (attrib != (const RenderAttrib *)NULL) {
    const TextureAttrib *ta = DCAST(TextureAttrib, attrib);
    return ta->is_off();
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_texture
//       Access: Published
//  Description: Returns the texture that has been set on this
//               particular node, or NULL if no texture has been set.
//               This is not necessarily the texture that will be
//               applied to the geometry at or below this level, as
//               another texture at a higher or lower level may
//               override.
//
//               See also find_texture().
////////////////////////////////////////////////////////////////////
Texture *qpNodePath::
get_texture() const {
  nassertr_always(!is_empty(), NULL);
  const RenderAttrib *attrib =
    node()->get_attrib(TextureAttrib::get_class_type());
  if (attrib != (const RenderAttrib *)NULL) {
    const TextureAttrib *ta = DCAST(TextureAttrib, attrib);
    return ta->get_texture();
  }

  return NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::find_texture
//       Access: Published
//  Description: Returns the first texture found applied to geometry
//               at this node or below that matches the indicated name
//               (which may contain wildcards).  Returns the texture
//               if it is found, or NULL if it is not.
////////////////////////////////////////////////////////////////////
Texture *qpNodePath::
find_texture(const string &name) const {
  GlobPattern glob(name);
  return r_find_texture(node(), RenderState::make_empty(), glob);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::find_all_textures
//       Access: Published
//  Description: Returns a list of a textures applied to geometry at
//               this node and below.
////////////////////////////////////////////////////////////////////
TextureCollection qpNodePath::
find_all_textures() const {
  Textures textures;
  r_find_all_textures(node(), RenderState::make_empty(), textures);

  TextureCollection tc;
  Textures::iterator ti;
  for (ti = textures.begin(); ti != textures.end(); ++ti) {
    tc.add_texture(*ti);
  }
  return tc;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::find_all_textures
//       Access: Published
//  Description: Returns a list of a textures applied to geometry at
//               this node and below that match the indicated name
//               (which may contain wildcard characters).
////////////////////////////////////////////////////////////////////
TextureCollection qpNodePath::
find_all_textures(const string &name) const {
  Textures textures;
  r_find_all_textures(node(), RenderState::make_empty(), textures);

  GlobPattern glob(name);

  TextureCollection tc;
  Textures::iterator ti;
  for (ti = textures.begin(); ti != textures.end(); ++ti) {
    Texture *texture = (*ti);
    if (glob.matches(texture->get_name())) {
      tc.add_texture(texture);
    }
  }
  return tc;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_material
//       Access: Published
//  Description: Sets the geometry at this level and below to render
//               using the indicated material.
//
//               This operation copies the given material pointer.  If
//               the material structure is changed later, it must be
//               reapplied via another call to set_material().
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_material(Material *mat, int priority) {
  nassertv_always(!is_empty());
  nassertv(mat != NULL);

  // We create a temporary Material pointer, a copy of the one we are
  // given, to allow the user to monkey with the material and set it
  // again later, with the desired effect.  If we stored the user's
  // pointer directly, it would be bad if the user later modified the
  // values within the Material.
  PT(Material) temp = new Material(*mat);
  const Material *mp = MaterialPool::get_material(temp);

  node()->set_attrib(MaterialAttrib::make(mp), priority);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_material_off
//       Access: Published
//  Description: Sets the geometry at this level and below to render
//               using no material.  This is normally the default, but
//               it may be useful to use this to contradict
//               set_material() at a higher node level (or, with a
//               priority, to override a set_material() at a lower
//               level).
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_material_off(int priority) {
  nassertv_always(!is_empty());
  node()->set_attrib(MaterialAttrib::make_off(), priority);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::clear_material
//       Access: Published
//  Description: Completely removes any material adjustment that may
//               have been set via set_material() from this particular
//               node.
////////////////////////////////////////////////////////////////////
void qpNodePath::
clear_material() {
  nassertv_always(!is_empty());
  node()->clear_attrib(MaterialAttrib::get_class_type());
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::has_material
//       Access: Published
//  Description: Returns true if a material has been applied to this
//               particular node via set_material(), false otherwise.
////////////////////////////////////////////////////////////////////
bool qpNodePath::
has_material() const {
  nassertr_always(!is_empty(), false);
  const RenderAttrib *attrib =
    node()->get_attrib(MaterialAttrib::get_class_type());
  if (attrib != (const RenderAttrib *)NULL) {
    const MaterialAttrib *ma = DCAST(MaterialAttrib, attrib);
    return !ma->is_off();
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_material
//       Access: Published
//  Description: Returns the material that has been set on this
//               particular node, or NULL if no material has been set.
//               This is not necessarily the material that will be
//               applied to the geometry at or below this level, as
//               another material at a higher or lower level may
//               override.
//
//               This function returns a copy of the given material,
//               to allow changes, if desired.  Once changes are made,
//               they should be reapplied via set_material().
////////////////////////////////////////////////////////////////////
PT(Material) qpNodePath::
get_material() const {
  nassertr_always(!is_empty(), NULL);
  const RenderAttrib *attrib =
    node()->get_attrib(MaterialAttrib::get_class_type());
  if (attrib != (const RenderAttrib *)NULL) {
    const MaterialAttrib *ma = DCAST(MaterialAttrib, attrib);
    return new Material(*ma->get_material());
  }

  return NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_fog
//       Access: Published
//  Description: Sets the geometry at this level and below to render
//               using the indicated fog.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_fog(qpFog *fog, int priority) {
  nassertv_always(!is_empty());
  node()->set_attrib(FogAttrib::make(fog), priority);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_fog_off
//       Access: Published
//  Description: Sets the geometry at this level and below to render
//               using no fog.  This is normally the default, but
//               it may be useful to use this to contradict
//               set_fog() at a higher node level (or, with a
//               priority, to override a set_fog() at a lower
//               level).
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_fog_off(int priority) {
  nassertv_always(!is_empty());
  node()->set_attrib(FogAttrib::make_off(), priority);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::clear_fog
//       Access: Published
//  Description: Completely removes any fog adjustment that may
//               have been set via set_fog() or set_fog_off()
//               from this particular node.  This allows whatever
//               fogs might be otherwise affecting the geometry to
//               show instead.
////////////////////////////////////////////////////////////////////
void qpNodePath::
clear_fog() {
  nassertv_always(!is_empty());
  node()->clear_attrib(FogAttrib::get_class_type());
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::has_fog
//       Access: Published
//  Description: Returns true if a fog has been applied to this
//               particular node via set_fog(), false otherwise.
//               This is not the same thing as asking whether the
//               geometry at this node will be rendered with
//               fog, as there may be a fog in effect from a higher or
//               lower level.
////////////////////////////////////////////////////////////////////
bool qpNodePath::
has_fog() const {
  nassertr_always(!is_empty(), false);
  const RenderAttrib *attrib =
    node()->get_attrib(FogAttrib::get_class_type());
  if (attrib != (const RenderAttrib *)NULL) {
    const FogAttrib *fa = DCAST(FogAttrib, attrib);
    return !fa->is_off();
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::has_fog_off
//       Access: Published
//  Description: Returns true if a fog has been specifically
//               disabled on this particular node via
//               set_fog_off(), false otherwise.  This is not the
//               same thing as asking whether the geometry at this
//               node will be rendered unfogged, as there may be a
//               fog in effect from a higher or lower level.
////////////////////////////////////////////////////////////////////
bool qpNodePath::
has_fog_off() const {
  nassertr_always(!is_empty(), false);
  const RenderAttrib *attrib =
    node()->get_attrib(FogAttrib::get_class_type());
  if (attrib != (const RenderAttrib *)NULL) {
    const FogAttrib *fa = DCAST(FogAttrib, attrib);
    return fa->is_off();
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_fog
//       Access: Published
//  Description: Returns the fog that has been set on this
//               particular node, or NULL if no fog has been set.
//               This is not necessarily the fog that will be
//               applied to the geometry at or below this level, as
//               another fog at a higher or lower level may
//               override.
////////////////////////////////////////////////////////////////////
qpFog *qpNodePath::
get_fog() const {
  nassertr_always(!is_empty(), NULL);
  const RenderAttrib *attrib =
    node()->get_attrib(FogAttrib::get_class_type());
  if (attrib != (const RenderAttrib *)NULL) {
    const FogAttrib *fa = DCAST(FogAttrib, attrib);
    return fa->get_fog();
  }

  return NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_render_mode_wireframe
//       Access: Published
//  Description: Sets up the geometry at this level and below (unless
//               overridden) to render in wireframe mode.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_render_mode_wireframe(int priority) {
  nassertv_always(!is_empty());
  node()->set_attrib(RenderModeAttrib::make(RenderModeAttrib::M_wireframe), priority);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_render_mode_filled
//       Access: Published
//  Description: Sets up the geometry at this level and below (unless
//               overridden) to render in filled (i.e. not wireframe)
//               mode.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_render_mode_filled(int priority) {
  nassertv_always(!is_empty());
  node()->set_attrib(RenderModeAttrib::make(RenderModeAttrib::M_filled), priority);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::clear_render_mode
//       Access: Published
//  Description: Completely removes any render mode adjustment that
//               may have been set on this node via
//               set_render_mode_wireframe() or
//               set_render_mode_filled().
////////////////////////////////////////////////////////////////////
void qpNodePath::
clear_render_mode() {
  nassertv_always(!is_empty());
  node()->clear_attrib(RenderModeAttrib::get_class_type());
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::has_render_mode
//       Access: Published
//  Description: Returns true if a render mode has been explicitly set
//               on this particular node via
//               set_render_mode_wireframe() or
//               set_render_mode_filled(), false otherwise.
////////////////////////////////////////////////////////////////////
bool qpNodePath::
has_render_mode() const {
  nassertr_always(!is_empty(), false);
  return node()->has_attrib(RenderModeAttrib::get_class_type());
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_two_sided
//       Access: Published
//  Description: Specifically sets or disables two-sided rendering
//               mode on this particular node.  If no other nodes
//               override, this will cause backfacing polygons to be
//               drawn (in two-sided mode, true) or culled (in
//               one-sided mode, false).
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_two_sided(bool two_sided, int priority) {
  nassertv_always(!is_empty());

  CullFaceAttrib::Mode mode =
    two_sided ?
    CullFaceAttrib::M_cull_none :
    CullFaceAttrib::M_cull_clockwise;

  node()->set_attrib(CullFaceAttrib::make(mode), priority);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::clear_two_sided
//       Access: Published
//  Description: Completely removes any two-sided adjustment that
//               may have been set on this node via set_two_sided().
//               The geometry at this level and below will
//               subsequently be rendered either two-sided or
//               one-sided, according to whatever other nodes may have
//               had set_two_sided() on it, or according to the
//               initial state otherwise.
////////////////////////////////////////////////////////////////////
void qpNodePath::
clear_two_sided() {
  nassertv_always(!is_empty());
  node()->clear_attrib(CullFaceAttrib::get_class_type());
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::has_two_sided
//       Access: Published
//  Description: Returns true if a two-sided adjustment has been
//               explicitly set on this particular node via
//               set_two_sided().  If this returns true, then
//               get_two_sided() may be called to determine which has
//               been set.
////////////////////////////////////////////////////////////////////
bool qpNodePath::
has_two_sided() const {
  nassertr_always(!is_empty(), false);
  return node()->has_attrib(CullFaceAttrib::get_class_type());
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_two_sided
//       Access: Published
//  Description: Returns true if two-sided rendering has been
//               specifically set on this node via set_two_sided(), or
//               false if one-sided rendering has been specifically
//               set, or if nothing has been specifically set.  See
//               also has_two_sided().  This does not necessarily
//               imply that the geometry will or will not be rendered
//               two-sided, as there may be other nodes that override.
////////////////////////////////////////////////////////////////////
bool qpNodePath::
get_two_sided() const {
  nassertr_always(!is_empty(), false);
  const RenderAttrib *attrib =
    node()->get_attrib(CullFaceAttrib::get_class_type());
  if (attrib != (const RenderAttrib *)NULL) {
    const CullFaceAttrib *cfa = DCAST(CullFaceAttrib, attrib);
    return (cfa->get_mode() == CullFaceAttrib::M_cull_none);
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_depth_test
//       Access: Published
//  Description: Specifically sets or disables the testing of the
//               depth buffer on this particular node.  This is
//               normally on in the 3-d scene graph and off in the 2-d
//               scene graph; it should be on for rendering most 3-d
//               objects properly.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_depth_test(bool depth_test, int priority) {
  nassertv_always(!is_empty());

  DepthTestAttrib::Mode mode =
    depth_test ?
    DepthTestAttrib::M_less :
    DepthTestAttrib::M_none;

  node()->set_attrib(DepthTestAttrib::make(mode), priority);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::clear_depth_test
//       Access: Published
//  Description: Completely removes any depth-test adjustment that
//               may have been set on this node via set_depth_test().
////////////////////////////////////////////////////////////////////
void qpNodePath::
clear_depth_test() {
  nassertv_always(!is_empty());
  node()->clear_attrib(DepthTestAttrib::get_class_type());
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::has_depth_test
//       Access: Published
//  Description: Returns true if a depth-test adjustment has been
//               explicitly set on this particular node via
//               set_depth_test().  If this returns true, then
//               get_depth_test() may be called to determine which has
//               been set.
////////////////////////////////////////////////////////////////////
bool qpNodePath::
has_depth_test() const {
  nassertr_always(!is_empty(), false);
  return node()->has_attrib(DepthTestAttrib::get_class_type());
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_depth_test
//       Access: Published
//  Description: Returns true if depth-test rendering has been
//               specifically set on this node via set_depth_test(), or
//               false if depth-test rendering has been specifically
//               disabled, or if nothing has been specifically set.  See
//               also has_depth_test().
////////////////////////////////////////////////////////////////////
bool qpNodePath::
get_depth_test() const {
  nassertr_always(!is_empty(), false);
  const RenderAttrib *attrib =
    node()->get_attrib(DepthTestAttrib::get_class_type());
  if (attrib != (const RenderAttrib *)NULL) {
    const DepthTestAttrib *dta = DCAST(DepthTestAttrib, attrib);
    return (dta->get_mode() != DepthTestAttrib::M_none);
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_depth_write
//       Access: Published
//  Description: Specifically sets or disables the writing to the
//               depth buffer on this particular node.  This is
//               normally on in the 3-d scene graph and off in the 2-d
//               scene graph; it should be on for rendering most 3-d
//               objects properly.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_depth_write(bool depth_write, int priority) {
  nassertv_always(!is_empty());

  DepthWriteAttrib::Mode mode =
    depth_write ?
    DepthWriteAttrib::M_on :
    DepthWriteAttrib::M_off;

  node()->set_attrib(DepthWriteAttrib::make(mode), priority);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::clear_depth_write
//       Access: Published
//  Description: Completely removes any depth-write adjustment that
//               may have been set on this node via set_depth_write().
////////////////////////////////////////////////////////////////////
void qpNodePath::
clear_depth_write() {
  nassertv_always(!is_empty());
  node()->clear_attrib(DepthWriteAttrib::get_class_type());
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::has_depth_write
//       Access: Published
//  Description: Returns true if a depth-write adjustment has been
//               explicitly set on this particular node via
//               set_depth_write().  If this returns true, then
//               get_depth_write() may be called to determine which has
//               been set.
////////////////////////////////////////////////////////////////////
bool qpNodePath::
has_depth_write() const {
  nassertr_always(!is_empty(), false);
  return node()->has_attrib(DepthWriteAttrib::get_class_type());
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_depth_write
//       Access: Published
//  Description: Returns true if depth-write rendering has been
//               specifically set on this node via set_depth_write(), or
//               false if depth-write rendering has been specifically
//               disabled, or if nothing has been specifically set.  See
//               also has_depth_write().
////////////////////////////////////////////////////////////////////
bool qpNodePath::
get_depth_write() const {
  nassertr_always(!is_empty(), false);
  const RenderAttrib *attrib =
    node()->get_attrib(DepthWriteAttrib::get_class_type());
  if (attrib != (const RenderAttrib *)NULL) {
    const DepthWriteAttrib *dta = DCAST(DepthWriteAttrib, attrib);
    return (dta->get_mode() != DepthWriteAttrib::M_off);
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::do_billboard_axis
//       Access: Published
//  Description: Performs a billboard-type rotate to the indicated
//               camera node, one time only, and leaves the object
//               rotated.  This is similar in principle to heads_up().
//               However, it does lose both translate and scale
//               components of the matrix.
////////////////////////////////////////////////////////////////////
void qpNodePath::
do_billboard_axis(const qpNodePath &camera, float offset) {
  nassertv_always(!is_empty());

  qpNodePath parent = get_parent();
  LMatrix4f rel_mat = camera.get_mat(parent);

  LVector3f up = LVector3f::up();
  LVector3f rel_pos = -rel_mat.get_row3(3);

  LMatrix4f mat;
  ::heads_up(mat, rel_pos, up);

  // Also slide the geometry towards the camera according to the
  // offset factor.
  if (offset != 0.0f) {
    LVector3f translate = rel_mat.get_row3(3);
    translate.normalize();
    translate *= offset;
    mat.set_row(3, translate);
  }

  set_mat(mat);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::do_billboard_point_eye
//       Access: Published
//  Description: Performs a billboard-type rotate to the indicated
//               camera node, one time only, and leaves the object
//               rotated.  This is similar in principle to look_at(),
//               although the point_eye billboard effect cannot be
//               achieved using the ordinary look_at() call.
////////////////////////////////////////////////////////////////////
void qpNodePath::
do_billboard_point_eye(const qpNodePath &camera, float offset) {
  nassertv_always(!is_empty());

  qpNodePath parent = get_parent();
  LMatrix4f rel_mat = camera.get_mat(parent);

  LVector3f up = LVector3f::up() * rel_mat;
  LVector3f rel_pos = LVector3f::forward() * rel_mat;

  LMatrix4f mat;
  ::look_at(mat, rel_pos, up);

  // Also slide the geometry towards the camera according to the
  // offset factor.
  if (offset != 0.0f) {
    LVector3f translate = rel_mat.get_row3(3);
    translate.normalize();
    translate *= offset;
    mat.set_row(3, translate);
  }

  set_mat(mat);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::do_billboard_point_world
//       Access: Published
//  Description: Performs a billboard-type rotate to the indicated
//               camera node, one time only, and leaves the object
//               rotated.  This is similar in principle to look_at().
////////////////////////////////////////////////////////////////////
void qpNodePath::
do_billboard_point_world(const qpNodePath &camera, float offset) {
  nassertv_always(!is_empty());

  qpNodePath parent = get_parent();
  LMatrix4f rel_mat = camera.get_mat(parent);

  LVector3f up = LVector3f::up();
  LVector3f rel_pos = -rel_mat.get_row3(3);

  LMatrix4f mat;
  ::look_at(mat, rel_pos, up);

  // Also slide the geometry towards the camera according to the
  // offset factor.
  if (offset != 0.0f) {
    LVector3f translate = rel_mat.get_row3(3);
    translate.normalize();
    translate *= offset;
    mat.set_row(3, translate);
  }

  set_mat(mat);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_billboard_axis
//       Access: Published
//  Description: Puts a billboard transition on the node such that it
//               will rotate in two dimensions around the up axis.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_billboard_axis(float offset) {
  nassertv_always(!is_empty());
  CPT(RenderEffect) billboard = BillboardEffect::make
    (LVector3f::up(), false, true, 
     offset, qpNodePath(), LPoint3f(0.0f, 0.0f, 0.0f));
  node()->set_effect(billboard);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_billboard_point_eye
//       Access: Published
//  Description: Puts a billboard transition on the node such that it
//               will rotate in three dimensions about the origin,
//               keeping its up vector oriented to the top of the
//               camera.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_billboard_point_eye(float offset) {
  nassertv_always(!is_empty());
  CPT(RenderEffect) billboard = BillboardEffect::make
    (LVector3f::up(), true, false,
     offset, qpNodePath(), LPoint3f(0.0f, 0.0f, 0.0f));
  node()->set_effect(billboard);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_billboard_point_world
//       Access: Published
//  Description: Puts a billboard transition on the node such that it
//               will rotate in three dimensions about the origin,
//               keeping its up vector oriented to the sky.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_billboard_point_world(float offset) {
  nassertv_always(!is_empty());
  CPT(RenderEffect) billboard = BillboardEffect::make
    (LVector3f::up(), false, false,
     offset, qpNodePath(), LPoint3f(0.0f, 0.0f, 0.0f));
  node()->set_effect(billboard);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::clear_billboard
//       Access: Published
//  Description: Removes any billboard effect from the node.
////////////////////////////////////////////////////////////////////
void qpNodePath::
clear_billboard() {
  nassertv_always(!is_empty());
  node()->clear_effect(BillboardEffect::get_class_type());
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::has_billboard
//       Access: Published
//  Description: Returns true if there is any billboard effect on
//               the node.
////////////////////////////////////////////////////////////////////
bool qpNodePath::
has_billboard() const {
  nassertr_always(!is_empty(), false);
  return node()->has_effect(BillboardEffect::get_class_type());
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::set_transparency
//       Access: Published
//  Description: Specifically sets or disables transparent rendering
//               mode on this particular node.  If no other nodes
//               override, this will cause items with a non-1 value
//               for alpha color to be rendered partially transparent.
////////////////////////////////////////////////////////////////////
void qpNodePath::
set_transparency(bool transparency, int priority) {
  nassertv_always(!is_empty());

  TransparencyAttrib::Mode mode =
    transparency ?
    TransparencyAttrib::M_alpha :
    TransparencyAttrib::M_none;

  node()->set_attrib(TransparencyAttrib::make(mode), priority);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::clear_transparency
//       Access: Published
//  Description: Completely removes any transparency adjustment that
//               may have been set on this node via set_transparency().
//               The geometry at this level and below will
//               subsequently be rendered either transparent or not,
//               to whatever other nodes may have had
//               set_transparency() on them.
////////////////////////////////////////////////////////////////////
void qpNodePath::
clear_transparency() {
  nassertv_always(!is_empty());
  node()->clear_attrib(TransparencyAttrib::get_class_type());
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::has_transparency
//       Access: Published
//  Description: Returns true if a transparent-rendering adjustment
//               has been explicitly set on this particular node via
//               set_transparency().  If this returns true, then
//               get_transparency() may be called to determine whether
//               transparency has been explicitly enabled or
//               explicitly disabled for this node.
////////////////////////////////////////////////////////////////////
bool qpNodePath::
has_transparency() const {
  nassertr_always(!is_empty(), false);
  return node()->has_attrib(TransparencyAttrib::get_class_type());
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_transparency
//       Access: Published
//  Description: Returns true if transparent rendering has been
//               specifically set on this node via set_transparency(), or
//               false if nontransparent rendering has been specifically
//               set, or if nothing has been specifically set.  See
//               also has_transparency().  This does not necessarily
//               imply that the geometry will or will not be rendered
//               transparent, as there may be other nodes that override.
////////////////////////////////////////////////////////////////////
bool qpNodePath::
get_transparency() const {
  nassertr_always(!is_empty(), false);
  const RenderAttrib *attrib =
    node()->get_attrib(TransparencyAttrib::get_class_type());
  if (attrib != (const RenderAttrib *)NULL) {
    const TransparencyAttrib *ta = DCAST(TransparencyAttrib, attrib);
    return (ta->get_mode() != TransparencyAttrib::M_none);
  }

  return false;
}


////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_hidden_ancestor
//       Access: Published
//  Description: Returns the NodePath at or above the referenced node
//               that is hidden to the indicated camera(s), or an
//               empty NodePath if no ancestor of the referenced node
//               is hidden (and the node should be visible).
////////////////////////////////////////////////////////////////////
qpNodePath qpNodePath::
get_hidden_ancestor(DrawMask camera_mask) const {
  qpNodePathComponent *comp;
  for (comp = _head; 
       comp != (qpNodePathComponent *)NULL; 
       comp = comp->get_next()) {
    PandaNode *node = comp->get_node();
    if ((node->get_draw_mask() & camera_mask).is_zero()) {
      qpNodePath result;
      result._head = comp;
      return result;
    }
  }

  return not_found();
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_stashed_ancestor
//       Access: Published
//  Description: Returns the NodePath at or above the referenced node
//               that is stashed, or an empty NodePath if no ancestor
//               of the referenced node is stashed (and the node should
//               be visible).
////////////////////////////////////////////////////////////////////
qpNodePath qpNodePath::
get_stashed_ancestor() const {
  qpNodePathComponent *comp = _head;
  if (comp != (qpNodePathComponent *)NULL) {
    qpNodePathComponent *next = comp->get_next();

    while (next != (qpNodePathComponent *)NULL) {
      PandaNode *node = comp->get_node();
      PandaNode *parent_node = next->get_node();

      if (parent_node->find_stashed(node) >= 0) {
        qpNodePath result;
        result._head = comp;
        return result;
      }

      comp = next;
      next = next->get_next();
    }
  }

  return not_found();
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::verify_complete
//       Access: Published
//  Description: Returns true if all of the nodes described in the
//               qpNodePath are connected and the top node is the top
//               of the graph, or false otherwise.
////////////////////////////////////////////////////////////////////
bool qpNodePath::
verify_complete() const {
  if (is_empty()) {
    return true;
  }

  uncollapse_head();
  const qpNodePathComponent *comp = _head;
  nassertr(comp != (const qpNodePathComponent *)NULL, false);

  PandaNode *node = comp->get_node();
  nassertr(node != (const PandaNode *)NULL, false);
  int length = comp->get_length();

  comp = comp->get_next();
  length--;
  while (comp != (const qpNodePathComponent *)NULL) {
    PandaNode *next_node = comp->get_node();
    nassertr(next_node != (const PandaNode *)NULL, false);

    if (node->find_parent(next_node) < 0) {
      return false;
    }

    if (comp->get_length() != length) {
      return false;
    }

    node = next_node;
    comp = comp->get_next();
    length--;
  }

  // Now that we've reached the top, we should have no parents.
  return length == 0 && node->get_num_parents() == 0;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::prepare_scene
//       Access: Published
//  Description: Walks through the scene graph beginning at the bottom
//               node, and does whatever initialization is required to
//               render the scene properly with the indicated GSG.  It
//               is not strictly necessary to call this, since the GSG
//               will initialize itself when the scene is rendered,
//               but this may take some of the overhead away from that
//               process.
//
//               If force_retained_mode is true, retained mode is set
//               on the geometry encountered, regardless of the
//               setting of the retained-mode Config variable.
//               Otherwise, retained mode is set only if the
//               retained-mode Config variable is true.
////////////////////////////////////////////////////////////////////
void qpNodePath::
prepare_scene(GraphicsStateGuardianBase *gsg, bool force_retained_mode) {
  nassertv_always(!is_empty());

  CPT(RenderState) net_state = get_net_state();
  r_prepare_scene(node(), net_state, gsg, 
                  retained_mode || force_retained_mode);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::show_bounds
//       Access: Published
//  Description: Causes the bounding volume of the bottom node and all
//               of its descendants (that is, the bounding volume
//               associated with the the bottom arc) to be rendered,
//               if possible.  The rendering method is less than
//               optimal; this is intended primarily for debugging.
////////////////////////////////////////////////////////////////////
void qpNodePath::
show_bounds() {
  nassertv_always(!is_empty());
  nassertv(false);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::hide_bounds
//       Access: Published
//  Description: Stops the rendering of the bounding volume begun with
//               show_bounds().
////////////////////////////////////////////////////////////////////
void qpNodePath::
hide_bounds() {
  nassertv_always(!is_empty());
  nassertv(false);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::get_bounds
//       Access: Published
//  Description: Returns a newly-allocated bounding volume containing
//               the bottom node and all of its descendants.  This is
//               the bounding volume on the bottom arc, converted to
//               the local coordinate space of the node.
////////////////////////////////////////////////////////////////////
PT(BoundingVolume) qpNodePath::
get_bounds() const {
  nassertr_always(!is_empty(), new BoundingSphere);

  PandaNode *this_node = node();
  PT(BoundingVolume) bv = this_node->get_bound().make_copy();
  if (bv->is_of_type(GeometricBoundingVolume::get_class_type()) &&
      !this_node->get_transform()->is_identity()) {

    // The bounding volume has already been transformed by the node's
    // matrix.  We'd rather return a bounding volume in the node's
    // space, so we have to untransform it now.  Ick.
    GeometricBoundingVolume *gbv = DCAST(GeometricBoundingVolume, bv);
    const LMatrix4f &mat = get_parent().get_mat(*this);
    gbv->xform(mat);
  }

  return bv;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::write_bounds
//       Access: Published
//  Description: Writes a description of the bounding volume
//               containing the bottom node and all of its descendants
//               to the indicated output stream.
////////////////////////////////////////////////////////////////////
void qpNodePath::
write_bounds(ostream &out) const {
  get_bounds()->write(out);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::calc_tight_bounds
//       Access: Published
//  Description: Calculates the minimum and maximum vertices of all
//               Geoms at this qpNodePath's bottom node and below.  This
//               is a tight bounding box; it will generally be tighter
//               than the bounding volume returned by get_bounds()
//               (but it is more expensive to compute).
//
//               The return value is true if any points are within the
//               bounding volume, or false if none are.
////////////////////////////////////////////////////////////////////
bool qpNodePath::
calc_tight_bounds(LPoint3f &min_point, LPoint3f &max_point) {
  min_point.set(0.0f, 0.0f, 0.0f);
  max_point.set(0.0f, 0.0f, 0.0f);
  nassertr_always(!is_empty(), false);

  bool found_any = false;
  r_calc_tight_bounds(node(), min_point, max_point, found_any, 
                      TransformState::make_identity());

  return found_any;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::flatten_light
//       Access: Published
//  Description: Lightly flattens out the hierarchy below this node by
//               applying transforms, colors, and texture matrices
//               from the arcs onto the vertices, but does not remove
//               any nodes.
//
//               This can result in improved rendering performance
//               because there will be fewer transforms in the
//               resulting scene graph, but the number of nodes will
//               remain the same.
//
//               Particularly, any qpNodePaths that reference nodes
//               within this hierarchy will not be damaged.  However,
//               since this operation will remove transforms from the
//               scene graph, it may be dangerous to apply to arcs
//               where you expect to dynamically modify the transform,
//               or where you expect the geometry to remain in a
//               particular local coordinate system.
//
//               The return value is always 0, since flatten_light
//               does not remove any arcs.
////////////////////////////////////////////////////////////////////
int qpNodePath::
flatten_light() {
  nassertr_always(!is_empty(), 0);
  qpSceneGraphReducer gr;
  gr.apply_attribs(node());

  return 0;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::flatten_medium
//       Access: Published
//  Description: A more thorough flattening than flatten_light(), this
//               first applies all the transforms, colors, and texture
//               matrices from the arcs onto the vertices, and then
//               removes unneeded grouping nodes--nodes that have
//               exactly one child, for instance, but have no special
//               properties in themselves.
//
//               This results in improved perforamance over
//               flatten_light() because the number of nodes in the
//               scene graph is reduced.
//
//               If max_children is specified, it represents the
//               maximum number of children a node is allowed to have
//               and still be flattened.  Normally, this is 1; we
//               don't typically want to flatten a node that has
//               multiple children.  However, sometimes this may be
//               desirable; set this parameter to control the limit.
//               If this is set to -1, there is no limit.
//
//               The return value is the number of arcs removed.
////////////////////////////////////////////////////////////////////
int qpNodePath::
flatten_medium() {
  nassertr_always(!is_empty(), 0);
  qpSceneGraphReducer gr;
  gr.apply_attribs(node());
  int num_removed = gr.flatten(node(), false);

  return num_removed;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::flatten_strong
//       Access: Published
//  Description: The strongest possible flattening.  This first
//               applies all of the transforms to the vertices, as in
//               flatten_medium(), but then it will combine sibling
//               nodes together when possible, in addition to removing
//               unnecessary parent-child nodes.  This can result in
//               substantially fewer nodes, but any nicely-grouped
//               hierachical bounding volumes may be lost.
//
//               It is generally a good idea to apply this kind of
//               flattening only to nodes that will be culled largely
//               as a single unit, like a car.  Applying this to an
//               entire scene may result in overall poorer performance
//               because of less-effective culling.
////////////////////////////////////////////////////////////////////
int qpNodePath::
flatten_strong() {
  nassertr_always(!is_empty(), 0);
  qpSceneGraphReducer gr;
  gr.apply_attribs(node());
  int num_removed = gr.flatten(node(), true);

  return num_removed;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::write_bam_file
//       Access: Published
//  Description: Writes the contents of this node and below out to a
//               bam file with the indicated filename.  This file may
//               then be read in again, as is, at some later point.
//               Returns true if successful, false on some kind of
//               error.
////////////////////////////////////////////////////////////////////
bool qpNodePath::
write_bam_file(const string &filename) const {
  nassertr_always(!is_empty(), false);

  /*
  BamFile bam_file;

  bool okflag = false;

  if (bam_file.open_write(filename)) {
    if (bam_file.write_object(node())) {
      okflag = true;
    }
    bam_file.close();
  }
  return okflag;
  */

  // At the moment, we can't do this because BamFile is defined in
  // loader and loader depends on pgraph.
  nassertr(false, false);
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::uncollapse_head
//       Access: Private
//  Description: Quietly and transparently uncollapses the _head
//               pointer if it needs it.  This can happen only when
//               two distinct NodePaths are collapsed into the same
//               path after the removal of an instance somewhere
//               higher up the chain.
////////////////////////////////////////////////////////////////////
void qpNodePath::
uncollapse_head() const {
  if (_head != (qpNodePathComponent *)NULL && _head->is_collapsed()) {
    ((qpNodePath *)this)->_head = _head->uncollapse();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::find_common_ancestor
//       Access: Private, Static
//  Description: Walks up from both qpNodePaths to find the first node
//               that both have in common, if any.  Fills a_count and
//               b_count with the number of nodes below the common
//               node in each path.
////////////////////////////////////////////////////////////////////
void qpNodePath::
find_common_ancestor(const qpNodePath &a, const qpNodePath &b,
                     int &a_count, int &b_count) {
  nassertv(!a.is_empty() && !b.is_empty());
  a.uncollapse_head();
  b.uncollapse_head();

  qpNodePathComponent *ac = a._head;
  qpNodePathComponent *bc = b._head;
  a_count = 0;
  b_count = 0;

  // Shorten up the longer one until they are the same length.
  while (ac->get_length() > bc->get_length()) {
    nassertv(ac != (qpNodePathComponent *)NULL);
    ac = ac->get_next();
    a_count++;
  }
  while (bc->get_length() > ac->get_length()) {
    nassertv(bc != (qpNodePathComponent *)NULL);
    bc = bc->get_next();
    b_count++;
  }

  // Now shorten them both up until we reach the same component.
  while (ac != bc) {
    // These shouldn't go to NULL unless they both go there together. 
    nassertv(ac != (qpNodePathComponent *)NULL);
    nassertv(bc != (qpNodePathComponent *)NULL);
    ac = ac->get_next();
    a_count++;
    bc = bc->get_next();
    b_count++;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::r_get_net_state
//       Access: Private
//  Description: Recursively determines the net state chnages to the
//               indicated component node from the root of the graph.
////////////////////////////////////////////////////////////////////
CPT(RenderState) qpNodePath::
r_get_net_state(qpNodePathComponent *comp) const {
  if (comp == (qpNodePathComponent *)NULL) {
    return RenderState::make_empty();
  } else {
    CPT(RenderState) state = comp->get_node()->get_state();
    return r_get_net_state(comp->get_next())->compose(state);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::r_get_partial_state
//       Access: Private
//  Description: Recursively determines the net state changes to the
//               indicated component node from the nth node above it.
//               If n exceeds the length of the path, this returns the
//               net transform from the root of the graph.
////////////////////////////////////////////////////////////////////
CPT(RenderState) qpNodePath::
r_get_partial_state(qpNodePathComponent *comp, int n) const {
  if (n == 0 || comp == (qpNodePathComponent *)NULL) {
    return RenderState::make_empty();
  } else {
    CPT(RenderState) state = comp->get_node()->get_state();
    return r_get_partial_state(comp->get_next(), n - 1)->compose(state);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::r_get_net_transform
//       Access: Private
//  Description: Recursively determines the net transform to the
//               indicated component node from the root of the graph.
////////////////////////////////////////////////////////////////////
CPT(TransformState) qpNodePath::
r_get_net_transform(qpNodePathComponent *comp) const {
  if (comp == (qpNodePathComponent *)NULL) {
    return TransformState::make_identity();
  } else {
    CPT(TransformState) transform = comp->get_node()->get_transform();
    return r_get_net_transform(comp->get_next())->compose(transform);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::r_get_partial_transform
//       Access: Private
//  Description: Recursively determines the net transform to the
//               indicated component node from the nth node above it.
//               If n exceeds the length of the path, this returns the
//               net transform from the root of the graph.
////////////////////////////////////////////////////////////////////
CPT(TransformState) qpNodePath::
r_get_partial_transform(qpNodePathComponent *comp, int n) const {
  if (n == 0 || comp == (qpNodePathComponent *)NULL) {
    return TransformState::make_identity();
  } else {
    CPT(TransformState) transform = comp->get_node()->get_transform();
    return r_get_partial_transform(comp->get_next(), n - 1)->compose(transform);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::r_output
//       Access: Private
//  Description: The recursive implementation of output(), this writes
//               the names of each node component in order from
//               beginning to end, by first walking to the end of the
//               linked list and then outputting from there.
////////////////////////////////////////////////////////////////////
void qpNodePath::
r_output(ostream &out, qpNodePathComponent *comp) const {
  PandaNode *node = comp->get_node();
  qpNodePathComponent *next = comp->get_next();
  if (next != (qpNodePathComponent *)NULL) {
    // This is not the head of the list; keep going up.
    r_output(out, next);
    out << "/";

    PandaNode *parent_node = next->get_node();
    if (parent_node->find_stashed(node) >= 0) {
      // The node is stashed.
      out << "@@";

    } else if (node->find_parent(parent_node) < 0) {
      // Oops, there's an error.  This shouldn't happen.
      out << ".../";
    }
  }

  // Now output this component.
  if (node->has_name()) {
    out << node->get_name();
  } else {
    out << "-" << node->get_type();
  }
  //  out << "[" << comp->get_length() << "]";
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::find_matches
//       Access: Private
//  Description: Finds up to max_matches matches against the given
//               path string from this node and deeper.  The
//               max_matches count indicates the maximum number of
//               matches to return, or -1 not to limit the number
//               returned.
////////////////////////////////////////////////////////////////////
void qpNodePath::
find_matches(qpNodePathCollection &result, const string &path,
             int max_matches) const {
  if (is_empty()) {
    pgraph_cat.warning()
      << "Attempt to extend an empty qpNodePath by '" << path
      << "'.\n";
    return;
  }
  qpFindApproxPath approx_path;
  if (approx_path.add_string(path)) {
    find_matches(result, approx_path, max_matches);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::find_matches
//       Access: Private
//  Description: Finds up to max_matches matches against the given
//               approx_path from this node and deeper.  The
//               max_matches count indicates the maximum number of
//               matches to return, or -1 not to limit the number
//               returned.
////////////////////////////////////////////////////////////////////
void qpNodePath::
find_matches(qpNodePathCollection &result, qpFindApproxPath &approx_path,
             int max_matches) const {
  if (is_empty()) {
    pgraph_cat.warning()
      << "Attempt to extend an empty qpNodePath by: " << approx_path << ".\n";
    return;
  }
  qpFindApproxLevelEntry start(*this, approx_path);
  qpFindApproxLevel level;
  level.add_entry(start);
  r_find_matches(result, level, max_matches, _max_search_depth);
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::r_find_matches
//       Access: Private
//  Description: The recursive implementation of find_matches.
////////////////////////////////////////////////////////////////////
void qpNodePath::
r_find_matches(qpNodePathCollection &result,
               const qpFindApproxLevel &level,
               int max_matches, int num_levels_remaining) const {
  // Go on to the next level.  If we exceeded the requested maximum
  // depth, stop.
  if (num_levels_remaining <= 0) {
    return;
  }
  num_levels_remaining--;

  qpFindApproxLevel next_level;
  bool okflag = true;

  // For each node in the current level, build up the set of possible
  // matches in the next level.
  qpFindApproxLevel::Vec::const_iterator li;
  for (li = level._v.begin(); li != level._v.end() && okflag; ++li) {
    const qpFindApproxLevelEntry &entry = (*li);

    if (entry.is_solution()) {
      // Does this entry already represent a solution?
      result.add_path(entry._node_path);
    } else {
      entry.consider_node(result, next_level, max_matches);
    }

    if (max_matches > 0 && result.get_num_paths() >= max_matches) {
      // Really, we just want to return here.  But returning from
      // within the conditional within the for loop seems to sometimes
      // cause a compiler fault in GCC.  We'll use a semaphore
      // variable instead.
      okflag = false;
    }
  }

  // Now recurse on the next level.
  if (okflag) {
    r_find_matches(result, next_level, max_matches, num_levels_remaining);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::r_adjust_all_priorities
//       Access: Private
//  Description: The recursive implementation of
//               adjust_all_priorities().  This walks through the
//               subgraph defined by the indicated node and below.
////////////////////////////////////////////////////////////////////
void qpNodePath::
r_adjust_all_priorities(PandaNode *node, int adjustment) {
  node->set_state(node->get_state()->adjust_all_priorities(adjustment));

  PandaNode::Children cr = node->get_children();
  int num_children = cr.get_num_children();
  for (int i = 0; i < num_children; i++) {
    r_adjust_all_priorities(cr.get_child(i), adjustment);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::r_calc_tight_bounds
//       Access: Private
//  Description: 
////////////////////////////////////////////////////////////////////
void qpNodePath::
r_calc_tight_bounds(PandaNode *node, LPoint3f &min_point, LPoint3f &max_point,
                    bool &found_any, const TransformState *transform) {
  CPT(TransformState) next_transform = 
    transform->compose(node->get_transform());
  if (node->is_geom_node()) {
    qpGeomNode *gnode;
    DCAST_INTO_V(gnode, node);

    const LMatrix4f &mat = next_transform->get_mat();
    int num_geoms = gnode->get_num_geoms();
    for (int i = 0; i < num_geoms; i++) {
      Geom *geom = gnode->get_geom(i);
      Geom::VertexIterator vi = geom->make_vertex_iterator();
      int num_prims = geom->get_num_prims();

      for (int p = 0; p < num_prims; p++) {
        int length = geom->get_length(p);
        for (int v = 0; v < length; v++) {
          Vertexf vertex = geom->get_next_vertex(vi) * mat;
          
          if (found_any) {
            min_point.set(min(min_point[0], vertex[0]),
                          min(min_point[1], vertex[1]),
                          min(min_point[2], vertex[2]));
            max_point.set(max(max_point[0], vertex[0]),
                          max(max_point[1], vertex[1]),
                          max(max_point[2], vertex[2]));
          } else {
            min_point = vertex;
            max_point = vertex;
            found_any = true;
          }
        }
      }
    }
  }

  // Now consider children.
  PandaNode::Children cr = node->get_children();
  int num_children = cr.get_num_children();
  for (int i = 0; i < num_children; i++) {
    r_calc_tight_bounds(cr.get_child(i), min_point, max_point,
                        found_any, next_transform);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::r_find_texture
//       Access: Private
//  Description: 
////////////////////////////////////////////////////////////////////
Texture * qpNodePath::
r_find_texture(PandaNode *node, const RenderState *state,
               const GlobPattern &glob) const {
  CPT(RenderState) next_state = state->compose(node->get_state());

  if (node->is_geom_node()) {
    qpGeomNode *gnode;
    DCAST_INTO_R(gnode, node, NULL);

    int num_geoms = gnode->get_num_geoms();
    for (int i = 0; i < num_geoms; i++) {
      CPT(RenderState) geom_state = 
        next_state->compose(gnode->get_geom_state(i));

      // Look for a TextureAttrib on the state.
      const RenderAttrib *attrib =
        geom_state->get_attrib(TextureAttrib::get_class_type());
      if (attrib != (const RenderAttrib *)NULL) {
        const TextureAttrib *ta = DCAST(TextureAttrib, attrib);
        Texture *texture = ta->get_texture();
        if (texture != (Texture *)NULL) {
          if (glob.matches(texture->get_name())) {
            return texture;
          }
        }
      }
    }
  }

  // Now consider children.
  PandaNode::Children cr = node->get_children();
  int num_children = cr.get_num_children();
  for (int i = 0; i < num_children; i++) {
    Texture *result = r_find_texture(cr.get_child(i), next_state, glob);
    if (result != (Texture *)NULL) {
      return result;
    }
  }

  return NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::r_find_all_textures
//       Access: Private
//  Description: 
////////////////////////////////////////////////////////////////////
void qpNodePath::
r_find_all_textures(PandaNode *node, const RenderState *state,
                    qpNodePath::Textures &textures) const {
  CPT(RenderState) next_state = state->compose(node->get_state());

  if (node->is_geom_node()) {
    qpGeomNode *gnode;
    DCAST_INTO_V(gnode, node);

    int num_geoms = gnode->get_num_geoms();
    for (int i = 0; i < num_geoms; i++) {
      CPT(RenderState) geom_state = 
        next_state->compose(gnode->get_geom_state(i));

      // Look for a TextureAttrib on the state.
      const RenderAttrib *attrib =
        geom_state->get_attrib(TextureAttrib::get_class_type());
      if (attrib != (const RenderAttrib *)NULL) {
        const TextureAttrib *ta = DCAST(TextureAttrib, attrib);
        Texture *texture = ta->get_texture();
        if (texture != (Texture *)NULL) {
          textures.insert(texture);
        }
      }
    }
  }

  // Now consider children.
  PandaNode::Children cr = node->get_children();
  int num_children = cr.get_num_children();
  for (int i = 0; i < num_children; i++) {
    r_find_all_textures(cr.get_child(i), next_state, textures);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: qpNodePath::r_prepare_scene
//       Access: Private
//  Description: The recursive implementation of prepare_scene.
////////////////////////////////////////////////////////////////////
void qpNodePath::
r_prepare_scene(PandaNode *node, const RenderState *state,
                GraphicsStateGuardianBase *gsg, bool do_retained_mode) {
  if (node->is_geom_node()) {
    qpGeomNode *gnode;
    DCAST_INTO_V(gnode, node);

    /* 
       Not implemented yet in pgraph.  Maybe we don't need this anyway.
    if (do_retained_mode) {
      gnode->prepare(gsg);
    }
    */

    int num_geoms = gnode->get_num_geoms();
    for (int i = 0; i < num_geoms; i++) {
      CPT(RenderState) geom_state = state->compose(gnode->get_geom_state(i));
      const RenderAttrib *attrib = 
        geom_state->get_attrib(TextureAttrib::get_class_type());
      if (attrib != (const RenderAttrib *)NULL) {
        const TextureAttrib *ta;
        DCAST_INTO_V(ta, attrib);
        Texture *texture = ta->get_texture();
        if (texture != (Texture *)NULL) {
          texture->prepare(gsg);
        }
      }
    }
  }

  int num_children = node->get_num_children();
  for (int i = 0; i < num_children; i++) {
    PandaNode *child = node->get_child(i);
    CPT(RenderState) child_state = state->compose(child->get_state());
    r_prepare_scene(child, child_state, gsg, do_retained_mode);
  }
}
