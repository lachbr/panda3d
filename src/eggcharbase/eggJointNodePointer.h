// Filename: eggJointNodePointer.h
// Created by:  drose (26Feb01)
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

#ifndef EGGJOINTNODEPOINTER_H
#define EGGJOINTNODEPOINTER_H

#include "pandatoolbase.h"

#include "eggJointPointer.h"

#include "eggGroup.h"
#include "pointerTo.h"

////////////////////////////////////////////////////////////////////
//       Class : EggJointNodePointer
// Description : This stores a pointer back to a <Joint> node.
////////////////////////////////////////////////////////////////////
class EggJointNodePointer : public EggJointPointer {
public:
  EggJointNodePointer(EggObject *object);

  virtual int get_num_frames() const;
  virtual LMatrix4d get_frame(int n) const;
  virtual void set_frame(int n, const LMatrix4d &mat);

  virtual void do_finish_reparent(EggJointPointer *new_parent);
  virtual void move_vertices_to(EggJointPointer *new_joint);

  virtual bool do_rebuild(EggCharacterDb &db);
  virtual void expose(EggGroup::DCSType dcs_type);

  virtual bool has_vertices() const;

  virtual EggJointPointer *make_new_joint(const string &name);

private:
  PT(EggGroup) _joint;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    EggJointPointer::init_type();
    register_type(_type_handle, "EggJointNodePointer",
                  EggJointPointer::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

#endif


