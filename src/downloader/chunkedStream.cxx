// Filename: chunkedStream.cxx
// Created by:  drose (25Sep02)
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

#include "chunkedStream.h"

// This module is not compiled if OpenSSL is not available.
#ifdef HAVE_SSL

////////////////////////////////////////////////////////////////////
//     Function: IChunkedStream::is_closed
//       Access: Public, Virtual
//  Description: Returns true if the last eof condition was triggered
//               because the socket has genuinely closed, or false if
//               we can expect more data to come along shortly.
////////////////////////////////////////////////////////////////////
INLINE bool IChunkedStream::
is_closed() {
  if (_buf._done || (*_buf._source)->is_closed()) {
    return true;
  }
  clear();
  return false;
}

#endif  // HAVE_SSL
