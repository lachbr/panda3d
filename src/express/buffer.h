// Filename: buffer.h
// Created by:  mike (09Jan97)
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
#ifndef BUFFER_H
#define BUFFER_H
//
////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////
#include <pandabase.h>
#include "typedef.h"
#include "referenceCount.h"

////////////////////////////////////////////////////////////////////
//       Class : Buffer
// Description :
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAEXPRESS Buffer : public ReferenceCount {
public:
  Buffer(int size);
  ~Buffer();

  INLINE int get_length(void) const;

#ifndef CPPPARSER
// hidden from interrogate
public:
  char *_buffer;
#endif

private:
  int _length;
};

////////////////////////////////////////////////////////////////////
//       Class : Ramfile
// Description :
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAEXPRESS Ramfile {
PUBLISHED:
  INLINE Ramfile(void);

public:
  string _data;
};

#include "buffer.I"

#endif
