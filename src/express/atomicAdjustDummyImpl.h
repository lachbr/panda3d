// Filename: atomicAdjustDummyImpl.h
// Created by:  drose (09Aug02)
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

#ifndef ATOMICADJUSTDUMMYIMPL_H
#define ATOMICADJUSTDUMMYIMPL_H

#include "pandabase.h"
#include "selectIpcImpl.h"

#ifdef IPC_DUMMY_IMPL
#include "notify.h"

////////////////////////////////////////////////////////////////////
//       Class : AtomicAdjustDummyImpl
// Description : A trivial implementation for atomic adjustments for
//               systems that don't require multiprogramming, and
//               therefore don't require special atomic operations.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA AtomicAdjustDummyImpl {
public:
  INLINE static int inc(int &var);
  INLINE static int dec(int &var);
  INLINE static int set(int &var, int new_value);
};

#include "atomicAdjustDummyImpl.I"

#endif  // IPC_DUMMY_IMPL

#endif
