// Filename: cullBinBackToFront.cxx
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

#include "cullBinBackToFront.h"
#include "graphicsStateGuardianBase.h"
#include "geometricBoundingVolume.h"
#include "cullableObject.h"
#include "cullHandler.h"
#include "pStatTimer.h"

#include <algorithm>


TypeHandle CullBinBackToFront::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: CullBinBackToFront::Destructor
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
CullBinBackToFront::
~CullBinBackToFront() {
  Objects::iterator oi;
  for (oi = _objects.begin(); oi != _objects.end(); ++oi) {
    CullableObject *object = (*oi)._object;
    delete object;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: CullBinBackToFront::make_bin
//       Access: Public, Static
//  Description: Factory constructor for passing to the CullBinManager.
////////////////////////////////////////////////////////////////////
CullBin *CullBinBackToFront::
make_bin(const string &name, GraphicsStateGuardianBase *gsg) {
  return new CullBinBackToFront(name, gsg);
}

////////////////////////////////////////////////////////////////////
//     Function: CullBinBackToFront::add_object
//       Access: Public, Virtual
//  Description: Adds a geom, along with its associated state, to
//               the bin for rendering.
////////////////////////////////////////////////////////////////////
void CullBinBackToFront::
add_object(CullableObject *object) {
  // Determine the center of the bounding volume.
  CPT(BoundingVolume) volume = object->_geom->get_bounds();

  if (!volume->is_empty()) {
    const GeometricBoundingVolume *gbv;
    DCAST_INTO_V(gbv, volume);
    
    LPoint3f center = gbv->get_approx_center();
    nassertv(object->_modelview_transform != (const TransformState *)NULL);
    center = center * object->_modelview_transform->get_mat();
    
    float distance = _gsg->compute_distance_to(center);
    _objects.push_back(ObjectData(object, distance));
  }
}

////////////////////////////////////////////////////////////////////
//     Function: CullBinBackToFront::finish_cull
//       Access: Public
//  Description: Called after all the geoms have been added, this
//               indicates that the cull process is finished for this
//               frame and gives the bins a chance to do any
//               post-processing (like sorting) before moving on to
//               draw.
////////////////////////////////////////////////////////////////////
void CullBinBackToFront::
finish_cull(SceneSetup *) {
  PStatTimer timer(_cull_this_pcollector);
  sort(_objects.begin(), _objects.end());
}

////////////////////////////////////////////////////////////////////
//     Function: CullBinBackToFront::draw
//       Access: Public
//  Description: Draws all the geoms in the bin, in the appropriate
//               order.
////////////////////////////////////////////////////////////////////
void CullBinBackToFront::
draw() {
  PStatTimer timer(_draw_this_pcollector);
  Objects::const_iterator oi;
  for (oi = _objects.begin(); oi != _objects.end(); ++oi) {
    CullableObject *object = (*oi)._object;
    CullHandler::draw(object, _gsg);
  }
}

