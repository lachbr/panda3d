// Filename: compose_matrix.h
// Created by:  drose (27Jan99)
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

#ifndef COMPOSE_MATRIX_H
#define COMPOSE_MATRIX_H

////////////////////////////////////////////////////////////////////
//
// compose_matrix(), decompose_matrix()
//
// These two functions build and/or extract an affine matrix into
// its constituent parts: scale, hpr, and translate.
//
// There are also two additional flavors for 3x3 matrices.  These are
// treated as the upper 3x3 part of a general 4x4 matrix, and so can
// only represent rotations and scales.
//
////////////////////////////////////////////////////////////////////

#include <pandabase.h>
#include <math.h>
#include "lmatrix.h"
#include "lvector3.h"
#include "lvector2.h"
#include "lpoint3.h"
#include "lpoint2.h"
#include "lmat_ops.h"
#include "lvec2_ops.h"
#include "lvec3_ops.h"

#include "fltnames.h"
#include "compose_matrix_src.h"

#include "dblnames.h"
#include "compose_matrix_src.h"

#endif

