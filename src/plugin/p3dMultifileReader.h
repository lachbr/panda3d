// Filename: p3dMultifileReader.h
// Created by:  drose (15Jun09)
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

#ifndef P3DMULTIFILEREADER_H
#define P3DMULTIFILEREADER_H

#include "p3d_plugin_common.h"

class P3DPackage;

////////////////////////////////////////////////////////////////////
//       Class : P3DMultifileReader
// Description : A way-simple implementation of Panda's multifile
//               reader.  See panda/src/express/multifile.cxx for a
//               full description of the binary format.  This
//               implementation doesn't support per-subfile
//               compression or encryption.
////////////////////////////////////////////////////////////////////
class P3DMultifileReader {
public:
  P3DMultifileReader();

  bool extract_all(const string &pathname, const string &to_dir,
                   P3DPackage *package, double start_progress, 
                   double progress_size);

  bool extract_one(const string &pathname, ostream &out, 
                   const string &filename);

private:
  class Subfile;

  bool read_header(const string &pathname);
  bool read_index();
  bool extract_subfile(ostream &out, const Subfile &s);

  inline unsigned int read_uint16();
  inline unsigned int read_uint32();

  ifstream _in;

  class Subfile {
  public:
    string _filename;
    size_t _start;
    size_t _length;
    size_t _timestamp;
  };

  typedef vector<Subfile> Subfiles;
  Subfiles _subfiles;

  static const char _header[];
  static const size_t _header_size;
  static const int _current_major_ver;
  static const int _current_minor_ver;
};

#include "p3dMultifileReader.I"

#endif
