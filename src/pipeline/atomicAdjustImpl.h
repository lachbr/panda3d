// Filename: atomicAdjustImpl.h
// Created by:  drose (09Aug02)
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

#ifndef ATOMICADJUSTIMPL_H
#define ATOMICADJUSTIMPL_H

#include "pandabase.h"
#include "selectThreadImpl.h"

#if defined(THREAD_DUMMY_IMPL)

#include "atomicAdjustDummyImpl.h"
typedef AtomicAdjustDummyImpl AtomicAdjustImpl;

#elif defined(THREAD_WIN32_IMPL)

#include "atomicAdjustWin32Impl.h"
typedef AtomicAdjustWin32Impl AtomicAdjustImpl;

#elif defined(THREAD_POSIX_IMPL)

#include "atomicAdjustPosixImpl.h"
typedef AtomicAdjustPosixImpl AtomicAdjustImpl;

#elif defined(THREAD_NSPR_IMPL)

#include "atomicAdjustNsprImpl.h"
typedef AtomicAdjustNsprImpl AtomicAdjustImpl;

#endif

#endif
