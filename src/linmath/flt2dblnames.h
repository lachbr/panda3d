// Filename: flt2dblnames.h
// Created by:  drose (04Apr01)
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


////////////////////////////////////////////////////////////////////
//
// This file is used particularly by lcast_to.h and lcast_to.cxx to
// define functions that convert from type float to type double.
//
////////////////////////////////////////////////////////////////////

#include "dblnames.h"

#undef FLOATTYPE2
#undef FLOATNAME2
#undef FLOATTOKEN2

#define FLOATTYPE2 float
#define FLOATNAME2(ARG) ARG##f
#define FLOATTOKEN2 'f'
