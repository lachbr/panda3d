// Filename: palettizer.cxx
// Created by:  drose (01Dec00)
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

#include "palettizer.h"
#include "eggFile.h"
#include "textureImage.h"
#include "pal_string_utils.h"
#include "paletteGroup.h"
#include "filenameUnifier.h"
#include "textureMemoryCounter.h"

#include <pnmImage.h>
#include <pnmFileTypeRegistry.h>
#include <pnmFileType.h>
#include <eggData.h>
#include <datagram.h>
#include <datagramIterator.h>
#include <bamReader.h>
#include <bamWriter.h>
#include <indent.h>

Palettizer *pal = (Palettizer *)NULL;

// This number is written out as the first number to the pi file, to
// indicate the version of egg-palettize that wrote it out.  This
// allows us to easily update egg-palettize to write out additional
// information to its pi file, without having it increment the bam
// version number for all bam and boo files anywhere in the world.
int Palettizer::_pi_version = 6;
// Updated to version 1 on 12/11/00 to add _remap_char_uv.
// Updated to version 2 on 12/19/00 to add TexturePlacement::_dest.
// Updated to version 3 on 12/19/00 to add PaletteGroup::_dependency_order.
// Updated to version 4 on 5/3/01 to add PaletteGroup::_dirname_order.
// Updated to version 5 on 10/31/01 to add TextureProperties::_force_format.
// Updated to version 6 on 3/14/02 to add TextureImage::_alpha_mode.

int Palettizer::_read_pi_version = 0;

TypeHandle Palettizer::_type_handle;

ostream &operator << (ostream &out, Palettizer::RemapUV remap) {
  switch (remap) {
  case Palettizer::RU_never:
    return out << "never";

  case Palettizer::RU_group:
    return out << "per group";

  case Palettizer::RU_poly:
    return out << "per polygon";

  case Palettizer::RU_invalid:
    return out << "(invalid)";
  }

  return out << "**invalid**(" << (int)remap << ")";
}


// This STL function object is used in report_statistics(), below.
class SortGroupsByDependencyOrder {
public:
  bool operator ()(PaletteGroup *a, PaletteGroup *b) {
    if (a->get_dependency_order() != b->get_dependency_order()) {
      return a->get_dependency_order() < b->get_dependency_order();
    }
    return a->get_name() < b->get_name();
  }
};

