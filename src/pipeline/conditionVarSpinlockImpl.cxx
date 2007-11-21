// Filename: conditionVarSpinlockImpl.cxx
// Created by:  drose (11Apr06)
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

#include "selectThreadImpl.h"

#ifdef MUTEX_SPINLOCK

#include "conditionVarSpinlockImpl.h"

////////////////////////////////////////////////////////////////////
//     Function: ConditionVarSpinlockImpl::wait
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
void ConditionVarSpinlockImpl::
wait() {
  AtomicAdjust::Integer current = _event;
  _mutex.release();

  while (AtomicAdjust::get(_event) == current) {
  }

  _mutex.lock();
}

#endif  // MUTEX_SPINLOCK
