// Filename: chunkedStream.h
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

#ifndef CHUNKEDSTREAM_H
#define CHUNKEDSTREAM_H

#include "pandabase.h"

// This module is not compiled if OpenSSL is not available.
#ifdef HAVE_SSL

#include "socketStream.h"
#include "chunkedStreamBuf.h"

class HTTPChannel;
class BioStreamPtr;

////////////////////////////////////////////////////////////////////
//       Class : IChunkedStream
// Description : An input stream object that reads data from a source
//               istream, but automatically decodes the "chunked"
//               transfer-coding specified by an HTTP server.
//
//               Seeking is not supported.
////////////////////////////////////////////////////////////////////
// No need to export from DLL.
class IChunkedStream : public ISocketStream {
public:
  INLINE IChunkedStream();
  INLINE IChunkedStream(BioStreamPtr *source, HTTPChannel *doc);

  INLINE IChunkedStream &open(BioStreamPtr *source, HTTPChannel *doc);
  INLINE IChunkedStream &close();

  virtual bool is_closed();

private:
  ChunkedStreamBuf _buf;
};

#include "chunkedStream.I"

#endif  // HAVE_SSL

#endif



