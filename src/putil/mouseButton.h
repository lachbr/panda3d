// Filename: mouseButton.h
// Created by:  drose (01Mar00)
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

#ifndef MOUSEBUTTON_H
#define MOUSEBUTTON_H

#include <pandabase.h>

#include "buttonHandle.h"

////////////////////////////////////////////////////////////////////
//       Class : MouseButton
// Description : This class is just used as a convenient namespace for
//               grouping all of these handy functions that return
//               buttons which map to standard mouse buttons.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA MouseButton {
PUBLISHED:
  static ButtonHandle button(int button_number);
  static ButtonHandle one();
  static ButtonHandle two();
  static ButtonHandle three();

public:
  static void init_mouse_buttons();
};

#endif