// And this one is used in report_pi().
class SortGroupsByPreference {
public:
  bool operator ()(PaletteGroup *a, PaletteGroup *b) {
    return !a->is_preferred_over(*b);
  }
};

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
Palettizer::
Palettizer() {
  _map_dirname = "%g";
  _shadow_dirname = "shadow";
  _margin = 2;
  _omit_solitary = false;
  _coverage_threshold = 2.5;
  _aggressively_clean_mapdir = true;
  _force_power_2 = true;
  _color_type = PNMFileTypeRegistry::get_ptr()->get_type_from_extension("rgb");
  _alpha_type = (PNMFileType *)NULL;
  _shadow_color_type = (PNMFileType *)NULL;
  _shadow_alpha_type = (PNMFileType *)NULL;
  _pal_x_size = _pal_y_size = 512;

  _round_uvs = true;
  _round_unit = 0.1;
  _round_fuzz = 0.01;
  _remap_uv = RU_poly;
  _remap_char_uv = RU_poly;
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::report_pi
//       Access: Public
//  Description: Output a verbose description of all the palettization
//               information to standard output, for the user's
//               perusal.
////////////////////////////////////////////////////////////////////
void Palettizer::
report_pi() const {
  // Start out with the cross links and back counts; some of these are
  // nice to report.
  EggFiles::const_iterator efi;
  for (efi = _egg_files.begin(); efi != _egg_files.end(); ++efi) {
    (*efi).second->build_cross_links();
  }

  cout
    << "\nparams\n"
    << "  map directory: " << _map_dirname << "\n"
    << "  shadow directory: "
    << FilenameUnifier::make_user_filename(_shadow_dirname) << "\n"
    << "  egg relative directory: "
    << FilenameUnifier::make_user_filename(_rel_dirname) << "\n"
    << "  palettize size: " << _pal_x_size << " by " << _pal_y_size << "\n"
    << "  margin: " << _margin << "\n"
    << "  coverage threshold: " << _coverage_threshold << "\n"
    << "  force textures to power of 2: " << yesno(_force_power_2) << "\n"
    << "  aggressively clean the map directory: "
    << yesno(_aggressively_clean_mapdir) << "\n"
    << "  round UV area: " << yesno(_round_uvs) << "\n";
  if (_round_uvs) {
    cout << "  round UV area to nearest " << _round_unit << " with fuzz "
         << _round_fuzz << "\n";
  }
  cout << "  remap UV's: " << _remap_uv << "\n"
       << "  remap UV's for characters: " << _remap_char_uv << "\n";

  if (_color_type != (PNMFileType *)NULL) {
    cout << "  generate image files of type: "
         << _color_type->get_suggested_extension();
    if (_alpha_type != (PNMFileType *)NULL) {
      cout << "," << _alpha_type->get_suggested_extension();
    }
    cout << "\n";
  }

  if (_shadow_color_type != (PNMFileType *)NULL) {
    cout << "  generate shadow palette files of type: "
         << _shadow_color_type->get_suggested_extension();
    if (_shadow_alpha_type != (PNMFileType *)NULL) {
      cout << "," << _shadow_alpha_type->get_suggested_extension();
    }
    cout << "\n";
  }

  cout << "\ntexture source pathnames and assignments\n";
  Textures::const_iterator ti;
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    TextureImage *texture = (*ti).second;
    if (texture->is_used()) {
      cout << "  " << texture->get_name() << ":\n";
      texture->write_source_pathnames(cout, 4);
    }
  }

  cout << "\negg files and textures referenced\n";
  EggFiles::const_iterator ei;
  for (ei = _egg_files.begin(); ei != _egg_files.end(); ++ei) {
    EggFile *egg_file = (*ei).second;
    egg_file->write_description(cout, 2);
    egg_file->write_texture_refs(cout, 4);
  }

  // Sort the palette groups into order of preference, so that the
  // more specific ones appear at the bottom.
  pvector<PaletteGroup *> sorted_groups;
  Groups::const_iterator gi;
  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    sorted_groups.push_back((*gi).second);
  }
  sort(sorted_groups.begin(), sorted_groups.end(),
       SortGroupsByPreference());

  cout << "\npalette groups\n";
  pvector<PaletteGroup *>::iterator si;
  for (si = sorted_groups.begin(); si != sorted_groups.end(); ++si) {
    PaletteGroup *group = (*si);
    if (si != sorted_groups.begin()) {
      cout << "\n";
    }
    cout << "  " << group->get_name()
      //         << " (" << group->get_dirname_order() << "," << group->get_dependency_order() << ")"
         << ": " << group->get_groups() << "\n";
    group->write_image_info(cout, 4);
  }

  cout << "\ntextures\n";
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    TextureImage *texture = (*ti).second;
    texture->write_scale_info(cout, 2);
  }

  cout << "\nsurprises\n";
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    TextureImage *texture = (*ti).second;
    if (texture->is_surprise()) {
      cout << "  " << texture->get_name() << "\n";
    }
  }
  for (ei = _egg_files.begin(); ei != _egg_files.end(); ++ei) {
    EggFile *egg_file = (*ei).second;
    if (egg_file->is_surprise()) {
      cout << "  " << egg_file->get_name() << "\n";
    }
  }

  cout << "\n";
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::report_statistics
//       Access: Public
//  Description: Output a report of the palettization effectiveness,
//               texture memory utilization, and so on.
////////////////////////////////////////////////////////////////////
void Palettizer::
report_statistics() const {
  // Sort the groups into order by dependency order, for the user's
  // convenience.
  pvector<PaletteGroup *> sorted_groups;

  Groups::const_iterator gi;
  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    sorted_groups.push_back((*gi).second);
  }

  sort(sorted_groups.begin(), sorted_groups.end(),
       SortGroupsByDependencyOrder());

  Placements overall_placements;

  pvector<PaletteGroup *>::const_iterator si;
  for (si = sorted_groups.begin();
       si != sorted_groups.end();
       ++si) {
    PaletteGroup *group = (*si);

    Placements placements;
    group->get_placements(placements);
    if (!placements.empty()) {
      group->get_placements(overall_placements);

      cout << "\n" << group->get_name() << ", by itself:\n";
      compute_statistics(cout, 2, placements);

      PaletteGroups complete;
      complete.make_complete(group->get_groups());

      if (complete.size() > 1) {
        Placements complete_placements;
        group->get_complete_placements(complete_placements);
        if (complete_placements.size() != placements.size()) {
          cout << "\n" << group->get_name()
               << ", with dependents (" << complete << "):\n";
          compute_statistics(cout, 2, complete_placements);
        }
      }
    }
  }

  cout << "\nOverall:\n";
  compute_statistics(cout, 2, overall_placements);

  cout << "\n";
}


