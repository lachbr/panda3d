// Filename: eggTrans.cxx
// Created by:  drose (14Feb00)
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

#include "eggTrans.h"

#include <eggGroupUniquifier.h>

////////////////////////////////////////////////////////////////////
//     Function: EggTrans::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
EggTrans::
EggTrans() {
  add_normals_options();
  add_transform_options();
  add_texture_options();

  set_program_description
    ("egg-trans reads an egg file and writes an essentially equivalent "
     "egg file to the standard output, or to the file specified with -o.  "
     "Some simple operations on the egg file are supported.");

  add_option
    ("F", "", 0,
     "Flatten out transforms.",
     &EggTrans::dispatch_none, &_flatten_transforms);

  add_option
    ("t", "", 0,
     "Apply texture matrices to UV's.",
     &EggTrans::dispatch_none, &_apply_texmats);

  add_option
    ("T", "", 0,
     "Collapse equivalent texture references.",
     &EggTrans::dispatch_none, &_collapse_equivalent_textures);

  add_option
    ("c", "", 0,
     "Clean out degenerate polygons and unused vertices.",
     &EggTrans::dispatch_none, &_remove_invalid_primitives);

  add_option
    ("C", "", 0,
     "Clean out higher-order polygons by subdividing into triangles.",
     &EggTrans::dispatch_none, &_triangulate_polygons);

  add_option
    ("N", "", 0,
     "Standardize and uniquify group names.",
     &EggTrans::dispatch_none, &_standardize_names);

}

////////////////////////////////////////////////////////////////////
//     Function: EggTrans::run
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
void EggTrans::
run() {
  if (_remove_invalid_primitives) {
    nout << "Removing invalid primitives.\n";
    int num_removed = _data.remove_invalid_primitives();
    nout << "  (" << num_removed << " removed.)\n";
    _data.remove_unused_vertices();
  }

  if (_triangulate_polygons) {
    nout << "Triangulating polygons.\n";
    int num_produced = _data.triangulate_polygons(true);
    nout << "  (" << num_produced << " triangles produced.)\n";
  }

  if (_apply_texmats) {
    nout << "Applying texture matrices.\n";
    _data.apply_texmats();
    _data.remove_unused_vertices();
  }

  if (_collapse_equivalent_textures) {
    nout << "Collapsing equivalent textures.\n";
    int num_removed = _data.collapse_equivalent_textures();
    nout << "  (" << num_removed << " removed.)\n";
  }

  if (_flatten_transforms) {
    nout << "Flattening transforms.\n";
    _data.flatten_transforms();
    _data.remove_unused_vertices();
  }

  if (_standardize_names) {
    nout << "Standardizing group names.\n";
    EggGroupUniquifier uniquifier;
    uniquifier.uniquify(&_data);
  }

  write_egg_file();
}


int main(int argc, char *argv[]) {
  EggTrans prog;
  prog.parse_command_line(argc, argv);
  prog.run();
  return 0;
}
