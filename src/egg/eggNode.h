// Filename: eggNode.h
// Created by:  drose (16Jan99)
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

#ifndef EGGNODE_H
#define EGGNODE_H

#include <pandabase.h>

#include "eggNamedObject.h"

#include <typedObject.h>
#include <lmatrix.h>
#include <pointerTo.h>
#include <referenceCount.h>

class EggGroupNode;
class EggRenderMode;
class EggTextureCollection;

////////////////////////////////////////////////////////////////////
//       Class : EggNode
// Description : A base class for things that may be directly added
//               into the egg hierarchy.  This includes groups,
//               joints, polygons, vertex pools, etc., but does not
//               include things like vertices.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAEGG EggNode : public EggNamedObject {
public:
  INLINE EggNode(const string &name = "");
  INLINE EggNode(const EggNode &copy);
  INLINE EggNode &operator = (const EggNode &copy);

  INLINE EggGroupNode *get_parent() const;
  INLINE int get_depth() const;
  INLINE bool is_under_instance() const;
  INLINE bool is_under_transform() const;
  INLINE bool is_local_coord() const;

  INLINE const LMatrix4d &get_vertex_frame() const;
  INLINE const LMatrix4d &get_node_frame() const;
  INLINE const LMatrix4d &get_vertex_frame_inv() const;
  INLINE const LMatrix4d &get_node_frame_inv() const;
  INLINE const LMatrix4d &get_vertex_to_node() const;

  INLINE void transform(const LMatrix4d &mat);
  INLINE void transform_vertices_only(const LMatrix4d &mat);
  INLINE void flatten_transforms();
  void apply_texmats();

  virtual EggRenderMode *determine_alpha_mode();
  virtual EggRenderMode *determine_depth_write_mode();
  virtual EggRenderMode *determine_depth_test_mode();
  virtual EggRenderMode *determine_draw_order();
  virtual EggRenderMode *determine_bin();

  virtual void write(ostream &out, int indent_level) const=0;
  bool parse_egg(const string &egg_syntax);

#ifndef NDEBUG
  void test_under_integrity() const;
#else
  void test_under_integrity() const { }
#endif  // NDEBUG


protected:
  enum UnderFlags {
    UF_under_instance  = 0x001,
    UF_under_transform = 0x002,
    UF_local_coord     = 0x004,
  };

  virtual bool egg_start_parse_body();

  virtual void update_under(int depth_offset);
  virtual void adjust_under();

  virtual void r_transform(const LMatrix4d &mat, const LMatrix4d &inv,
                           CoordinateSystem to_cs);
  virtual void r_transform_vertices(const LMatrix4d &mat);
  virtual void r_mark_coordsys(CoordinateSystem cs);
  virtual void r_flatten_transforms();
  virtual void r_apply_texmats(EggTextureCollection &textures);

  // These members are updated automatically by prepare_add_child(),
  // prepare_remove_child(), and update_under().  Other functions
  // shouldn't be fiddling with them.

  EggGroupNode *_parent;
  int _depth;
  int _under_flags;

  typedef RefCountObj<LMatrix4d> MatrixFrame;

  PT(MatrixFrame) _vertex_frame;
  PT(MatrixFrame) _node_frame;
  PT(MatrixFrame) _vertex_frame_inv;
  PT(MatrixFrame) _node_frame_inv;
  PT(MatrixFrame) _vertex_to_node;


public:

  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    EggNamedObject::init_type();
    register_type(_type_handle, "EggNode",
                  EggNamedObject::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;

friend class EggGroupNode;
};

#include "eggNode.I"

#endif