////////////////////////////////////////////////////////////////////
//     Function: Palettizer::read_txa_file
//       Access: Public
//  Description: Reads in the .txa file and keeps it ready for
//               matching textures and egg files.
////////////////////////////////////////////////////////////////////
void Palettizer::
read_txa_file(const Filename &txa_filename) {
  // Clear out the group dependencies, in preparation for reading them
  // again from the .txa file.
  Groups::iterator gi;
  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    PaletteGroup *group = (*gi).second;
    group->clear_depends();
  }

  // Also reset _shadow_color_type.
  _shadow_color_type = (PNMFileType *)NULL;
  _shadow_alpha_type = (PNMFileType *)NULL;

  if (!_txa_file.read(txa_filename)) {
    exit(1);
  }

  if (_color_type == (PNMFileType *)NULL) {
    nout << "No valid output image file type available; cannot run.\n"
         << "Use :imagetype command in .txa file.\n";
    exit(1);
  }

  // Compute the correct dependency level and order for each group.
  // This will help us when we assign the textures to their groups.
  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    PaletteGroup *group = (*gi).second;
    group->reset_dependency_level();
  }

  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    PaletteGroup *group = (*gi).second;
    group->set_dependency_level(1);
  }

  bool any_changed;
  do {
    any_changed = false;
    for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
      PaletteGroup *group = (*gi).second;
      if (group->set_dependency_order()) {
        any_changed = true;
      }
    }
  } while (any_changed);
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::all_params_set
//       Access: Public
//  Description: Called after all command line parameters have been
//               set up, this is a hook to do whatever initialization
//               is necessary.
////////////////////////////////////////////////////////////////////
void Palettizer::
all_params_set() {
  // Make sure the palettes have their shadow images set up properly.
  Groups::iterator gi;
  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    PaletteGroup *group = (*gi).second;
    group->setup_shadow_images();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::process_command_line_eggs
//       Access: Public
//  Description: Processes all the textures named in the
//               _command_line_eggs, placing them on the appropriate
//               palettes or whatever needs to be done with them.
//
//               If force_texture_read is true, it forces each texture
//               image file to be read (and thus legitimately checked
//               for grayscaleness etc.) before placing.
////////////////////////////////////////////////////////////////////
void Palettizer::
process_command_line_eggs(bool force_texture_read, const Filename &state_filename) {
  _command_line_textures.clear();

  // Start by scanning all the egg files we read up on the command
  // line.
  CommandLineEggs::const_iterator ei;
  for (ei = _command_line_eggs.begin();
       ei != _command_line_eggs.end();
       ++ei) {
    EggFile *egg_file = (*ei);

    egg_file->scan_textures();
    egg_file->get_textures(_command_line_textures);

    egg_file->pre_txa_file();
    _txa_file.match_egg(egg_file);
    egg_file->post_txa_file();
  }

  // Now that all of our egg files are read in, build in all the cross
  // links and back pointers and stuff.
  EggFiles::const_iterator efi;
  for (efi = _egg_files.begin(); efi != _egg_files.end(); ++efi) {
    (*efi).second->build_cross_links();
  }

  // Now match each of the textures mentioned in those egg files
  // against a line in the .txa file.
  CommandLineTextures::iterator ti;
  for (ti = _command_line_textures.begin();
       ti != _command_line_textures.end();
       ++ti) {
    TextureImage *texture = *ti;

    if (force_texture_read || texture->is_newer_than(state_filename)) {
      // If we're forcing a redo, or the texture image has changed,
      // re-read the complete image.
      texture->read_source_image();
    } else {
      // Otherwise, just the header is sufficient.
      texture->read_header();
    }

    texture->pre_txa_file();
    _txa_file.match_texture(texture);
    texture->post_txa_file();
  }

  // And now, assign each of the current set of textures to an
  // appropriate group or groups.
  for (ti = _command_line_textures.begin();
       ti != _command_line_textures.end();
       ++ti) {
    TextureImage *texture = *ti;
    texture->assign_groups();
  }

  // And then the egg files need to sign up for a particular
  // TexturePlacement, so we can determine some more properties about
  // how the textures are placed (for instance, how big the UV range
  // is for a particular TexturePlacement).
  for (efi = _egg_files.begin(); efi != _egg_files.end(); ++efi) {
    (*efi).second->choose_placements();
  }

  // Now that *that's* done, we need to make sure the various
  // TexturePlacements require the right size for their textures.
  for (ti = _command_line_textures.begin();
       ti != _command_line_textures.end();
       ++ti) {
    TextureImage *texture = *ti;
    texture->determine_placement_size();
  }

  // Now that each texture has been assigned to a suitable group,
  // make sure the textures are placed on specific PaletteImages.
  Groups::iterator gi;
  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    PaletteGroup *group = (*gi).second;
    group->update_unknown_textures(_txa_file);
    group->place_all();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::process_all
//       Access: Public
//  Description: Reprocesses all textures known.
//
//               If force_texture_read is true, it forces each texture
//               image file to be read (and thus legitimately checked
//               for grayscaleness etc.) before placing.
////////////////////////////////////////////////////////////////////
void Palettizer::
process_all(bool force_texture_read, const Filename &state_filename) {
  // If there *were* any egg files on the command line, deal with
  // them.
  CommandLineEggs::const_iterator ei;
  for (ei = _command_line_eggs.begin();
       ei != _command_line_eggs.end();
       ++ei) {
    EggFile *egg_file = (*ei);

    egg_file->scan_textures();
    egg_file->get_textures(_command_line_textures);
  }

  // Then match up all the egg files we know about with the .txa file.
  EggFiles::const_iterator efi;
  for (efi = _egg_files.begin(); efi != _egg_files.end(); ++efi) {
    EggFile *egg_file = (*efi).second;

    egg_file->pre_txa_file();
    _txa_file.match_egg(egg_file);
    egg_file->post_txa_file();
  }

  // Now that all of our egg files are read in, build in all the cross
  // links and back pointers and stuff.
  for (efi = _egg_files.begin(); efi != _egg_files.end(); ++efi) {
    (*efi).second->build_cross_links();
  }

  // Now match each of the textures in the world against a line in the
  // .txa file.
  Textures::iterator ti;
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    TextureImage *texture = (*ti).second;

    if (force_texture_read || texture->is_newer_than(state_filename)) {
      texture->read_source_image();
    }

    texture->pre_txa_file();
    _txa_file.match_texture(texture);
    texture->post_txa_file();
  }

  // And now, assign each texture to an appropriate group or groups.
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    TextureImage *texture = (*ti).second;
    texture->assign_groups();
  }

  // And then the egg files need to sign up for a particular
  // TexturePlacement, so we can determine some more properties about
  // how the textures are placed (for instance, how big the UV range
  // is for a particular TexturePlacement).
  for (efi = _egg_files.begin(); efi != _egg_files.end(); ++efi) {
    (*efi).second->choose_placements();
  }

  // Now that *that's* done, we need to make sure the various
  // TexturePlacements require the right size for their textures.
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    TextureImage *texture = (*ti).second;
    texture->determine_placement_size();
  }

  // Now that each texture has been assigned to a suitable group,
  // make sure the textures are placed on specific PaletteImages.
  Groups::iterator gi;
  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    PaletteGroup *group = (*gi).second;
    group->update_unknown_textures(_txa_file);
    group->place_all();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::optimal_resize
