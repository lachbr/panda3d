// Filename: directdClient.h
// Created by:  skyler 2002.04.08
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

#include "directd.h"

// Description: DirectDClient is a test app for DriectDServer.
class EXPCL_DIRECT DirectDClient: public DirectD {
public:
  DirectDClient();
  ~DirectDClient();

  void run_client(const string& host, int port);

protected:
  void cli_command(const string& cmd);
};

