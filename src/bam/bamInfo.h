// Filename: bamInfo.h
// Created by:  drose (02Jul00)
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

#ifndef BAMINFO_H
#define BAMINFO_H

#include <pandatoolbase.h>

#include <programBase.h>
#include <filename.h>
#include <pt_Node.h>
#include <sceneGraphAnalyzer.h>

#include <vector>

class TypedWritable;

////////////////////////////////////////////////////////////////////
//       Class : BamInfo
// Description :
////////////////////////////////////////////////////////////////////
class BamInfo : public ProgramBase {
public:
  BamInfo();

  void run();

protected:
  virtual bool handle_args(Args &args);

private:
  bool get_info(const Filename &filename);
  void describe_scene_graph(Node *node);
  void describe_general_object(TypedWritable *object);
  void list_hierarchy(Node *node, int indent_level);

  typedef vector<Filename> Filenames;
  Filenames _filenames;

  bool _ls;
  bool _verbose_transitions;
  bool _verbose_geoms;

  int _num_scene_graphs;
  SceneGraphAnalyzer _analyzer;
};

#endif
