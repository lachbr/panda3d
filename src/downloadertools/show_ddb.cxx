// Filename: show_ddb.cxx
// Created by:  drose (02Nov02)
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

#include "pandabase.h"
#include "downloadDb.h"
#include "filename.h"

int
main(int argc, char *argv[]) {
  if (argc != 3) {
    cerr << "Usage: show_ddb server.ddb client.ddb\n";
    return 1;
  }

  Filename server_ddb = Filename::from_os_specific(argv[1]);
  Filename client_ddb = Filename::from_os_specific(argv[2]);

  DownloadDb db(server_ddb, client_ddb);
  db.output_version_map(cout);

  return 0;
}
