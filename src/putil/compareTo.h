// Filename: compareTo.h
// Created by:  drose (22Feb02)
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

#ifndef COMPARETO_H
#define COMPARETO_H

#include <pandabase.h>

////////////////////////////////////////////////////////////////////
//       Class : CompareTo
// Description : An STL function object class, this is intended to be
//               used on any ordered collection of classes that
//               contain a compare_to() method.  It defines the order
//               of the pointers via compare_to().
////////////////////////////////////////////////////////////////////
template<class ObjectType>
class CompareTo {
public:
  INLINE bool operator () (const ObjectType &a, const ObjectType &b) const;
};

#include "compareTo.I"

#endif

