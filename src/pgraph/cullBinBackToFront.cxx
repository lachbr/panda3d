// Filename: cullBinBackToFront.cxx
// Created by:  drose (28Feb02)
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

#include "cullBinBackToFront.h"
#include "graphicsStateGuardianBase.h"
#include "geometricBoundingVolume.h"
#include "cullableObject.h"
#include "cullHandler.h"

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
//     Function: CullBinBackToFront::add_object
//       Access: Public, Virtual
//  Description: Adds a geom, along with its associated state, to
//               the bin for rendering.
////////////////////////////////////////////////////////////////////
void CullBinBackToFront::
add_object(CullableObject *object) {
  // Determine the center of the bounding volume.
  const BoundingVolume &volume = object->_geom->get_bound();

  if (!volume.is_empty() &&
      volume.is_of_type(GeometricBoundingVolume::get_class_type())) {
    const GeometricBoundingVolume *gbv;
    DCAST_INTO_V(gbv, &volume);
    
    LPoint3f center = gbv->get_approx_center();
    nassertv(object->_transform != (const TransformState *)NULL);
    center = center * object->_transform->get_mat();
    
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
finish_cull() {
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
  Objects::const_iterator oi;
  for (oi = _objects.begin(); oi != _objects.end(); ++oi) {
    CullableObject *object = (*oi)._object;
    CullHandler::draw(object, _gsg);
  }
}

