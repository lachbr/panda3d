// Filename: check_crc.cxx
// Created by:  
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

#include <download_utils.h>

int
main(int argc, char *argv[]) {
  if (argc < 2) {
    cerr << "Usage: check_crc <file>" << endl;
    return 0;
  }

  Filename source_file = argv[1];

  cout << check_crc(source_file);

  return 1;
}
