// Filename: bioStreamBuf.cxx
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

#include "bioStreamBuf.h"
#include "config_downloader.h"

#ifdef HAVE_SSL

#ifdef REPORT_OPENSSL_ERRORS
#include <openssl/err.h>
#endif

#ifndef HAVE_STREAMSIZE
// Some compilers (notably SGI) don't define this for us
typedef int streamsize;
#endif /* HAVE_STREAMSIZE */

////////////////////////////////////////////////////////////////////
//     Function: BioStreamBuf::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
BioStreamBuf::
BioStreamBuf() {
  _is_closed = false;

#ifdef WIN32_VC
  // In spite of the claims of the MSDN Library to the contrary,
  // Windows doesn't seem to provide an allocate() function, so we'll
  // do it by hand.
  char *buf = new char[8192];
  char *ebuf = buf + 8192;
  char *mbuf = buf + 4096;
  setg(buf, mbuf, mbuf);
  setp(mbuf, ebuf);

#else
  allocate();
  // Chop the buffer in half.  The bottom half goes to the get buffer;
  // the top half goes to the put buffer.
  char *b = base();
  char *t = ebuf();
  char *m = b + (t - b) / 2;
  setg(b, m, m);
  setp(b, m);
#endif
}

////////////////////////////////////////////////////////////////////
//     Function: BioStreamBuf::Destructor
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
BioStreamBuf::
~BioStreamBuf() {
  close();
}

////////////////////////////////////////////////////////////////////
//     Function: BioStreamBuf::open
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
void BioStreamBuf::
open(BioPtr *source) {
  _source = source;
}

////////////////////////////////////////////////////////////////////
//     Function: BioStreamBuf::close
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
void BioStreamBuf::
close() {
  sync();
  _source.clear();
}

////////////////////////////////////////////////////////////////////
//     Function: BioStreamBuf::overflow
//       Access: Protected, Virtual
//  Description: Called by the system ostream implementation when its
//               internal buffer is filled, plus one character.
////////////////////////////////////////////////////////////////////
int BioStreamBuf::
overflow(int ch) {
  size_t n = pptr() - pbase();
  if (n != 0) {
    size_t num_wrote = write_chars(pbase(), n);
    pbump(-(int)n);
    if (num_wrote != n) {
      return EOF;
    }
  }

  if (ch != EOF) {
    // Store the next character back in the buffer.
    *pptr() = ch;
    pbump(1);
  }

  return 0;
}

////////////////////////////////////////////////////////////////////
//     Function: BioStreamBuf::sync
//       Access: Protected, Virtual
//  Description: Called by the system iostream implementation to
//               implement a flush operation.
////////////////////////////////////////////////////////////////////
int BioStreamBuf::
sync() {
  /*
  size_t n = egptr() - gptr();
  gbump(n);
  */

  size_t n = pptr() - pbase();
  size_t num_wrote = write_chars(pbase(), n);
  pbump(-(int)n);
  if (num_wrote != n) {
    return EOF;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////
//     Function: BioStreamBuf::underflow
//       Access: Protected, Virtual
//  Description: Called by the system istream implementation when its
//               internal buffer needs more characters.
////////////////////////////////////////////////////////////////////
int BioStreamBuf::
underflow() {
  // Sometimes underflow() is called even if the buffer is not empty.
  if (gptr() >= egptr()) {
    size_t buffer_size = egptr() - eback();
    gbump(-(int)buffer_size);

    size_t num_bytes = buffer_size;

    // BIO_read might return -1 or -2 on eof or error, so we have to
    // allow for negative numbers.
    int read_count = BIO_read(*_source, gptr(), buffer_size);

    if (read_count != (int)num_bytes) {
      // Oops, we didn't read what we thought we would.
      if (read_count <= 0) {
        _is_closed = !BIO_should_retry(*_source);
        if (_is_closed) {
          if (read_count == 0) {
            downloader_cat.info()
              << "Lost connection to "
              << _source->get_server_name() << ":" 
              << _source->get_port() << ".\n";
          } else {
            downloader_cat.info()
              << "Socket error detected on " 
              << _source->get_server_name() << ":" 
              << _source->get_port() << ", return code "
              << read_count << ".\n";
          }
#ifdef REPORT_OPENSSL_ERRORS
          ERR_print_errors_fp(stderr);
#endif
        }
        gbump(num_bytes);
        return EOF;
      }

      // Slide what we did read to the top of the buffer.
      nassertr(read_count < (int)num_bytes, EOF);
      size_t delta = (int)num_bytes - read_count;
      memmove(gptr() + delta, gptr(), read_count);
      gbump(delta);
    }
  }

  return (unsigned char)*gptr();
}

////////////////////////////////////////////////////////////////////
//     Function: BioStreamBuf::write_chars
//       Access: Private
//  Description: Sends some characters to the dest stream.  Does not
//               return until all characters are sent or the socket is
//               closed, even if the underlying BIO is non-blocking.
////////////////////////////////////////////////////////////////////
size_t BioStreamBuf::
write_chars(const char *start, size_t length) {
  size_t wrote_so_far = 0;

  int write_count = BIO_write(*_source, start, length);
  while (write_count != (int)(length - wrote_so_far)) {
    if (write_count <= 0) {
      _is_closed = !BIO_should_retry(*_source);
      if (_is_closed) {
        return wrote_so_far;
      }

      // Block on the underlying socket before we try to write some
      // more.
      int fd = -1;
      BIO_get_fd(*_source, &fd);
      if (fd < 0) {
        downloader_cat.warning()
          << "socket BIO has no file descriptor.\n";
      } else {
        downloader_cat.spam()
          << "waiting to write to BIO.\n";
        fd_set wset;
        FD_ZERO(&wset);
        FD_SET(fd, &wset);
        select(fd + 1, NULL, &wset, NULL, NULL);
      }        

    } else {
      // wrote some characters.
      wrote_so_far += write_count;
    }

    // Try to write some more.
    write_count = BIO_write(*_source, start + wrote_so_far, length - wrote_so_far);
  }

  return length;
}

#endif  // HAVE_SSL
