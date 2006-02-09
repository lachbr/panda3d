// Filename: portalNode.cxx
// Created by:  drose (16Mar02)
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

#include "portalNode.h"

#include "geomNode.h"
#include "cullTraverserData.h"
#include "cullTraverser.h"
#include "renderState.h"
#include "portalClipper.h"
#include "transformState.h"
#include "colorScaleAttrib.h"
#include "transparencyAttrib.h"
#include "datagram.h"
#include "datagramIterator.h"
#include "bamReader.h"
#include "bamWriter.h"

TypeHandle PortalNode::_type_handle;


////////////////////////////////////////////////////////////////////
//     Function: PortalNode::Constructor
//       Access: Public
//  Description: Default constructor, just an empty node, no geo
//               This is used to read portal from model. You can also
//               use this from python to create an empty portal. Then
//               you can set the vertices yourself, with addVertex.
////////////////////////////////////////////////////////////////////
PortalNode::
PortalNode(const string &name) :
  PandaNode(name),
  _from_portal_mask(PortalMask::all_on()),
  _into_portal_mask(PortalMask::all_on()),
  _flags(0)
{
  _visible = false;
  _open = true;
}

////////////////////////////////////////////////////////////////////
//     Function: PortalNode::Constructor
//       Access: Public
//  Description: Create a default rectangle as portal. Use this
//               to create an arbitrary portal and setup from Python
////////////////////////////////////////////////////////////////////
PortalNode::
PortalNode(const string &name, LPoint3f pos, float scale) :
  PandaNode(name),
  _from_portal_mask(PortalMask::all_on()),
  _into_portal_mask(PortalMask::all_on()),
  _flags(0)
{
  add_vertex(LPoint3f(pos[0]-1.0*scale, pos[1], pos[2]-1.0*scale));
  add_vertex(LPoint3f(pos[0]+1.0*scale, pos[1], pos[2]-1.0*scale));
  add_vertex(LPoint3f(pos[0]+1.0*scale, pos[1], pos[2]+1.0*scale));
  add_vertex(LPoint3f(pos[0]-1.0*scale, pos[1], pos[2]+1.0*scale));

  _visible = false;
  _open = true;
}

////////////////////////////////////////////////////////////////////
//     Function: PortalNode::Copy Constructor
//       Access: Protected
//  Description:
////////////////////////////////////////////////////////////////////
PortalNode::
PortalNode(const PortalNode &copy) :
  PandaNode(copy),
  _from_portal_mask(copy._from_portal_mask),
  _into_portal_mask(copy._into_portal_mask),
  _flags(copy._flags),
  _vertices(copy._vertices),
  _cell_in(copy._cell_in),
  _cell_out(copy._cell_out),
  _visible(copy._visible),
  _open(copy._open)
{
}

////////////////////////////////////////////////////////////////////
//     Function: PortalNode::Destructor
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
PortalNode::
~PortalNode() {
}

////////////////////////////////////////////////////////////////////
//     Function: PortalNode::make_copy
//       Access: Public, Virtual
//  Description: Returns a newly-allocated Node that is a shallow copy
//               of this one.  It will be a different Node pointer,
//               but its internal data may or may not be shared with
//               that of the original Node.
////////////////////////////////////////////////////////////////////
PandaNode *PortalNode::
make_copy() const {
  return new PortalNode(*this);
}

