// Filename: eggPalettize.cxx
// Created by:  drose (28Nov00)
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

#include "eggPalettize.h"
#include "palettizer.h"
#include "eggFile.h"
#include "pal_string_utils.h"
#include "filenameUnifier.h"

#include <eggData.h>
#include <bamFile.h>
#include <notify.h>
#include <notifyCategory.h>
#include <notifySeverity.h>

#include <stdio.h>

////////////////////////////////////////////////////////////////////
//     Function: EggPalettize::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
EggPalettize::
EggPalettize() : EggMultiFilter(true) {
  set_program_description
    ("egg-palettize attempts to pack several texture maps from various models "
     "together into one or more palette images, for improved rendering performance "
     "and ease of texture management.  It can also resize textures and convert "
     "them to another image file format, whether or not they are actually "
     "placed on a palette, and can manage some "
     "simple texture properties, like mipmapping and rendering "
     "format.\n\n"

     "egg-palettize reads a texture attributes file, usually named "
     "textures.txa, which contains instructions from the user about "
     "resizing particular textures.  Type egg-palettize -H for an "
     "introduction to the syntax of this file.\n\n"

     "The palettization information from previous runs is recorded in a file "
     "named textures.boo (assuming the attributes file is named "
     "textures.txa); a complete record of every egg file and every texture "
     "that has been referenced is kept here.  This allows the program "
     "to intelligently manage the multiple egg files that may reference "
     "the textures in question.");


  clear_runlines();
  add_runline("[opts] attribfile.txa file.egg [file.egg ...]");

  // We always have EggMultiBase's -f on: force complete load.  In
  // fact, we use -f for our own purposes, below.
  remove_option("f");
  _force_complete = true;

  add_option
    ("a", "filename", 0,
     "Read the indicated file as the .txa file.  The default is textures.txa.",
     &EggPalettize::dispatch_filename, &_got_txa_filename, &_txa_filename);

  add_option
    ("pi", "", 0,
     "Do not process anything, but instead report the detailed palettization "
     "information.",
     &EggPalettize::dispatch_none, &_report_pi);

  add_option
    ("s", "", 0,
     "Do not process anything, but report statistics on palette "
     "and texture utilization.",
     &EggPalettize::dispatch_none, &_report_statistics);

  add_option
    ("R", "", 0,
     "Remove the named egg files from the previously-generated state data "
     "in textures.boo.",
     &EggPalettize::dispatch_none, &_remove_eggs);

  // We redefine -d using add_option() instead of redescribe_option()
  // so it gets listed along with these other options that relate.
  add_option
    ("d", "dirname", 0,
     "The directory in which to write the palettized egg files.  This is "
     "only necessary if more than one egg file is processed at the same "
     "time; if it is included, each egg file will be processed and written "
     "into the indicated directory.",
     &EggPalettize::dispatch_filename, &_got_output_dirname, &_output_dirname);
  add_option
    ("dm", "dirname", 0,
     "The directory in which to place all maps: generated palettes, "
     "as well as images which were not placed on palettes "
     "(but may have been resized).  If this contains the string %g, "
     "this will be replaced with the 'dir' string associated with a "
     "palette group; see egg-palettize -H.",
     &EggPalettize::dispatch_string, &_got_map_dirname, &_map_dirname);
  add_option
    ("ds", "dirname", 0,
     "The directory to write palette shadow images to.  These are working "
     "copies of the palette images, useful when the palette image type is "
     "a lossy-compression type like JPEG; you can avoid generational loss "
     "of quality on the palette images with each pass through the palettes "
     "by storing these extra shadow images in a lossless image type.  This "
     "directory is only used if the :shadowtype keyword appears in the .txa "
     "file.",
     &EggPalettize::dispatch_filename, &_got_shadow_dirname, &_shadow_dirname);
  add_option
    ("dr", "dirname", 0,
     "The directory to make map filenames relative to when writing egg "
     "files.  If specified, this should be an initial substring of -dm.",
     &EggPalettize::dispatch_filename, &_got_rel_dirname, &_rel_dirname);
  add_option
    ("g", "group", 0,
     "The default palette group that egg files will be assigned to if they "
     "are not explicitly assigned to any other group.",
     &EggPalettize::dispatch_string, &_got_default_groupname, &_default_groupname);
  add_option
    ("gdir", "name", 0,
     "The \"dir\" string to associate with the default palette group "
     "specified with -g, if no other dir name is given in the .txa file.",
     &EggPalettize::dispatch_string, &_got_default_groupdir, &_default_groupdir);

  add_option
    ("all", "", 0,
     "Consider all the textures referenced in all egg files that have "
     "ever been palettized, not just the egg files that appear on "
     "the command line.",
     &EggPalettize::dispatch_none, &_all_textures);
  add_option
    ("egg", "", 0,
     "Regenerate all egg files that need modification, even those that "
     "aren't named on the command line.",
     &EggPalettize::dispatch_none, &_redo_eggs);
  add_option
    ("redo", "", 0,
     "Force a regeneration of each image from its original source(s).  "
     "When used in conjunction with -egg, this also forces each egg file to "
     "be regenerated.",
     &EggPalettize::dispatch_none, &_redo_all);
  add_option
    ("opt", "", 0,
     "Force an optimal packing.  By default, textures are added to "
     "existing palettes without disturbing them, which can lead to "
     "suboptimal packing.  Including this switch forces the palettes "
     "to be rebuilt if necessary to optimize the packing, but this "
     "may invalidate other egg files which share this palette.",
     &EggPalettize::dispatch_none, &_optimal);

  // This isn't even implement yet.  Presently, we never lock anyway.
  // Dangerous, but hard to implement reliable file locking across
  // NFS/Samba and between multiple OS's.
  /*
  add_option
    ("nolock", "", 0,
     "Don't attempt to grab a file lock on the .txa file.  Use "
     "with extreme caution, as multiple processes running on the same "
     ".txa file may overwrite each other.  Use this only if the lock "
     "cannot be achieved for some reason.",
     &EggPalettize::dispatch_none, &_dont_lock_txa);
  */

  add_option
    ("H", "", 0,
     "Describe the syntax of the attributes file.",
     &EggPalettize::dispatch_none, &_describe_input_file);

  _txa_filename = "textures.txa";
}


