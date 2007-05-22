// Filename: datagramInputFile.h
// Created by:  drose (30Oct00)
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

#ifndef DATAGRAMINPUTFILE_H
#define DATAGRAMINPUTFILE_H

#include "pandabase.h"

#include "datagramGenerator.h"
#include "filename.h"
#include "virtualFile.h"

////////////////////////////////////////////////////////////////////
//       Class : DatagramInputFile
// Description : This class can be used to read a binary file that
//               consists of an arbitrary header followed by a number
//               of datagrams.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA DatagramInputFile : public DatagramGenerator {
public:
  INLINE DatagramInputFile();

  bool open(Filename filename);
  bool open(istream &in);

  void close();

  bool read_header(string &header, size_t num_bytes);
  virtual bool get_datagram(Datagram &data);
  virtual bool is_eof();
  virtual bool is_error();

  virtual VirtualFile *get_file();
  virtual streampos get_file_pos();

private:
  bool _read_first_datagram;
  bool _error;
  PT(VirtualFile) _vfile;
  ifstream _in_file;
  istream *_in;
  bool _owns_in;
};

#include "datagramInputFile.I"

#endif
