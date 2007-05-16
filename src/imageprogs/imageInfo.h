// Filename: imageInfo.h
// Created by:  drose (13Mar03)
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

#ifndef IMAGEINFO_H
#define IMAGEINFO_H

#include "pandatoolbase.h"

#include "programBase.h"

////////////////////////////////////////////////////////////////////
//       Class : ImageInfo
// Description : This program reads the headers of a series of one or
//               more images and reports their sizes to standard
//               output.
////////////////////////////////////////////////////////////////////
class ImageInfo : public ProgramBase {
public:
  ImageInfo();

  void run();

protected:
  virtual bool handle_args(Args &args);

private:
  bool is_power_2(int value) const;

  Args _filenames;
  bool _report_power_2;
};

#endif