////////////////////////////////////////////////////////////////////
//     Function: EggPalettize::handle_args
//       Access: Protected, Virtual
//  Description: Does something with the additional arguments on the
//               command line (after all the -options have been
//               parsed).  Returns true if the arguments are good,
//               false otherwise.
////////////////////////////////////////////////////////////////////
bool EggPalettize::
handle_args(ProgramBase::Args &args) {
  if (_describe_input_file) {
    describe_input_file();
    exit(1);
  }

  if (_remove_eggs) {
    // If we're removing these egg files from the database, we don't
    // want to try to load them up.  Instead, just save the filenames.
    _remove_egg_list = args;
    return true;
  }

  // Otherwise, load the named egg files up normally.
  return EggMultiFilter::handle_args(args);
}

////////////////////////////////////////////////////////////////////
//     Function: EggPalettize::describe_input_file
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
void EggPalettize::
describe_input_file() {
  nout <<
    "An attributes file consists mostly of lines describing desired sizes of "
    "texture maps.  The format resembles, but is not identical to, that of "
    "the qtess input file.  Examples:\n\n"

    "  texturename.rgb : 64 64\n"
    "  texture-a.rgb texture-b.rgb : 32 16 margin 2\n"
    "  *.rgb : 50% cont\n"
    "  eyelids.rgb : 16 16 omit\n\n"

    "In general, each line consists of one or more filenames (and can "
    "contain shell globbing characters like '*' or '?'), and a colon "
    "followed by a size request.  For each texture appearing in an egg "
    "file, the input list is scanned from the beginning and the first "
    "line that matches the filename defines the size of the texture, as "
    "well as other properties associated with the texture.\n\n"

    "A size request is most often a pair of numbers, giving a specific x y "
    "size of the texture.  A third number may also be supplied, giving a "
    "specific number of channels to convert to (for instance, to force an "
    "image to a 64x64 grayscale image, set its size to 64 64 1).  "
    "Alternatively, a percentage scaling may be specified, e.g. 30%.  The "
    "requested size need not be a power of 2.\n\n"

    "Other valid keywords that may be specified on the same line with the "
    "texture are:\n\n";

  show_text("  omit", 10,
            "This indicates that the texture should not be placed on any "
            "palette image.  It may still be resized, and it will in any "
            "case be copied into the install directory.\n\n");

  show_text("  margin i", 10,
            "This specifies the number of pixels that should be written "
            "around the border of the texture when it is placed in a "
            "palette image; i is the integer number of pixels.  The "
            "use of a margin helps cut down on color bleed "
            "from neighboring images.  If the texture does "
            "not end up placed in a palette image, the "
            "margin is not used.  If not specified, the default margin is "
            "used, which is specified by the :margin command (see below).\n\n");

  show_text("  coverage f", 10,
            "This parameter specifies the maximum coverage to allow for this "
            "particular texture before rejecting it "
            "from the palette.  If not specified, the default is "
            "specified by the :coverage command (see below).\n\n");

  nout << "  nearest\n"
       << "  linear\n";
  show_text("  mipmap", 10,
            "One of these options may be used to force the texture to use "
            "a particular minfilter/magfilter sampling mode.  If this is not "
            "specified, the sampling mode specified in the egg file is "
            "used.  Textures that use different sampling modes cannot "
            "be placed together on the same palette images.\n\n");

  show_text("  rgba", 10,
            "This specifies format 'rgba' should be in effect for this "
            "particular texture.  Any valid egg texture format, such as "
            "rgba, rgba12, rgba8, rgb5, luminance, etc. may be specified.  "
            "If nothing is specified, the format specified in the egg file "
            "is used.  The format will automatically be downgraded to match "
            "the number of channels in the texture image; e.g. rgba will "
            "automatically be converted to rgb for a three-channel image.  "
            "As with the filter modes above, textures that use different "
            "formats cannot be placed together on the same palette "
            "images.\n\n");

  show_text("  (image type)", 10,
            "A texture may be converted to a particular image type, for "
            "instance jpg or rgb, by naming the type.  If present, this "
            "overrides the :imagetype command, described below.  As with "
            ":imagetype, you may also specify two type names separated "
            "by a comma, to indicate that a different file should be written "
            "for the color and alpha components.\n\n");

  show_text("  (group name)", 10,
            "A texture may also be assigned to a specific group by naming "
            "the group.  The groups are defined using the :group command "
            "(see below).  Normally, textures are not assigned directly "
            "to groups; instead, it is more useful to assign the egg files "
            "they are referenced in to groups; see below.\n\n");

  show_text("  cont", 10,
            "Normally, a texture file (or egg file) scans the lines in the "
            "attributes file from the top, and stops on the first line that "
            "matches its name.  If the keyword 'cont' is included on the "
            "line, however, the texture will apply the properties given "
            "on the line, and then continue scanning.  This trick may be "
            "used to specify general parameters for all files while still "
            "allowing the texture to match a more specific line below.\n\n");

  nout <<
    "The attributes file may also assign egg files to various "
    "named palette groups.  The syntax is similar to the above:\n\n"

    "  car-blue.egg : main\n"
    "  road.egg house.egg : main\n"
    "  plane.egg : phase_2 main\n"
    "  *.egg : phase_2\n\n"

    "Any number of egg files may be named on one line, and the set of "
    "named egg files may be simultaneously assigned to one or more groups.  "
    "Each group must have been previously defined using the :group command "
    "(see below).  Each texture that is referenced by a given "
    "egg file will be palettized "
    "into at least one of the groups assigned to the egg file.\n\n"

    "Finally, there are a number of special commands that may appear in the "
    "attributes file; some of these have been alluded to in the above "
    "comments.  These commands typically specify global parameters or "
    "palettization options.  The command names begin with a colon to "
    "distinguish them from other kinds of lines.  Each command must "
    "appear on a line by itself.  The commands are:\n\n";

  show_text("  :palette xsize ysize", 10,
            "This specifies the size of the palette images to be "
            "created.  The default is 512 by 512.\n\n");

  show_text("  :margin msize", 10,
            "This specifies the amount of default margin to apply to all "
            "textures that are placed within a palette image.  The margin "
            "is a number of additional pixels that are written around the "
            "texture image to help prevent color bleeding between "
            "neighboring images within the same palette.  The default "
            "is 2.\n\n");

  show_text("  :coverage area", 10,
            "The 'coverage' of a texture refers to the fraction of "
            "the area in the texture image that is actually used, according "
            "to the UV's that appear in the various egg files.  If a texture's "
            "coverage is less than 1, only some of the texture image is used "
            "(and only this part will be written to the palette).  If the "
            "coverage is greater than 1, the texture repeats that number of "
            "times.  A repeating texture may still be palettized by writing "
            "the required number of copies into the palette image, according "
            "to the coverage area.\n\n"

            "This command specifies the maximum coverage to allow for any "
            "texture before rejecting it from the palette.  It may be any "
            "floating-point number greater than zero.  Set this to 1 "
            "to avoid palettizing repeating textures altogether.  This may "
            "also be overridden for a particular texture using the 'coverage' "
            "keyword on the texture line.\n\n");

  show_text("  :round fraction fuzz", 10,
            "When the coverage area is computed, it may optionally be "
            "rounded up to the next sizeable unit before placing the "
            "texture within the palette.  This helps reduce constant "
            "repalettization caused by slight differences in coverage "
            "between egg files.  For instance, say file a.egg references a "
            "texture with a coverage of 0.91, and then later file b.egg "
            "is discovered to reference the same texture with a coverage of "
            "0.92.  If the texture was already palettized with the original "
            "coverage of 0.91, it must now be moved in the palette.\n\n"

            "Rounding the coverage area up to some fixed unit reduces this "
            "problem.  For instance, if you specified a value 0.5 for "
            "fraction in the above command, it would round both of these "
            "values up to the next half-unit, or 1.0.\n\n"

            "The second number is a fuzz factor, and should be a small "
            "number; if the coverage area is just slightly larger than "
            "the last unit (within the fuzz factor), it is rounded down "
            "instead of up.  This is intended to prevent UV coordinates "
            "that are just slightly out of the range [0, 1] (which happens "
            "fairly often) from forcing the palettization area all the "
            "way up to the next stop.\n\n"

            "The default if this is unspecified is 0.1 0.01.  That is, "
            "round up to the next tenth, unless within a hundredth of the "
            "last tenth.  To disable rounding, specify ':round no'.  "
            "Rounding is implicitly disabled when you run with the -opt "
            "command line option.\n\n");

  show_text("  :remap (never | group | poly)", 10,
            "Sometimes two different parts of an egg file may reference "
            "different regions of a repeating texture.  For instance, "
            "group A may reference UV coordinate values ranging from (0,5) "
            "to (1,6), for a coverage of 1.0, while group B references "
            "values ranging from (0,2) to (1,4), for a coverage of 2.0.  "
            "The maximum coverage used is only 2.0, and thus the texture "
            "only needs to appear in the palette twice, but the total range "
            "of UV's is from (0,2) to (1,6), causing an apparent coverage "
            "of 4.0.\n\n"

            "It's possible for egg-palettize to reduce this kind of mistake "
            "by remapping both groups of UV's so that they overlap.  This "
            "parameter specifies how this operation should be done.  If "
            "the option is 'never', remapping will not be performed; if "
            "'group', entire groups will be remapped as a unit, if 'poly', "
            "individual polygons within a group may be remapped.  This last "
            "option provides the greatest minimization of UV coverage, "
            "but possibly at the expense of triangle strips in the resulting "
            "model (since some vertices can no longer be shared).\n\n"

            "Sometimes, it may be necessary to be more restrictive on "
            "character geometry than on non-character geometry, because "
            "the cost of adding additional vertices on characters is "
            "greater.  You can specify a different kind of remapping for "
            "characters only, by using the keyword 'char' on the same line, "
            "e.g. ':remap group char never'.\n\n"

            "The default remap mode for all geometry, character or otherwise, "
            "if no remap mode is specified is 'poly'.\n\n");

  show_text("  :imagetype type[,alpha_type]", 10,
            "This specifies the default type of image file that should be "
            "generated for each palette image and for each unplaced texture "
            "copied into the install directory.  This may be overridden for "
            "a particular texture by specifying the image type on the "
            "texture line.\n\n"

            "If two image type names separate by a comma are given, it means "
            "to generate a second file of the second type for the alpha "
            "channel, for images that require an alpha channel.  This allows "
            "support for image file formats that do not support alpha "
            "(for instance, JPEG).\n\n");

  show_text("  :shadowtype type[,alpha_type]", 10,
            "When generating palette images, egg-palettize sometimes has to "
            "read and write the same palette image repeatedly.  If the "
            "palette image is stored in a lossy file format (like JPEG, see "
            ":imagetype), this can eventually lead to degradation of the "
            "palette images.  As a workaround, egg-palettize can store "
            "its working copies of the palette images in lossless shadow "
            "images.  Specify this to enable this feature; give it the "
            "name of a lossless image file format.  The shadow images will "
            "be written to the directory specified by -ds on the command "
            "line.\n\n");

  show_text("  :group groupname [dir dirname] [with group1 group2 ...]", 10,
            "This defines a palette group, a logical division of textures.  "
            "Each texture is assigned to one or more palette groups before "
            "being placed in any palette image; the palette images are "
            "tied to the groups.\n\n"

            "The optional parameter 'dir' specifies a directory name to "
            "associate with this group.  This name is substituted in for "
            "the string '%g' when it appears in the map directory name "
            "specified on the command line with -dm; this may be used to "
            "install textures and palettes into different directories based "
            "on the groups they are assigned to.\n\n"

            "Palette groups can also be hierarchically related.  The "
            "keyword 'with' specifies any number of groups that this "
            "palette group depends on; if a texture has already been "
            "assigned to one of this group's dependent groups, it will "
            "not need to be assigned to this group.\n\n");


  nout <<
    "Comments may appear freely throughout the file, and are set off by a "
    "hash mark (#).\n\n";
}


