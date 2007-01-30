// Filename: ode_includes.h
// Created by:  joswilso (30Jan07)
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

#ifndef _ODE_INCLUDES_H_
#define _ODE_INCLUDES_H_
 
#ifdef int8
  #define temp_ode_int8 int8
#endif

#ifdef int32
  #define temp_ode_int32 int32
#endif

#ifdef uint32
  #define temp_ode_uint32 uint32
#endif


#define int8 ode_int8
#define int32 ode_int32
#define uint32 ode_uint32

#include "ode/ode.h"

#undef int8
#undef int32
#undef uint32

#ifdef temp_ode_int8
  #define int8 temp_ode_int8
  #undef temp_ode_int8
#endif

#ifdef temp_ode_int32
  #define int32 temp_ode_int32
  #undef temp_ode_int32
#endif

#ifdef temp_ode_uint32
  #define uint32 temp_ode_uint32
  #undef temp_ode_uint32
#endif


#endif
