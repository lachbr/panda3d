// Filename: buttonEventList.h
// Created by:  drose (12Mar02)
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

#ifndef BUTTONEVENTLIST_H
#define BUTTONEVENTLIST_H

#include "pandabase.h"

#include "buttonEvent.h"
#include "typedReferenceCount.h"
#include "pvector.h"

class ModifierButtons;

////////////////////////////////////////////////////////////////////
//       Class : ButtonEventList
// Description : Records a set of button events that happened
//               recently.  This class is usually used only in the
//               data graph, to transmit the recent button presses,
//               but it may be used anywhere a list of ButtonEvents
//               is desired.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA ButtonEventList : public TypedReferenceCount {
public:
  INLINE ButtonEventList();

  INLINE void add_event(ButtonEvent event);
  INLINE int get_num_events() const;
  INLINE const ButtonEvent &get_event(int n) const;
  INLINE void clear();

  void update_mods(ModifierButtons &mods) const;

  void output(ostream &out) const;
  void write(ostream &out, int indent_level = 0) const;

private:
  typedef pvector<ButtonEvent> Events;
  Events _events;
  
public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    TypedReferenceCount::init_type();
    register_type(_type_handle, "ButtonEventList",
                  TypedReferenceCount::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

INLINE ostream &operator << (ostream &out, const ButtonEventList &list) {
  list.output(out);
  return out;
}

#include "buttonEventList.I"

#endif

