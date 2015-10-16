// Filename: pta_LVecBase2.cxx
// Created by:  drose (27Feb10)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) Carnegie Mellon University.  All rights reserved.
//
// All use of this software is subject to the terms of the revised BSD
// license.  You should have received a copy of this license along
// with this source code in a file named "LICENSE."
//
////////////////////////////////////////////////////////////////////

#include "pta_LVecBase2.h"

// Tell GCC that we'll take care of the instantiation explicitly here.
#ifdef __GNUC__
#pragma implementation
#endif

template class PointerToBase<ReferenceCountedVector<LVecBase2f> >;
template class PointerToArrayBase<LVecBase2f>;
template class PointerToArray<LVecBase2f>;
template class ConstPointerToArray<LVecBase2f>;

template class PointerToBase<ReferenceCountedVector<LVecBase2d> >;
template class PointerToArrayBase<LVecBase2d>;
template class PointerToArray<LVecBase2d>;
template class ConstPointerToArray<LVecBase2d>;

template class PointerToBase<ReferenceCountedVector<LVecBase2i> >;
template class PointerToArrayBase<LVecBase2i>;
template class PointerToArray<LVecBase2i>;
template class ConstPointerToArray<LVecBase2i>;
