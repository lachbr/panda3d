// Filename: eggFile.cxx
// Created by:  drose (29Nov00)
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

#include "eggFile.h"
#include "textureImage.h"
#include "paletteGroup.h"
#include "texturePlacement.h"
#include "textureReference.h"
#include "sourceTextureImage.h"
#include "palettizer.h"
#include "filenameUnifier.h"

#include <eggData.h>
#include <eggTextureCollection.h>
#include <datagram.h>
#include <datagramIterator.h>
#include <bamReader.h>
#include <bamWriter.h>
#include <executionEnvironment.h>
#include <dSearchPath.h>

TypeHandle EggFile::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: EggFile::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
EggFile::
EggFile() {
  _data = (EggData *)NULL;
  _default_group = (PaletteGroup *)NULL;
  _is_surprise = true;
  _is_stale = true;
  _first_txa_match = false;
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::from_command_line
//       Access: Public
//  Description: Accepts the information about the egg file as
//               supplied from the command line.
////////////////////////////////////////////////////////////////////
void EggFile::
from_command_line(EggData *data,
                  const Filename &source_filename,
                  const Filename &dest_filename) {
  _data = data;

  // We save the current directory at the time the egg file appeared
  // on the command line, so that we'll later be able to properly
  // resolve external references (like textures) that might be
  // relative to this directory.
  _current_directory = ExecutionEnvironment::get_cwd();
  _source_filename = source_filename;
  _source_filename.make_absolute();
  _dest_filename = dest_filename;
  _dest_filename.make_absolute();

  // We save the default PaletteGroup at this point, because the egg
  // file inherits the default group that was in effect when it was
  // specified on the command line.
  _default_group = pal->get_default_group();
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::scan_textures
//       Access: Public
//  Description: Scans the egg file for texture references and updates
//               the _textures list appropriately.  This assumes the
//               egg file was supplied on the command line and thus
//               the _data member is available.
////////////////////////////////////////////////////////////////////
void EggFile::
scan_textures() {
  nassertv(_data != (EggData *)NULL);

  EggTextureCollection tc;
  tc.find_used_textures(_data);

  // Remove the old TextureReference objects.
  Textures::iterator ti;
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    delete (*ti);
  }
  _textures.clear();

  EggTextureCollection::iterator eti;
  for (eti = tc.begin(); eti != tc.end(); ++eti) {
    EggTexture *egg_tex = (*eti);

    TextureReference *ref = new TextureReference;
    ref->from_egg(this, _data, egg_tex);

    if (!ref->has_uvs()) {
      // This texture isn't *really* referenced.  (Usually this
      // happens if the texture is only referenced by "backstage"
      // geometry, which we don't care about.)
      delete ref;

    } else {
      _textures.push_back(ref);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::get_textures
//       Access: Public
//  Description: Fills up the indicated set with the set of textures
//               referenced by this egg file.  It is the user's
//               responsibility to ensure the set is empty before
//               making this call; otherwise, the new textures will be
//               appended to the existing set.
////////////////////////////////////////////////////////////////////
void EggFile::
get_textures(pset<TextureImage *> &result) const {
  Textures::const_iterator ti;
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    result.insert((*ti)->get_texture());
  }
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::pre_txa_file
//       Access: Public
//  Description: Does some processing prior to scanning the .txa file.
////////////////////////////////////////////////////////////////////
void EggFile::
pre_txa_file() {
  _is_surprise = true;
  _first_txa_match = true;
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::match_txa_groups
//       Access: Public
//  Description: Adds the indicated set of groups, read from the .txa
//               file, to the set of groups to which the egg file is
//               assigned.
////////////////////////////////////////////////////////////////////
void EggFile::
match_txa_groups(const PaletteGroups &groups) {
  if (_first_txa_match) {
    // If this is the first line we matched in the .txa file, clear
    // the set of groups we'd matched from before.  We don't clear
    // until we match a line in the .txa file, because if we don't
    // match any lines we still want to remember what groups we used
    // to be assigned to.
    _explicitly_assigned_groups.clear();
    _first_txa_match = false;
  }

  _explicitly_assigned_groups.make_union(_explicitly_assigned_groups, groups);
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::post_txa_file
//       Access: Public
//  Description: Once the egg file has been matched against all of the
//               matching lines the .txa file, do whatever adjustment
//               is necessary.
////////////////////////////////////////////////////////////////////
void EggFile::
post_txa_file() {
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::get_explicit_groups
//       Access: Public
//  Description: Returns the set of PaletteGroups that the egg file
//               has been explicitly assigned to in the .txa file.
////////////////////////////////////////////////////////////////////
const PaletteGroups &EggFile::
get_explicit_groups() const {
  return _explicitly_assigned_groups;
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::get_default_group
//       Access: Public
//  Description: Returns the PaletteGroup that was specified as the
//               default group on the command line at the time the egg
//               file last appeared on the command line.
////////////////////////////////////////////////////////////////////
PaletteGroup *EggFile::
get_default_group() const {
  return _default_group;
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::get_complete_groups
//       Access: Public
//  Description: Returns the complete set of PaletteGroups that the
//               egg file is assigned to.  This is the set of all the
//               groups it is explicitly assigned to, plus all the
//               groups that these groups depend on.
////////////////////////////////////////////////////////////////////
const PaletteGroups &EggFile::
get_complete_groups() const {
  return _complete_groups;
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::clear_surprise
//       Access: Public
//  Description: Removes the 'surprise' flag; this file has been
//               successfully matched against a line in the .txa file.
////////////////////////////////////////////////////////////////////
void EggFile::
clear_surprise() {
  _is_surprise = false;
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::is_surprise
//       Access: Public
//  Description: Returns true if this particular egg file is a
//               'surprise', i.e. it wasn't matched by a line in the
//               .txa file that didn't include the keyword 'cont'.
////////////////////////////////////////////////////////////////////
bool EggFile::
is_surprise() const {
  return _is_surprise;
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::mark_stale
//       Access: Public
//  Description: Marks this particular egg file as stale, meaning that
//               something has changed, such as the location of a
//               texture within its palette, which causes the egg file
//               to need to be regenerated.
////////////////////////////////////////////////////////////////////
void EggFile::
mark_stale() {
  _is_stale = true;
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::is_stale
//       Access: Public
//  Description: Returns true if the egg file needs to be updated,
//               i.e. some palettizations have changed affecting it,
//               or false otherwise.
////////////////////////////////////////////////////////////////////
bool EggFile::
is_stale() const {
  return _is_stale;
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::build_cross_links
//       Access: Public
//  Description: Calls TextureImage::note_egg_file() and
//               SourceTextureImage::increment_egg_count() for each
//               texture the egg file references, and
//               PaletteGroup::increment_egg_count() for each palette
//               group it wants.  This sets up some of the back
//               references to support determining an ideal texture
//               assignment.
////////////////////////////////////////////////////////////////////
void EggFile::
build_cross_links() {
  if (_explicitly_assigned_groups.empty()) {
    // If the egg file has been assigned to no groups, we have to
    // assign it to something.
    _complete_groups.clear();
    _complete_groups.insert(_default_group);
    _complete_groups.make_complete(_complete_groups);

  } else {
    _complete_groups.make_complete(_explicitly_assigned_groups);
  }

  Textures::const_iterator ti;
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    TextureReference *reference = (*ti);
    TextureImage *texture = reference->get_texture();
    nassertv(texture != (TextureImage *)NULL);
    texture->note_egg_file(this);

    // Actually, this may count the same egg file multiple times for a
    // particular SourceTextureImage, since a given texture may be
    // reference multiples times within an egg file.  No harm done,
    // however.
    reference->get_source()->increment_egg_count();
  }

  PaletteGroups::const_iterator gi;
  for (gi = _complete_groups.begin();
       gi != _complete_groups.end();
       ++gi) {
    (*gi)->increment_egg_count();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::choose_placements
//       Access: Public
//  Description: Once all the textures have been assigned to groups
//               (but before they may actually be placed), chooses a
//               suitable TexturePlacement for each texture that
//               appears in the egg file.  This will be necessary to
//               do at some point before writing out the egg file
//               anyway, and doing it before the textures are placed
//               allows us to decide what the necessary UV range is
//               for each to-be-placed texture.
////////////////////////////////////////////////////////////////////
void EggFile::
choose_placements() {
  Textures::const_iterator ti;
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    TextureReference *reference = (*ti);
    TextureImage *texture = reference->get_texture();

    if (reference->get_placement() != (TexturePlacement *)NULL &&
        texture->get_groups().count(reference->get_placement()->get_group()) != 0) {
      // The egg file is already using a TexturePlacement that is
      // suitable.  Don't bother changing it.

    } else {
      // We need to select a new TexturePlacement.
      PaletteGroups groups;
      groups.make_intersection(get_complete_groups(), texture->get_groups());

      // Now groups is the set of groups that the egg file requires,
      // which also happen to include the texture.  It better not be
      // empty.
      if (groups.empty()) {
        nout << "Warning!  Egg file " << get_name() << " and texture "
             << *reference << " do not have any groups in common.\n"
             << "Egg groups:\n";
        get_complete_groups().write(nout, 2);
        nout << "Texture groups:\n";
        texture->get_groups().write(nout, 2);

      } else {
        // It doesn't really matter which group in the set we choose, so
        // we arbitrarily choose the first one.
        PaletteGroup *group = (*groups.begin());

        // Now get the TexturePlacement object that corresponds to the
        // placement of this texture into this group.
        TexturePlacement *placement = texture->get_placement(group);
        nassertv(placement != (TexturePlacement *)NULL);

        reference->set_placement(placement);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::has_data
//       Access: Public
//  Description: Returns true if the EggData for this EggFile has ever
//               been loaded.
////////////////////////////////////////////////////////////////////
bool EggFile::
has_data() const {
  return (_data != (EggData *)NULL);
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::update_egg
//       Access: Public
//  Description: Once all textures have been placed appropriately,
//               updates the egg file with all the information to
//               reference the new textures.
////////////////////////////////////////////////////////////////////
void EggFile::
update_egg() {
  Textures::iterator ti;
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    TextureReference *reference = (*ti);
    reference->update_egg();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::remove_egg
//       Access: Public
//  Description: Removes this egg file from all things that reference
//               it, in preparation for removing it from the database.
////////////////////////////////////////////////////////////////////
void EggFile::
remove_egg() {
  Textures::iterator ti;
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    TextureReference *reference = (*ti);
    TexturePlacement *placement = reference->get_placement();
    placement->remove_egg(reference);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::read_egg
//       Access: Public
//  Description: Reads in the egg file from its _source_filename.  It
//               is only valid to call this if it has not already been
//               read in, e.g. from the command line.  Returns true if
//               successful, false if there is an error.
////////////////////////////////////////////////////////////////////
bool EggFile::
read_egg() {
  nassertr(_data == (EggData *)NULL, false);
  nassertr(!_source_filename.empty(), false);

  if (!_source_filename.exists()) {
    nout << _source_filename << " does not exist.\n";
    return false;
  }

  EggData *data = new EggData;
  if (!data->read(_source_filename)) {
    // Failure reading.
    delete data;
    return false;
  }

  // We also want to search for filenames based on our current
  // directory from which we originally loaded the egg file.  This is
  // important because it's possible the egg file referenced some
  // textures or something relative to that directory.
  DSearchPath dir;
  dir.append_directory(_current_directory);
  data->resolve_filenames(dir);

  if (!data->resolve_externals()) {
    // Failure reading an external.
    delete data;
    return false;
  }

  _data = data;
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::write_egg
//       Access: Public
//  Description: Writes out the egg file to its _dest_filename.
//               Returns true if successful, false if there is an
//               error.
////////////////////////////////////////////////////////////////////
bool EggFile::
write_egg() {
  nassertr(_data != (EggData *)NULL, false);
  nassertr(!_dest_filename.empty(), false);

  _dest_filename.make_dir();
  nout << "Writing " << FilenameUnifier::make_user_filename(_dest_filename)
       << "\n";
  if (!_data->write_egg(_dest_filename)) {
    // Some error while writing.  Most unusual.
    _is_stale = true;
    return false;
  }

  _is_stale = false;
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::write_description
//       Access: Public
//  Description: Writes a one-line description of the egg file and its
//               group assignments to the indicated output stream.
////////////////////////////////////////////////////////////////////
void EggFile::
write_description(ostream &out, int indent_level) const {
  indent(out, indent_level) << get_name() << ": ";
  if (_explicitly_assigned_groups.empty()) {
    if (_default_group != (PaletteGroup *)NULL) {
      out << _default_group->get_name();
    }
  } else {
    out << _explicitly_assigned_groups;
  }

  if (is_stale()) {
    out << " (needs update)";
  }
  out << "\n";
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::write_texture_refs
//       Access: Public
//  Description: Writes the list of texture references to the
//               indicated output stream, one per line.
////////////////////////////////////////////////////////////////////
void EggFile::
write_texture_refs(ostream &out, int indent_level) const {
  Textures::const_iterator ti;
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    TextureReference *reference = (*ti);
    reference->write(out, indent_level);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::register_with_read_factory
//       Access: Public, Static
//  Description: Registers the current object as something that can be
//               read from a Bam file.
////////////////////////////////////////////////////////////////////
void EggFile::
register_with_read_factory() {
  BamReader::get_factory()->
    register_factory(get_class_type(), make_EggFile);
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::write_datagram
//       Access: Public, Virtual
//  Description: Fills the indicated datagram up with a binary
//               representation of the current object, in preparation
//               for writing to a Bam file.
////////////////////////////////////////////////////////////////////
void EggFile::
write_datagram(BamWriter *writer, Datagram &datagram) {
  datagram.add_string(get_name());

  // We don't write out _data; that needs to be reread each session.

  datagram.add_string(FilenameUnifier::make_bam_filename(_current_directory));
  datagram.add_string(FilenameUnifier::make_bam_filename(_source_filename));
  datagram.add_string(FilenameUnifier::make_bam_filename(_dest_filename));

  datagram.add_uint32(_textures.size());
  Textures::iterator ti;
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    writer->write_pointer(datagram, (*ti));
  }

  _explicitly_assigned_groups.write_datagram(writer, datagram);
  writer->write_pointer(datagram, _default_group);

  // We don't write out _complete_groups; that is recomputed each
  // session.

  datagram.add_bool(_is_surprise);
  datagram.add_bool(_is_stale);
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::complete_pointers
//       Access: Public, Virtual
//  Description: Called after the object is otherwise completely read
//               from a Bam file, this function's job is to store the
//               pointers that were retrieved from the Bam file for
//               each pointer object written.  The return value is the
//               number of pointers processed from the list.
////////////////////////////////////////////////////////////////////
int EggFile::
complete_pointers(vector_typedWritable &p_list, BamReader *manager) {
  nassertr((int)p_list.size() >= _num_textures + 1, 0);
  int index = 0;

  int i;
  _textures.reserve(_num_textures);
  for (i = 0; i < _num_textures; i++) {
    TextureReference *texture;
    DCAST_INTO_R(texture, p_list[index], index);
    _textures.push_back(texture);
    index++;
  }

  if (p_list[index] != (TypedWritable *)NULL) {
    DCAST_INTO_R(_default_group, p_list[index], index);
  }
  index++;

  return index;
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::make_EggFile
//       Access: Protected
//  Description: This method is called by the BamReader when an object
//               of this type is encountered in a Bam file; it should
//               allocate and return a new object with all the data
//               read.
////////////////////////////////////////////////////////////////////
TypedWritable* EggFile::
make_EggFile(const FactoryParams &params) {
  EggFile *me = new EggFile;
  DatagramIterator scan;
  BamReader *manager;

  parse_params(params, scan, manager);
  me->fillin(scan, manager);
  return me;
}

////////////////////////////////////////////////////////////////////
//     Function: EggFile::fillin
//       Access: Protected
//  Description: Reads the binary data from the given datagram
//               iterator, which was written by a previous call to
//               write_datagram().
////////////////////////////////////////////////////////////////////
void EggFile::
fillin(DatagramIterator &scan, BamReader *manager) {
  set_name(scan.get_string());
  _current_directory = FilenameUnifier::get_bam_filename(scan.get_string());
  _source_filename = FilenameUnifier::get_bam_filename(scan.get_string());
  _dest_filename = FilenameUnifier::get_bam_filename(scan.get_string());

  _num_textures = scan.get_uint32();
  manager->read_pointers(scan, this, _num_textures);

  _explicitly_assigned_groups.fillin(scan, manager);
  manager->read_pointer(scan, this);  // _default_group

  _is_surprise = scan.get_bool();
  _is_stale = scan.get_bool();
}
