// Filename: fltInfo.h
// Created by:  drose (05Sep01)
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

#ifndef FLTINFO_H
#define FLTINFO_H

#include "pandatoolbase.h"

#include "programBase.h"

class FltRecord;

////////////////////////////////////////////////////////////////////
//       Class : FltInfo
// Description : A program to read a flt file and report interesting
//               things about it.
////////////////////////////////////////////////////////////////////
class FltInfo : public ProgramBase {
public:
  FltInfo();

  void run();

protected:
  virtual bool handle_args(Args &args);

  void list_hierarchy(FltRecord *record, int indent_level);

  Filename _input_filename;
  bool _list_hierarchy;
};

#endif

