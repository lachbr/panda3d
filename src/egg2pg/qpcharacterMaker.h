// Filename: qpcharacterMaker.h
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

#ifndef qpCHARACTERMAKER_H
#define qpCHARACTERMAKER_H

#include "pandabase.h"

#include "computedVerticesMaker.h"

#include "vector_PartGroupStar.h"
#include "typedef.h"
#include "pmap.h"

class EggNode;
class EggGroup;
class EggGroupNode;
class EggPrimitive;
class PartGroup;
class CharacterJointBundle;
class Character;
class CharacterSlider;
class MovingPartBase;
class NamedNode;
class EggLoaderBase;

///////////////////////////////////////////////////////////////////
//       Class : qpCharacterMaker
// Description : Converts an EggGroup hierarchy, beginning with a
//               group with <Dart> set, to a character node with
//               joints.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAEGG qpCharacterMaker {
public:
  qpCharacterMaker(EggGroup *root, EggLoaderBase &loader);

  qpCharacter *make_node();

  PartGroup *egg_to_part(EggNode *egg_node) const;
  int egg_to_index(EggNode *egg_node) const;
  PandaNode *part_to_node(PartGroup *part) const;

  int create_slider(const string &name);

private:
  CharacterJointBundle *make_bundle();
  void build_joint_hierarchy(EggNode *egg_node, PartGroup *part);
  void parent_joint_nodes(PartGroup *part);

  void make_geometry(EggNode *egg_node);

  void make_static_primitive(EggPrimitive *egg_primitive,
                             EggGroupNode *prim_home);
  void make_dynamic_primitive(EggPrimitive *egg_primitive,
                              EggGroupNode *prim_home);
  EggGroupNode *determine_primitive_home(EggPrimitive *egg_primitive);

  typedef pmap<EggNode *, int> NodeMap;
  NodeMap _node_map;

  typedef vector_PartGroupStar Parts;
  Parts _parts;

  EggLoaderBase &_loader;
  EggGroup *_egg_root;
  qpCharacter *_character_node;
  CharacterJointBundle *_bundle;
  ComputedVerticesMaker _comp_verts_maker;
  PartGroup *_morph_root;
  PartGroup *_skeleton_root;
};

#endif