////////////////////////////////////////////////////////////////////
//     Function: EggPalettize::run
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
void EggPalettize::
run() {
  // Fiddle with the loader severity, so we don't confuse the user
  // with spurious "reading" and "writing" messages about the state
  // file.  If the severity is currently NS_info (the default), set it
  // to NS_warning instead.
  Notify *notify = Notify::ptr();
  NotifyCategory *loader_cat = notify->get_category(":loader");
  if (loader_cat != (NotifyCategory *)NULL &&
      loader_cat->get_severity() == NS_info) {
    loader_cat->set_severity(NS_warning);
  }

  if (!_txa_filename.exists() && !_got_txa_filename) {
    // If we did not specify a filename, and the default filename of
    // "textures.txa" doesn't exist, try looking in src/maps, as
    // another likely possibility.
    Filename maybe = _txa_filename;
    maybe.set_dirname("src/maps");
    if (maybe.exists()) {
      _txa_filename = maybe;
    }
  }

  if (!_txa_filename.exists()) {
    nout << FilenameUnifier::make_user_filename(_txa_filename)
         << " does not exist; cannot run.\n";
    exit(1);
  }

  FilenameUnifier::set_txa_filename(_txa_filename);

  Filename state_filename = _txa_filename;
  state_filename.set_extension("boo");

  BamFile state_file;

  if (!state_filename.exists()) {
    nout << FilenameUnifier::make_user_filename(state_filename)
         << " does not exist; starting palettization from scratch.\n";
    pal = new Palettizer;

  } else {
    // Read the Palettizer object from the Bam file written
    // previously.  This will recover all of the state saved from the
    // past session.
    nout << "Reading " << FilenameUnifier::make_user_filename(state_filename)
         << "\n";

    if (!state_file.open_read(state_filename)) {
      nout << FilenameUnifier::make_user_filename(state_filename)
           << " exists, but cannot be read.  Perhaps you should "
           << "remove it so a new one can be created.\n";
      exit(1);
    }

    TypedWritable *obj = state_file.read_object();
    if (obj == (TypedWritable *)NULL || !state_file.resolve()) {
      nout << FilenameUnifier::make_user_filename(state_filename)
           << " exists, but appears to be corrupt.  Perhaps you "
           << "should remove it so a new one can be created.\n";
      exit(1);
    }

    if (!obj->is_of_type(Palettizer::get_class_type())) {
      nout << FilenameUnifier::make_user_filename(state_filename)
           << " exists, but does not appear to be "
           << "an egg-palettize output file.  Perhaps you "
           << "should remove it so a new one can be created.\n";
      exit(1);
    }

    state_file.close();

    pal = DCAST(Palettizer, obj);
  }

  if (_report_pi) {
    pal->report_pi();
    exit(0);
  }

  if (_report_statistics) {
    pal->report_statistics();
    exit(0);
  }

  bool okflag = true;

  pal->read_txa_file(_txa_filename);

  if (_got_default_groupname) {
    pal->_default_groupname = _default_groupname;
  } else {
    pal->_default_groupname = _txa_filename.get_basename_wo_extension();
  }

  if (_got_default_groupdir) {
    pal->_default_groupdir = _default_groupdir;
  }

  if (_got_map_dirname) {
    pal->_map_dirname = _map_dirname;
  }
  if (_got_shadow_dirname) {
    pal->_shadow_dirname = _shadow_dirname;
  }
  if (_got_rel_dirname) {
    pal->_rel_dirname = _rel_dirname;
    FilenameUnifier::set_rel_dirname(_rel_dirname);
  }

  // We only omit solitary textures from palettes if we're running in
  // optimal mode.  Otherwise, we're likely to invalidate old egg
  // files by changing a texture from solitary to nonsolitary state or
  // vice-versa.
  pal->_omit_solitary = _optimal;

  pal->all_params_set();

  // Remove any files named for removal.
  Args::const_iterator ai;
  for (ai = _remove_egg_list.begin(); ai != _remove_egg_list.end(); ++ai) {
    Filename filename = (*ai);
    pal->remove_egg_file(filename.get_basename());
  }

  // And process the egg files named for addition.
  Eggs::const_iterator ei;
  for (ei = _eggs.begin(); ei != _eggs.end(); ++ei) {
    EggData *egg_data = (*ei);
    Filename source_filename = egg_data->get_egg_filename();
    Filename dest_filename = get_output_filename(source_filename);
    string name = source_filename.get_basename();

    EggFile *egg_file = pal->get_egg_file(name);
    egg_file->from_command_line(egg_data, source_filename, dest_filename);

    pal->_command_line_eggs.push_back(egg_file);
  }

  if (_optimal) {
    // If we're asking for an optimal packing, throw away the old
    // packing and start fresh.
    pal->reset_images();
    _all_textures = true;

    // Also turn off the rounding-up of UV's for this purpose.
    pal->_round_uvs = false;
  }

  if (_all_textures) {
    pal->process_all(_redo_all);
  } else {
    pal->process_command_line_eggs(_redo_all);
  }

  if (_optimal) {
    // If we're asking for optimal packing, this also implies we want
    // to resize the big empty palette images down.
    pal->optimal_resize();
  }

  if (_redo_eggs) {
    if (!pal->read_stale_eggs(_redo_all)) {
      okflag = false;
    }
  }

  pal->generate_images(_redo_all);

  if (!pal->write_eggs()) {
    okflag = false;
  }

  // Make up a temporary filename to write the state file to, then
  // move the state file into place.  We do this in case the user
  // interrupts us (or we core dump) before we're done; that way we
  // won't leave the state file incompletely written.
  string dirname = state_filename.get_dirname();
  if (dirname.empty()) {
    dirname = ".";
  }
  Filename temp_filename = Filename::temporary(dirname, "pi");

  if (!state_file.open_write(temp_filename) ||
      !state_file.write_object(pal)) {
    nout << "Unable to write palettization information to "
         << FilenameUnifier::make_user_filename(temp_filename)
         << "\n";
    exit(1);
  }

  state_file.close();
  state_filename.unlink();
  if (!temp_filename.rename_to(state_filename)) {
    nout << "Unable to rename temporary file "
         << FilenameUnifier::make_user_filename(temp_filename) << " to "
         << FilenameUnifier::make_user_filename(state_filename) << "\n";
    exit(1);
  }

  if (!okflag) {
    exit(1);
  }
}

int
main(int argc, char *argv[]) {
  EggPalettize prog;
  prog.parse_command_line(argc, argv);
  prog.run();
  return 0;
}