////////////////////////////////////////////////////////////////////
//     Function: PortalNode::preserve_name
//       Access: Public, Virtual
//  Description: Returns true if the node's name has extrinsic meaning
//               and must be preserved across a flatten operation,
//               false otherwise.
////////////////////////////////////////////////////////////////////
bool PortalNode::
preserve_name() const {
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: PortalNode::xform
//       Access: Public, Virtual
//  Description: Transforms the contents of this node by the indicated
//               matrix, if it means anything to do so.  For most
//               kinds of nodes, this does nothing.
////////////////////////////////////////////////////////////////////
void PortalNode::
xform(const LMatrix4f &mat) {
  nassertv(!mat.is_nan());

}

////////////////////////////////////////////////////////////////////
//     Function: PortalNode::combine_with
//       Access: Public, Virtual
//  Description: Collapses this node with the other node, if possible,
//               and returns a pointer to the combined node, or NULL
//               if the two nodes cannot safely be combined.
//
//               The return value may be this, other, or a new node
//               altogether.
//
//               This function is called from GraphReducer::flatten(),
//               and need not deal with children; its job is just to
//               decide whether to collapse the two nodes and what the
//               collapsed node should look like.
////////////////////////////////////////////////////////////////////
PandaNode *PortalNode::
combine_with(PandaNode *other) {
  if (is_exact_type(get_class_type()) &&
      other->is_exact_type(get_class_type())) {
    // Two PortalNodes can combine, but only if they have the same
    // name, because the name is often meaningful.
    PortalNode *cother = DCAST(PortalNode, other);
    if (get_name() == cother->get_name()) {
      return this;
    }

    // Two PortalNodes with different names can't combine.
    return (PandaNode *)NULL;
  }

  return PandaNode::combine_with(other);
}

////////////////////////////////////////////////////////////////////
//     Function: PortalNode::has_cull_callback
//       Access: Public, Virtual
//  Description: Should be overridden by derived classes to return
//               true if cull_callback() has been defined.  Otherwise,
//               returns false to indicate cull_callback() does not
//               need to be called for this node during the cull
//               traversal.
////////////////////////////////////////////////////////////////////
bool PortalNode::
has_cull_callback() const {
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: PortalNode::cull_callback
//       Access: Public, Virtual
//  Description: If has_cull_callback() returns true, this function
//               will be called during the cull traversal to perform
//               reduced frustum culling. Basically, once the scenegraph
//               comes across a portal node, it calculates a CulltraverserData
//               with which cell, this portal leads out to and the new frustum.
//               Then it traverses that child
//
//               The return value is true if this node should be
//               visible, or false if it should be culled.
////////////////////////////////////////////////////////////////////
bool PortalNode::
cull_callback(CullTraverser *trav, CullTraverserData &data) {
  PortalClipper *portal_viewer = trav->get_portal_clipper();
  set_visible(false);
  if (is_open() && !_cell_out.is_empty() && portal_viewer) {
    //CullTraverserData next_data(data, _cell_out);
    portal_cat.debug() << "checking portal node  " << *this << endl;
    PT(GeometricBoundingVolume) vf = trav->get_view_frustum();
    PT(BoundingVolume) reduced_frustum;
     
    // following three functions do nothing, if the portal is not visible
    portal_viewer->prepare_portal(data._node_path.get_node_path());
    portal_viewer->clip_portal(data._node_path.get_node_path());
    if ((reduced_frustum = portal_viewer->get_reduced_frustum(data._node_path.get_node_path()))) {
      set_visible(true);
      // This reduced frustum is in camera space
      portal_cat.debug() << "got reduced frustum " << reduced_frustum << endl;
      vf = DCAST(GeometricBoundingVolume, reduced_frustum);
      
      // keep a copy of this reduced frustum
      PT(BoundingHexahedron) new_bh = DCAST(BoundingHexahedron, vf->make_copy());
      
      // trasform it to cull_center space
      //      CPT(TransformState) cull_center_transform = 
      //portal_viewer->_scene_setup->get_cull_center().get_transform(_cell_out);

      /*
      CPT(TransformState) transform = portal_viewer->_scene_setup->get_cull_center().get_net_transform();
      CPT(TransformState) portal_transform =
        data._modelview_transform->compose(transform);
      */
    
      // Get the net trasform of the _cell_out as seen from the camera.
      CPT(TransformState) cell_transform = 
        trav->get_camera_transform()->invert_compose(_cell_out.get_net_transform());

      CPT(TransformState) frustum_transform = 
        _cell_out.get_net_transform()->invert_compose(portal_viewer->_scene_setup->get_cull_center().get_net_transform());

      new_bh->xform(frustum_transform->get_mat());
      
      portal_cat.spam() << "new_bh is " << *new_bh << "\n";
      
      CullTraverserData next_data(_cell_out, 
                                  cell_transform,
                                  data._state, new_bh, NULL);

      // Make this cell show with the reduced frustum
      //      _cell_out.show();
      // all nodes visible through this portal, should have this node's frustum
      PT(BoundingHexahedron) old_bh = portal_viewer->get_reduced_frustum();
      portal_viewer->set_reduced_frustum(new_bh);
      portal_cat.spam() << "cull_callback: before traversing " << _cell_out.get_name() << endl;
      trav->traverse(next_data);
      portal_cat.spam() << "cull_callback: after traversing " << _cell_out.get_name() << endl;
      // make sure traverser is not drawing this node again
      //    _cell_out.hide();

      // reset portal viewer frustum for the siblings;
      portal_viewer->set_reduced_frustum(old_bh);
    }
  }
  // Now carry on to render our child nodes.
  return true;
}


////////////////////////////////////////////////////////////////////
//     Function: PortalNode::output
//       Access: Public, Virtual
//  Description: Writes a brief description of the node to the
//               indicated output stream.  This is invoked by the <<
//               operator.  It may be overridden in derived classes to
//               include some information relevant to the class.
////////////////////////////////////////////////////////////////////
void PortalNode::
output(ostream &out) const {
  PandaNode::output(out);
}

/*
////////////////////////////////////////////////////////////////////
//     Function: PortalNode::draw
//       Access: Public
//  Description: Draws the vertices of this portal rectangle to the 
//               screen with a line 

////////////////////////////////////////////////////////////////////
void PortalNode::
draw() const {
  move_to(get_vertex(0));
  draw_to(get_vertex(1));
  draw_to(get_vertex(2));
  draw_to(get_vertex(3));
}
*/

////////////////////////////////////////////////////////////////////
//     Function: PortalNode::recompute_bound
//       Access: Protected, Virtual
//  Description: Recomputes the dynamic bounding volume for this
//               object.  This is the bounding volume for the node and
//               all of its children, and normally does not need to be
//               specialized beyond PandaNode; we specialize this
//               function just so we can piggyback on it the
//               setting the _net_portal_mask bits.
////////////////////////////////////////////////////////////////////
BoundingVolume *PortalNode::
recompute_bound(int pipeline_stage) {
  BoundingVolume *result = PandaNode::recompute_bound(pipeline_stage);
  return result;
}

////////////////////////////////////////////////////////////////////
//     Function: PortalNode::recompute_internal_bound
//       Access: Protected, Virtual
//  Description: Called when needed to recompute the node's
//               _internal_bound object.  Nodes that contain anything
//               of substance should redefine this to do the right
//               thing.
////////////////////////////////////////////////////////////////////
BoundingVolume *PortalNode::
recompute_internal_bound(int pipeline_stage) {
  // First, get ourselves a fresh, empty bounding volume.
  BoundingVolume *bound = PandaNode::recompute_internal_bound(pipeline_stage);
  nassertr(bound != (BoundingVolume *)NULL, bound);

  GeometricBoundingVolume *gbv = DCAST(GeometricBoundingVolume, bound);

  // Now actually compute the bounding volume by putting it around all
  // of our vertices.

  const LPoint3f *vertices_begin = &_vertices[0];
  const LPoint3f *vertices_end = vertices_begin + _vertices.size();

  // Now actually compute the bounding volume by putting it around all
  gbv->around(vertices_begin, vertices_end);

  return bound;
}

////////////////////////////////////////////////////////////////////
//     Function: PortalNode::get_last_pos_state
//       Access: Protected
//  Description: Returns a RenderState for rendering the ghosted
//               portal rectangle that represents the previous frame's
//               position, for those collision nodes that indicate a
//               velocity.
////////////////////////////////////////////////////////////////////
CPT(RenderState) PortalNode::
get_last_pos_state() {
  // Once someone asks for this pointer, we hold its reference count
  // and never free it.
  static CPT(RenderState) state = (const RenderState *)NULL;
  if (state == (const RenderState *)NULL) {
    state = RenderState::make
      (ColorScaleAttrib::make(LVecBase4f(1.0f, 1.0f, 1.0f, 0.5f)),
       TransparencyAttrib::make(TransparencyAttrib::M_alpha));
  }

  return state;
}


////////////////////////////////////////////////////////////////////
//     Function: PortalNode::register_with_read_factory
//       Access: Public, Static
//  Description: Tells the BamReader how to create objects of type
//               PortalNode.
////////////////////////////////////////////////////////////////////
void PortalNode::
register_with_read_factory() {
  BamReader::get_factory()->register_factory(get_class_type(), make_from_bam);
}

////////////////////////////////////////////////////////////////////
//     Function: PortalNode::write_datagram
//       Access: Public, Virtual
//  Description: Writes the contents of this object to the datagram
//               for shipping out to a Bam file.
////////////////////////////////////////////////////////////////////
void PortalNode::
write_datagram(BamWriter *manager, Datagram &dg) {
  PandaNode::write_datagram(manager, dg);

  dg.add_uint16(_vertices.size());
  for (Vertices::const_iterator vi = _vertices.begin();
       vi != _vertices.end();
       ++vi) {
    (*vi).write_datagram(dg);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PortalNode::complete_pointers
//       Access: Public, Virtual
//  Description: Receives an array of pointers, one for each time
//               manager->read_pointer() was called in fillin().
//               Returns the number of pointers processed.
////////////////////////////////////////////////////////////////////
int PortalNode::
complete_pointers(TypedWritable **p_list, BamReader *manager) {
  int pi = PandaNode::complete_pointers(p_list, manager);

  return pi;
}

////////////////////////////////////////////////////////////////////
//     Function: PortalNode::make_from_bam
//       Access: Protected, Static
//  Description: This function is called by the BamReader's factory
//               when a new object of type PortalNode is encountered
//               in the Bam file.  It should create the PortalNode
//               and extract its information from the file.
////////////////////////////////////////////////////////////////////
TypedWritable *PortalNode::
make_from_bam(const FactoryParams &params) {
  PortalNode *node = new PortalNode("");
  DatagramIterator scan;
  BamReader *manager;

  parse_params(params, scan, manager);
  node->fillin(scan, manager);

  return node;
}

////////////////////////////////////////////////////////////////////
//     Function: PortalNode::fillin
//       Access: Protected
//  Description: This internal function is called by make_from_bam to
//               read in all of the relevant data from the BamFile for
//               the new PortalNode.
////////////////////////////////////////////////////////////////////
void PortalNode::
fillin(DatagramIterator &scan, BamReader *manager) {
  PandaNode::fillin(scan, manager);

  int num_vertices = scan.get_uint16();
  _vertices.reserve(num_vertices);
  for (int i = 0; i < num_vertices; i++) {
    LPoint3f vertex;
    vertex.read_datagram(scan);
    _vertices.push_back(vertex);
  }

  /*
  _from_portal_mask.set_word(scan.get_uint32());
  _into_portal_mask.set_word(scan.get_uint32());
  _flags = scan.get_uint8();
  */
}
