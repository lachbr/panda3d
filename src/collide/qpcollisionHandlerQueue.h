// Filename: qpcollisionHandlerQueue.h
// Created by:  drose (16Mar02)
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

#ifndef qpCOLLISIONHANDLERQUEUE_H
#define qpCOLLISIONHANDLERQUEUE_H

#include <pandabase.h>

#include "qpcollisionHandler.h"
#include "qpcollisionEntry.h"

///////////////////////////////////////////////////////////////////
//       Class : qpCollisionHandlerQueue
// Description : A special kind of qpCollisionHandler that does nothing
//               except remember the CollisionEntries detected the
//               last pass.  This set of CollisionEntries may then be
//               queried by the calling function.  It's primarily
//               useful when a simple intersection test is being made,
//               e.g. for picking from the window.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA qpCollisionHandlerQueue : public qpCollisionHandler {
PUBLISHED:
  qpCollisionHandlerQueue();

public:
  virtual void begin_group();
  virtual void add_entry(qpCollisionEntry *entry);

PUBLISHED:
  void sort_entries();
  void clear_entries();

  int get_num_entries() const;
  qpCollisionEntry *get_entry(int n) const;

private:
  typedef pvector< PT(qpCollisionEntry) > Entries;
  Entries _entries;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    qpCollisionHandler::init_type();
    register_type(_type_handle, "qpCollisionHandlerQueue",
                  qpCollisionHandler::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

#endif



