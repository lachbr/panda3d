// Filename: lightLensNode.h
// Created by:  drose (26Mar02)
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

#ifndef LIGHTLENSNODE_H
#define LIGHTLENSNODE_H

#include "pandabase.h"

#include "light.h"
#include "qplensNode.h"

////////////////////////////////////////////////////////////////////
//       Class : LightLensNode
// Description : A derivative of Light and of LensNode.  
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA LightLensNode : public Light, public qpLensNode {
PUBLISHED:
  LightLensNode(const string &name);

protected:
  LightLensNode(const LightLensNode &copy);

public:
  virtual PandaNode *as_node();
  virtual Light *as_light();

PUBLISHED:
  // We have to explicitly publish these because they resolve the
  // multiple inheritance.
  virtual void output(ostream &out) const;
  virtual void write(ostream &out, int indent_level = 0) const;

public:
  virtual void write_datagram(BamWriter *manager, Datagram &dg);

protected:
  void fillin(DatagramIterator &scan, BamReader *manager);

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    Light::init_type();
    qpLensNode::init_type();
    register_type(_type_handle, "LightLensNode",
                  Light::get_class_type(),
                  qpLensNode::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

INLINE ostream &operator << (ostream &out, const LightLensNode &light) {
  light.output(out);
  return out;
}

#include "lightLensNode.I"

#endif
