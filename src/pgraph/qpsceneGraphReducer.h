// Filename: qpsceneGraphReducer.h
// Created by:  drose (14Mar02)
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

#ifndef qpSCENEGRAPHREDUCER_H
#define qpSCENEGRAPHREDUCER_H

#include "pandabase.h"
#include "transformState.h"
#include "renderAttrib.h"
#include "renderState.h"
#include "qpgeomTransformer.h"

#include "typedObject.h"
#include "pointerTo.h"

///////////////////////////////////////////////////////////////////
//       Class : qpSceneGraphReducer
// Description : An interface for simplifying ("flattening") scene
//               graphs by eliminating unneeded nodes and collapsing
//               out unneeded state changes and transforms.
//
//               This class is designed so that it may be inherited
//               from and specialized, if needed, to fine-tune the
//               flattening behavior, but normally the default
//               behavior is sufficient.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA qpSceneGraphReducer {
PUBLISHED:
  qpSceneGraphReducer();
  virtual ~qpSceneGraphReducer();

  enum AttribTypes {
    TT_transform       = 0x001,
    TT_color           = 0x002,
    TT_color_scale     = 0x004,
    TT_tex_matrix      = 0x008,
    TT_other           = 0x010,
  };

  void apply_attribs(PandaNode *node, int attrib_types = ~0);

  int flatten(PandaNode *root, bool combine_siblings);

protected:
  class AccumulatedAttribs {
  public:
    INLINE AccumulatedAttribs();
    INLINE AccumulatedAttribs(const AccumulatedAttribs &copy);
    INLINE void operator = (const AccumulatedAttribs &copy);

    void write(ostream &out, int attrib_types, int indent_level) const;

    void collect(PandaNode *node, int attrib_types);
    void apply_to_node(PandaNode *node, int attrib_types);
    void apply_to_vertices(PandaNode *node, int attrib_types,
                           qpGeomTransformer &transfomer);

    CPT(TransformState) _transform;
    CPT(RenderAttrib) _color;
    CPT(RenderAttrib) _color_scale;
    CPT(RenderAttrib) _tex_matrix;
    CPT(RenderState) _other;
  };

  void r_apply_attribs(PandaNode *node, int attrib_types,
                       AccumulatedAttribs trans);

  int r_flatten(PandaNode *grandparent_node, PandaNode *parent_node,
                bool combine_siblings);
  int flatten_siblings(PandaNode *parent_node);

  virtual bool consider_child(PandaNode *grandparent_node,
                              PandaNode *parent_node, PandaNode *child_node);
  virtual bool consider_siblings(PandaNode *parent_node, PandaNode *child1,
                                 PandaNode *child2);

  virtual bool do_flatten_child(PandaNode *grandparent_node, 
                                PandaNode *parent_node, PandaNode *child_node);

  virtual PandaNode *do_flatten_siblings(PandaNode *parent_node, 
                                         PandaNode *child1, PandaNode *child2);

  virtual PT(PandaNode) collapse_nodes(PandaNode *node1, PandaNode *node2, 
                                       bool siblings);
  virtual void choose_name(PandaNode *preserve, PandaNode *source1, 
                           PandaNode *source2);

private:
  qpGeomTransformer _transformer;
};

#include "qpsceneGraphReducer.I"

#endif
