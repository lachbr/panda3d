// Filename: xFileToEgg.cxx
// Created by:  drose (21Jun01)
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

#include "xFileToEgg.h"
#include "xFileToEggConverter.h"

////////////////////////////////////////////////////////////////////
//     Function: XFileToEgg::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
XFileToEgg::
XFileToEgg() :
  SomethingToEgg("DirectX", ".x")
{
  add_normals_options();
  add_transform_options();
  add_texture_path_options();
  add_rel_dir_options();
  add_search_path_options(true);

  set_program_description
    ("This program converts DirectX retained-mode (.x) files to egg.  This "
     "is a simple file format that only supports basic polygons, materials, "
     "and textures, in a hierarchy.");

  redescribe_option
    ("cs",
     "Specify the coordinate system of the input " + _format_name +
     " file.  Normally, this is y-up-left.");

  _coordinate_system = CS_yup_left;
}

////////////////////////////////////////////////////////////////////
//     Function: XFileToEgg::run
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
void XFileToEgg::
run() {
  _data.set_coordinate_system(_coordinate_system);

  XFileToEggConverter converter;
  converter.set_egg_data(&_data, false);
  converter.set_texture_path_convert(_texture_path_convert, _make_rel_dir);

  if (!converter.convert_file(_input_filename)) {
    nout << "Unable to read " << _input_filename << "\n";
    exit(1);
  }

  write_egg_file();
  nout << "\n";
}


int main(int argc, char *argv[]) {
  XFileToEgg prog;
  prog.parse_command_line(argc, argv);
  prog.run();
  return 0;
}
