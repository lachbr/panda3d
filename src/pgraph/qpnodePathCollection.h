// Filename: qpnodePathCollection.h
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

#ifndef qpNODEPATHCOLLECTION_H
#define qpNODEPATHCOLLECTION_H

#include "pandabase.h"
#include "qpnodePath.h"
#include "pointerToArray.h"

////////////////////////////////////////////////////////////////////
//       Class : NodePathCollection
// Description : This is a set of zero or more NodePaths.  It's handy
//               for returning from functions that need to return
//               multiple NodePaths (for instance,
//               NodePaths::get_children).
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA qpNodePathCollection {
PUBLISHED:
  qpNodePathCollection();
  qpNodePathCollection(const qpNodePathCollection &copy);
  void operator = (const qpNodePathCollection &copy);
  INLINE ~qpNodePathCollection();

  void add_path(const qpNodePath &node_path);
  bool remove_path(const qpNodePath &node_path);
  void add_paths_from(const qpNodePathCollection &other);
  void remove_paths_from(const qpNodePathCollection &other);
  void remove_duplicate_paths();
  bool has_path(const qpNodePath &path) const;
  void clear();

  bool is_empty() const;
  int get_num_paths() const;
  qpNodePath get_path(int index) const;
  qpNodePath operator [] (int index) const;

  // Handy operations on many NodePaths at once.
  INLINE void ls() const;
  void ls(ostream &out, int indent_level = 0) const;

  //  qpNodePathCollection find_all_matches(const string &path) const;
  void reparent_to(const qpNodePath &other);
  void wrt_reparent_to(const qpNodePath &other);

  void show();
  void hide();
  void stash();
  void unstash();

  void output(ostream &out) const;
  void write(ostream &out, int indent_level = 0) const;

private:
  typedef PTA(qpNodePath) NodePaths;
  NodePaths _node_paths;
};

INLINE ostream &operator << (ostream &out, const qpNodePathCollection &col) {
  col.output(out);
  return out;
}

#include "qpnodePathCollection.I"

#endif


