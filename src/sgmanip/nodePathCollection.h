// Filename: nodePathCollection.h
// Created by:  drose (06Mar00)
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

#ifndef NODEPATHCOLLECTION_H
#define NODEPATHCOLLECTION_H

#include <pandabase.h>

// We don't include NodePath in the header file, so NodePath can
// include us.
#include <arcChain.h>
#include <pointerToArray.h>

class NodePath;

////////////////////////////////////////////////////////////////////
//       Class : NodePathCollection
// Description : This is a set of zero or more NodePaths.  It's handy
//               for returning from functions that need to return
//               multiple NodePaths (for instance,
//               NodePaths::get_children).
//
//               All the NodePaths added to a NodePathCollection must
//               share the same graph_type.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA NodePathCollection {
PUBLISHED:
  NodePathCollection();
  NodePathCollection(const NodePathCollection &copy);
  void operator = (const NodePathCollection &copy);
  INLINE ~NodePathCollection();

  void add_path(const NodePath &node_path);
  bool remove_path(const NodePath &node_path);
  void add_paths_from(const NodePathCollection &other);
  void remove_paths_from(const NodePathCollection &other);
  void remove_duplicate_paths();
  bool has_path(const NodePath &path) const;
  void clear();

  bool is_empty() const;
  int get_num_paths() const;
  NodePath get_path(int index) const;
  NodePath operator [] (int index) const;
  TypeHandle get_graph_type() const;

  // Handy operations on many NodePaths at once.
  INLINE void ls() const;
  void ls(ostream &out, int indent_level = 0) const;

  NodePathCollection find_all_matches(const string &path) const;
  void reparent_to(const NodePath &other);
  void wrt_reparent_to(const NodePath &other);
  NodePathCollection instance_to(const NodePath &other) const;

  void show();
  void hide();
  void stash();
  void unstash();

  void output(ostream &out) const;
  void write(ostream &out, int indent_level = 0) const;

private:
  typedef PTA(ArcChain) NodePaths;
  NodePaths _node_paths;
  TypeHandle _graph_type;
};

INLINE ostream &operator << (ostream &out, const NodePathCollection &col) {
  col.output(out);
  return out;
}

#include "nodePathCollection.I"

#endif


