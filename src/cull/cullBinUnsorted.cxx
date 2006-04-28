// Filename: cullBinUnsorted.cxx
// Created by:  drose (28Feb02)
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

#include "cullBinUnsorted.h"
#include "cullHandler.h"
#include "graphicsStateGuardianBase.h"
#include "pStatTimer.h"


TypeHandle CullBinUnsorted::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: CullBinUnsorted::Destructor
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
CullBinUnsorted::
~CullBinUnsorted() {
  Objects::iterator oi;
  for (oi = _objects.begin(); oi != _objects.end(); ++oi) {
    CullableObject *object = (*oi);
    delete object;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: CullBinUnsorted::make_bin
//       Access: Public, Static
//  Description: Factory constructor for passing to the CullBinManager.
////////////////////////////////////////////////////////////////////
CullBin *CullBinUnsorted::
make_bin(const string &name, GraphicsStateGuardianBase *gsg,
         const PStatCollector &draw_region_pcollector) {
  return new CullBinUnsorted(name, gsg, draw_region_pcollector);
}

////////////////////////////////////////////////////////////////////
//     Function: CullBinUnsorted::add_object
//       Access: Public, Virtual
//  Description: Adds a geom, along with its associated state, to
//               the bin for rendering.
////////////////////////////////////////////////////////////////////
void CullBinUnsorted::
add_object(CullableObject *object, Thread *current_thread) {
  _objects.push_back(object);
}

////////////////////////////////////////////////////////////////////
//     Function: CullBinUnsorted::draw
//       Access: Public, Virtual
//  Description: Draws all the objects in the bin, in the appropriate
//               order.
////////////////////////////////////////////////////////////////////
void CullBinUnsorted::
draw(Thread *current_thread) {
  PStatTimer timer(_draw_this_pcollector, current_thread);
  Objects::iterator oi;
  for (oi = _objects.begin(); oi != _objects.end(); ++oi) {
    CullableObject *object = (*oi);
    CullHandler::draw(object, _gsg, current_thread);
  }
}

