// Filename: reversedNumericData.cxx
// Created by:  drose (09May01)
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

#include "reversedNumericData.h"

#include <notify.h>

////////////////////////////////////////////////////////////////////
//     Function: ReversedNumericData::reverse_assign
//       Access: Private
//  Description: Actually does the data reversal.
////////////////////////////////////////////////////////////////////
void ReversedNumericData::
reverse_assign(const char *source, size_t length) {
  nassertv((int)length <= max_numeric_size);
  for (size_t i = 0; i < length; i++) {
    _data[i] = source[length - 1 - i];
  }
}
