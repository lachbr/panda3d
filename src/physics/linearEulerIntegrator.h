// Filename: linearEulerIntegrator.h
// Created by:  charles (13Jun00)
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

#ifndef LINEAREULERINTEGRATOR_H
#define LINEAREULERINTEGRATOR_H

#include "linearIntegrator.h"

////////////////////////////////////////////////////////////////////
//       Class : LINEAREulerIntegrator
// Description : Performs Euler integration on a vector of
//               physically modelable objects given a quantum dt.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAPHYSICS LinearEulerIntegrator : public LinearIntegrator {
private:
  virtual void child_integrate(Physical *physical,
                               pvector< PT(LinearForce) >& forces,
                               float dt);

PUBLISHED:
  LinearEulerIntegrator(void);
  virtual ~LinearEulerIntegrator(void);
};

#endif // EULERINTEGRATOR_H
