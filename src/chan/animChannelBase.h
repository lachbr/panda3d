// Filename: animChannelBase.h
// Created by:  drose (19Feb99)
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

#ifndef ANIMCHANNELBASE_H
#define ANIMCHANNELBASE_H

#include "pandabase.h"

#include "animGroup.h"
#include "animControl.h"

#include "pointerTo.h"

////////////////////////////////////////////////////////////////////
//       Class : AnimChannelBase
// Description : Parent class for all animation channels.  An
//               AnimChannel is an arbitrary function that changes
//               over time (actually, over frames), usually defined by
//               a table read from an egg file (but possibly computed
//               or generated in any other way).
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA_CHAN AnimChannelBase : public AnimGroup {
protected:
  // The default constructor is protected: don't try to create an
  // AnimChannel without a parent.  To create an AnimChannel hierarchy,
  // you must first create an AnimBundle, and use that to create any
  // subsequent children.
  INLINE AnimChannelBase(const string &name = "");
  INLINE AnimChannelBase(AnimGroup *parent, const AnimChannelBase &copy);

public:
  INLINE AnimChannelBase(AnimGroup *parent, const string &name);

  virtual bool has_changed(int last_frame, double last_frac, 
                           int this_frame, double this_frac);

  virtual TypeHandle get_value_type() const=0;

protected:

  int _last_frame;

public:
  virtual void write_datagram(BamWriter* manager, Datagram &me);

protected:
  void fillin(DatagramIterator& scan, BamReader* manager);

PUBLISHED:
  virtual TypeHandle get_type() const {
    return get_class_type();
  }

public:
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    AnimGroup::init_type();
    register_type(_type_handle, "AnimChannelBase",
                  AnimGroup::get_class_type());
  }

private:
  static TypeHandle _type_handle;
};

#include "animChannelBase.I"

#endif
