// Filename: dcast.cxx
// Created by:  drose (07Aug01)
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

#include "dcast.h"
#include "config_express.h"

#ifdef _WIN32
#include <windows.h>  // for IsBadWritePtr()
#endif


#ifndef NDEBUG
////////////////////////////////////////////////////////////////////
//     Function: _dcast_verify
//  Description: This function performs the actual check that the
//               indicated TypedObject pointer is of the intended
//               type.
////////////////////////////////////////////////////////////////////
bool
_dcast_verify(TypeHandle want_handle, size_t want_size, 
              const TypedObject *ptr) {
  if (get_verify_dcast()) {
    if ((ptr == (const TypedObject *)NULL)
#if defined(_DEBUG) && defined(_WIN32)
        || IsBadWritePtr(ptr, want_size)
#endif
        ) {
      express_cat->warning()
        << "Attempt to cast NULL or invalid pointer to " 
        << want_handle << "\n";
      return false;
    }
    if (!ptr->is_of_type(want_handle)) {
      express_cat->error()
        << "Attempt to cast pointer from " << ptr->get_type()
        << " to " << want_handle << "\n";
      if (ptr->get_type() == TypedObject::get_class_type()) {
        express_cat->error(false)
          << "Perhaps pointer was inadvertently deleted?\n";
      }
      return false;
    }
  }

  return true;
}
#endif  // NDEBUG