//       Access: Public
//  Description: Attempts to resize each PalettteImage down to its
//               smallest possible size.
////////////////////////////////////////////////////////////////////
void Palettizer::
optimal_resize() {
  Groups::iterator gi;
  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    PaletteGroup *group = (*gi).second;
    group->optimal_resize();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::reset_images
//       Access: Public
//  Description: Throws away all of the current PaletteImages, so that
//               new ones may be created (and the packing made more
//               optimal).
////////////////////////////////////////////////////////////////////
void Palettizer::
reset_images() {
  Groups::iterator gi;
  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    PaletteGroup *group = (*gi).second;
    group->reset_images();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::generate_images
//       Access: Public
//  Description: Actually generates the appropriate palette and
//               unplaced texture images into the map directories.  If
//               redo_all is true, this forces a regeneration of each
//               image file.
////////////////////////////////////////////////////////////////////
void Palettizer::
generate_images(bool redo_all) {
  Groups::iterator gi;
  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    PaletteGroup *group = (*gi).second;
    group->update_images(redo_all);
  }

  Textures::iterator ti;
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    TextureImage *texture = (*ti).second;
    texture->copy_unplaced(redo_all);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::read_stale_eggs
//       Access: Public
//  Description: Reads in any egg file that is known to be stale, even
//               if it was not listed on the command line, so that it
//               may be updated and written out when write_eggs() is
//               called.  If redo_all is true, this even reads egg
//               files that were not flagged as stale.
//
//               Returns true if successful, or false if there was
//               some error.
////////////////////////////////////////////////////////////////////
bool Palettizer::
read_stale_eggs(bool redo_all) {
  bool okflag = true;
  
  pvector<EggFiles::iterator> invalid_eggs;

  EggFiles::iterator ei;
  for (ei = _egg_files.begin(); ei != _egg_files.end(); ++ei) {
    EggFile *egg_file = (*ei).second;
    if (!egg_file->has_data() &&
        (egg_file->is_stale() || redo_all)) {
      if (!egg_file->read_egg()) {
        invalid_eggs.push_back(ei);

      } else {
        egg_file->scan_textures();
        egg_file->choose_placements();
      }
    }
  }

  // Now eliminate all the invalid egg files.
  pvector<EggFiles::iterator>::iterator ii;
  for (ii = invalid_eggs.begin(); ii != invalid_eggs.end(); ++ii) {
    EggFiles::iterator ei = (*ii);
    EggFile *egg_file = (*ei).second;
    nout << "Removing " << (*ei).first << "\n";
    egg_file->remove_egg();
    _egg_files.erase(ei);
  }

  return okflag;
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::write_eggs
//       Access: Public
//  Description: Adjusts the egg files to reference the newly
//               generated textures, and writes them out.  Returns
//               true if successful, or false if there was some error.
////////////////////////////////////////////////////////////////////
bool Palettizer::
write_eggs() {
  bool okflag = true;

  EggFiles::iterator ei;
  for (ei = _egg_files.begin(); ei != _egg_files.end(); ++ei) {
    EggFile *egg_file = (*ei).second;
    if (egg_file->has_data()) {
      egg_file->update_egg();
      if (!egg_file->write_egg()) {
        okflag = false;
      }
    }
  }

  return okflag;
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::get_egg_file
//       Access: Public
//  Description: Returns the EggFile with the given name.  If there is
//               no EggFile with the indicated name, creates one.
//               This is the key name used to sort the egg files,
//               which is typically the basename of the filename.
////////////////////////////////////////////////////////////////////
EggFile *Palettizer::
get_egg_file(const string &name) {
  EggFiles::iterator ei = _egg_files.find(name);
  if (ei != _egg_files.end()) {
    return (*ei).second;
  }

  EggFile *file = new EggFile;
  file->set_name(name);
  _egg_files.insert(EggFiles::value_type(name, file));
  return file;
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::remove_egg_file
//       Access: Public
//  Description: Removes the named egg file from the database, if it
//               exists.  Returns true if the egg file was found,
//               false if it was not.
////////////////////////////////////////////////////////////////////
bool Palettizer::
remove_egg_file(const string &name) {
  EggFiles::iterator ei = _egg_files.find(name);
  if (ei != _egg_files.end()) {
    EggFile *file = (*ei).second;
    file->remove_egg();
    _egg_files.erase(ei);
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::get_palette_group
//       Access: Public
//  Description: Returns the PaletteGroup with the given name.  If
//               there is no PaletteGroup with the indicated name,
//               creates one.
////////////////////////////////////////////////////////////////////
PaletteGroup *Palettizer::
get_palette_group(const string &name) {
  Groups::iterator gi = _groups.find(name);
  if (gi != _groups.end()) {
    return (*gi).second;
  }

  PaletteGroup *group = new PaletteGroup;
  group->set_name(name);
  _groups.insert(Groups::value_type(name, group));
  return group;
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::test_palette_group
//       Access: Public
//  Description: Returns the PaletteGroup with the given name.  If
//               there is no PaletteGroup with the indicated name,
//               returns NULL.
////////////////////////////////////////////////////////////////////
PaletteGroup *Palettizer::
test_palette_group(const string &name) const {
  Groups::const_iterator gi = _groups.find(name);
  if (gi != _groups.end()) {
    return (*gi).second;
  }

  return (PaletteGroup *)NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::get_default_group
//       Access: Public
//  Description: Returns the default group to which an egg file should
//               be assigned if it is not mentioned in the .txa file.
////////////////////////////////////////////////////////////////////
PaletteGroup *Palettizer::
get_default_group() {
  PaletteGroup *default_group = get_palette_group(_default_groupname);
  if (!_default_groupdir.empty() && !default_group->has_dirname()) {
    default_group->set_dirname(_default_groupdir);
  }
  return default_group;
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::get_texture
//       Access: Public
//  Description: Returns the TextureImage with the given name.  If
//               there is no TextureImage with the indicated name,
//               creates one.  This is the key name used to sort the
//               textures, which is typically the basename of the
//               primary filename.
////////////////////////////////////////////////////////////////////
TextureImage *Palettizer::
get_texture(const string &name) {
  Textures::iterator ti = _textures.find(name);
  if (ti != _textures.end()) {
    return (*ti).second;
  }

  TextureImage *image = new TextureImage;
  image->set_name(name);
  //  image->set_filename(name);
  _textures.insert(Textures::value_type(name, image));
  return image;
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::yesno
//       Access: Private, Static
//  Description: A silly function to return "yes" or "no" based on a
//               bool flag for nicely formatted output.
////////////////////////////////////////////////////////////////////
const char *Palettizer::
yesno(bool flag) {
  return flag ? "yes" : "no";
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::string_remap
//       Access: Public, Static
//  Description: Returns the RemapUV code corresponding to the
//               indicated string, or RU_invalid if the string is
//               invalid.
////////////////////////////////////////////////////////////////////
Palettizer::RemapUV Palettizer::
string_remap(const string &str) {
  if (str == "never") {
    return RU_never;

  } else if (str == "group") {
    return RU_group;

  } else if (str == "poly") {
    return RU_poly;

  } else {
    return RU_invalid;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::compute_statistics
//       Access: Private
//  Description: Determines how much memory, etc. is required by the
//               indicated set of texture placements, and reports this
//               to the indicated output stream.
////////////////////////////////////////////////////////////////////
void Palettizer::
compute_statistics(ostream &out, int indent_level,
                   const Palettizer::Placements &placements) const {
  TextureMemoryCounter counter;

  Placements::const_iterator pi;
  for (pi = placements.begin(); pi != placements.end(); ++pi) {
    TexturePlacement *placement = (*pi);
    counter.add_placement(placement);
  }

  counter.report(out, indent_level);
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::register_with_read_factory
//       Access: Public, Static
//  Description: Registers the current object as something that can be
//               read from a Bam file.
////////////////////////////////////////////////////////////////////
void Palettizer::
register_with_read_factory() {
  BamReader::get_factory()->
    register_factory(get_class_type(), make_Palettizer);
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::write_datagram
//       Access: Public, Virtual
//  Description: Fills the indicated datagram up with a binary
//               representation of the current object, in preparation
//               for writing to a Bam file.
////////////////////////////////////////////////////////////////////
void Palettizer::
write_datagram(BamWriter *writer, Datagram &datagram) {
  TypedWritable::write_datagram(writer, datagram);

  datagram.add_int32(_pi_version);
  datagram.add_string(_map_dirname);
  datagram.add_string(FilenameUnifier::make_bam_filename(_shadow_dirname));
  datagram.add_string(FilenameUnifier::make_bam_filename(_rel_dirname));
  datagram.add_int32(_pal_x_size);
  datagram.add_int32(_pal_y_size);
  datagram.add_int32(_margin);
  datagram.add_bool(_omit_solitary);
  datagram.add_float64(_coverage_threshold);
  datagram.add_bool(_force_power_2);
  datagram.add_bool(_aggressively_clean_mapdir);
  datagram.add_bool(_round_uvs);
  datagram.add_float64(_round_unit);
  datagram.add_float64(_round_fuzz);
  datagram.add_int32((int)_remap_uv);
  datagram.add_int32((int)_remap_char_uv);

  writer->write_pointer(datagram, _color_type);
  writer->write_pointer(datagram, _alpha_type);
  writer->write_pointer(datagram, _shadow_color_type);
  writer->write_pointer(datagram, _shadow_alpha_type);

  datagram.add_int32(_egg_files.size());
  EggFiles::const_iterator ei;
  for (ei = _egg_files.begin(); ei != _egg_files.end(); ++ei) {
    writer->write_pointer(datagram, (*ei).second);
  }

  // We don't write _command_line_eggs; that's specific to each
  // session.

  datagram.add_int32(_groups.size());
  Groups::const_iterator gi;
  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    writer->write_pointer(datagram, (*gi).second);
  }

  datagram.add_int32(_textures.size());
  Textures::const_iterator ti;
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    writer->write_pointer(datagram, (*ti).second);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::complete_pointers
//       Access: Public, Virtual
//  Description: Called after the object is otherwise completely read
//               from a Bam file, this function's job is to store the
//               pointers that were retrieved from the Bam file for
//               each pointer object written.  The return value is the
//               number of pointers processed from the list.
////////////////////////////////////////////////////////////////////
int Palettizer::
complete_pointers(TypedWritable **p_list, BamReader *manager) {
  int index = TypedWritable::complete_pointers(p_list, manager);

  if (p_list[index] != (TypedWritable *)NULL) {
    DCAST_INTO_R(_color_type, p_list[index], index);
  }
  index++;

  if (p_list[index] != (TypedWritable *)NULL) {
    DCAST_INTO_R(_alpha_type, p_list[index], index);
  }
  index++;

  if (p_list[index] != (TypedWritable *)NULL) {
    DCAST_INTO_R(_shadow_color_type, p_list[index], index);
  }
  index++;

  if (p_list[index] != (TypedWritable *)NULL) {
    DCAST_INTO_R(_shadow_alpha_type, p_list[index], index);
  }
  index++;

  int i;
  for (i = 0; i < _num_egg_files; i++) {
    EggFile *egg_file;
    DCAST_INTO_R(egg_file, p_list[index], index);
    _egg_files.insert(EggFiles::value_type(egg_file->get_name(), egg_file));
    index++;
  }

  for (i = 0; i < _num_groups; i++) {
    PaletteGroup *group;
    DCAST_INTO_R(group, p_list[index], index);
    _groups.insert(Groups::value_type(group->get_name(), group));
    index++;
  }

  for (i = 0; i < _num_textures; i++) {
    TextureImage *texture;
    DCAST_INTO_R(texture, p_list[index], index);
    _textures.insert(Textures::value_type(texture->get_name(), texture));
    index++;
  }

  return index;
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::make_Palettizer
//       Access: Protected
//  Description: This method is called by the BamReader when an object
//               of this type is encountered in a Bam file; it should
//               allocate and return a new object with all the data
//               read.
////////////////////////////////////////////////////////////////////
TypedWritable* Palettizer::
make_Palettizer(const FactoryParams &params) {
  Palettizer *me = new Palettizer;
  DatagramIterator scan;
  BamReader *manager;

  parse_params(params, scan, manager);
  me->fillin(scan, manager);
  return me;
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::fillin
//       Access: Protected
//  Description: Reads the binary data from the given datagram
//               iterator, which was written by a previous call to
//               write_datagram().
////////////////////////////////////////////////////////////////////
void Palettizer::
fillin(DatagramIterator &scan, BamReader *manager) {
  TypedWritable::fillin(scan, manager);

  _read_pi_version = scan.get_int32();
  _map_dirname = scan.get_string();
  _shadow_dirname = FilenameUnifier::get_bam_filename(scan.get_string());
  _rel_dirname = FilenameUnifier::get_bam_filename(scan.get_string());
  FilenameUnifier::set_rel_dirname(_rel_dirname);
  _pal_x_size = scan.get_int32();
  _pal_y_size = scan.get_int32();
  _margin = scan.get_int32();
  _omit_solitary = scan.get_bool();
  _coverage_threshold = scan.get_float64();
  _force_power_2 = scan.get_bool();
  _aggressively_clean_mapdir = scan.get_bool();
  _round_uvs = scan.get_bool();
  _round_unit = scan.get_float64();
  _round_fuzz = scan.get_float64();
  _remap_uv = (RemapUV)scan.get_int32();
  if (_read_pi_version < 1) {
    _remap_char_uv = _remap_uv;
  } else {
    _remap_char_uv = (RemapUV)scan.get_int32();
  }

  manager->read_pointer(scan);  // _color_type
  manager->read_pointer(scan);  // _alpha_type
  manager->read_pointer(scan);  // _shadow_color_type
  manager->read_pointer(scan);  // _shadow_alpha_type

  _num_egg_files = scan.get_int32();
  manager->read_pointers(scan, _num_egg_files);

  _num_groups = scan.get_int32();
  manager->read_pointers(scan, _num_groups);

  _num_textures = scan.get_int32();
  manager->read_pointers(scan, _num_textures);
}

