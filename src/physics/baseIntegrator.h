// Filename: baseIntegrator.h
// Created by:  charles (11Aug00)
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

#ifndef BASEINTEGRATOR_H
#define BASEINTEGRATOR_H

#include <pandabase.h>
#include <pointerTo.h>
#include <referenceCount.h>
#include <luse.h>

#include "linearForce.h"
#include "angularForce.h"

#include "pvector.h"

class Physical;

////////////////////////////////////////////////////////////////////
//       Class : BaseIntegrator
// Description : pure virtual integrator class that holds cached
//               matrix information that really should be common to
//               any possible child implementation.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAPHYSICS BaseIntegrator : public ReferenceCount {
private:
  // since the wrt for each physicsobject between its physicalnode
  // and however many forces will be the same among one physical,
  // the transformation matrices can be pulled out of the inner loop
  // and precomputed.
  pvector< LMatrix4f > _precomputed_linear_matrices;
  pvector< LMatrix4f > _precomputed_angular_matrices;

protected:
  BaseIntegrator(void);

  INLINE const pvector< LMatrix4f > &get_precomputed_linear_matrices(void) const;
  INLINE const pvector< LMatrix4f > &get_precomputed_angular_matrices(void) const;

  void precompute_linear_matrices(Physical *physical,
                                  const pvector< PT(LinearForce) > &forces);
  void precompute_angular_matrices(Physical *physical,
                                   const pvector< PT(AngularForce) > &forces);

public:
  virtual ~BaseIntegrator(void);
};

#include "baseIntegrator.I"

#endif // BASEINTEGRATOR_H
