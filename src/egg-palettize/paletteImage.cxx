// Filename: paletteImage.cxx
// Created by:  drose (01Dec00)
// 
////////////////////////////////////////////////////////////////////

#include "paletteImage.h"
#include "palettePage.h"
#include "paletteGroup.h"
#include "texturePlacement.h"
#include "palettizer.h"
#include "textureImage.h"
#include "sourceTextureImage.h"
#include "filenameUnifier.h"

#include <indent.h>
#include <datagram.h>
#include <datagramIterator.h>
#include <bamReader.h>
#include <bamWriter.h>

#include <algorithm>

TypeHandle PaletteImage::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::ClearedRegion::Default Constructor
//       Access: Public
//  Description: The default constructor is only for the convenience
//               of the bam reader.
////////////////////////////////////////////////////////////////////
PaletteImage::ClearedRegion::
ClearedRegion() {
  _x = 0;
  _y = 0;
  _x_size = 0;
  _y_size = 0;
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::ClearedRegion::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
PaletteImage::ClearedRegion::
ClearedRegion(TexturePlacement *placement) {
  _x = placement->get_placed_x();
  _y = placement->get_placed_y();
  _x_size = placement->get_placed_x_size();
  _y_size = placement->get_placed_y_size();
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::ClearedRegion::Copy Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
PaletteImage::ClearedRegion::
ClearedRegion(const PaletteImage::ClearedRegion &copy) :
  _x(copy._x),
  _y(copy._y),
  _x_size(copy._x_size),
  _y_size(copy._y_size)
{
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::ClearedRegion::Copy Assignment Operator
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
void PaletteImage::ClearedRegion::
operator = (const PaletteImage::ClearedRegion &copy) {
  _x = copy._x;
  _y = copy._y;
  _x_size = copy._x_size;
  _y_size = copy._y_size;
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::ClearedRegion::clear
//       Access: Public
//  Description: Sets the appropriate region of the image to black.
////////////////////////////////////////////////////////////////////
void PaletteImage::ClearedRegion::
clear(PNMImage &image) {
  for (int y = _y; y < _y + _y_size; y++) {
    for (int x = _x; x < _x + _x_size; x++) {
      image.set_xel_val(x, y, 0);
    }
  }
  if (image.has_alpha()) {
    for (int y = _y; y < _y + _y_size; y++) {
      for (int x = _x; x < _x + _x_size; x++) {
	image.set_alpha_val(x, y, 0);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::ClearedRegion::write_datagram
//       Access: Public
//  Description: Writes the contents of the ClearedRegion to the
//               indicated datagram.
////////////////////////////////////////////////////////////////////
void PaletteImage::ClearedRegion::
write_datagram(Datagram &datagram) const {
  datagram.add_int32(_x);
  datagram.add_int32(_y);
  datagram.add_int32(_x_size);
  datagram.add_int32(_y_size);
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::ClearedRegion::write_datagram
//       Access: Public
//  Description: Extracts the contents of the ClearedRegion from the
//               indicated datagram.
////////////////////////////////////////////////////////////////////
void PaletteImage::ClearedRegion::
fillin(DatagramIterator &scan) {
  _x = scan.get_int32();
  _y = scan.get_int32();
  _x_size = scan.get_int32();
  _y_size = scan.get_int32();
}






////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::Default Constructor
//       Access: Private
//  Description: The default constructor is only for the convenience
//               of the Bam reader.
////////////////////////////////////////////////////////////////////
PaletteImage::
PaletteImage() {
  _page = (PalettePage *)NULL;
  _index = 0;
  _new_image = false;
  _got_image = false;
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
PaletteImage::
PaletteImage(PalettePage *page, int index) :
  _page(page),
  _index(index)
{
  _properties = page->get_properties();
  _size_known = true;
  _x_size = pal->_pal_x_size;
  _y_size = pal->_pal_y_size;
  _new_image = true;
  _got_image = false;

  ostringstream name;
  name << page->get_group()->get_name() << "_palette_" 
       << page->get_name() << "_" << index + 1;

  set_filename(page->get_group(), name.str());
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::get_page
//       Access: Public
//  Description: Returns the particular PalettePage this image is
//               associated with.
////////////////////////////////////////////////////////////////////
PalettePage *PaletteImage::
get_page() const {
  return _page;
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::is_empty
//       Access: Public
//  Description: Returns true if there are no textures, or only one
//               texture, placed on the image.  In either case, the
//               PaletteImage need not be generated.
////////////////////////////////////////////////////////////////////
bool PaletteImage::
is_empty() const {
  return _placements.size() < 2;
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::place
//       Access: Public
//  Description: Attempts to place the indicated texture on the image.
//               Returns true if successful, or false if there was no
//               available space.
////////////////////////////////////////////////////////////////////
bool PaletteImage::
place(TexturePlacement *placement) {
  int x, y;
  if (find_hole(x, y, placement->get_x_size(), placement->get_y_size())) {
    placement->place_at(this, x, y);
    _placements.push_back(placement);
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::unplace
//       Access: Public
//  Description: Removes the texture from the image.
////////////////////////////////////////////////////////////////////
void PaletteImage::
unplace(TexturePlacement *placement) {
  nassertv(placement->is_placed() && placement->get_image() == this);
  
  Placements::iterator pi;
  pi = find(_placements.begin(), _placements.end(), placement);
  if (pi != _placements.end()) {
    _placements.erase(pi);
  }
  
  _cleared_regions.push_back(ClearedRegion(placement));
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::check_solitary
//       Access: Public
//  Description: To be called after all textures have been placed on
//               the image, this checks to see if there is only one
//               texture on the image.  If there is, it is flagged as
//               'solitary' so that the egg files will not needlessly
//               reference the palettized image.
////////////////////////////////////////////////////////////////////
void PaletteImage::
check_solitary() {
  if (_placements.size() == 1) {
    // How sad, only one.
    TexturePlacement *placement = *_placements.begin();
    nassertv(placement->get_omit_reason() == OR_none ||
	     placement->get_omit_reason() == OR_solitary);
    placement->omit_solitary();

  } else {
    // Zero or multiple.
    Placements::const_iterator pi;
    for (pi = _placements.begin(); pi != _placements.end(); ++pi) {
      TexturePlacement *placement = (*pi);
      nassertv(placement->get_omit_reason() == OR_none ||
	       placement->get_omit_reason() == OR_solitary);
      placement->not_solitary();
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::optimal_resize
//       Access: Public
//  Description: Attempts to resize the palette image to as small as
//               it can go.
////////////////////////////////////////////////////////////////////
void PaletteImage::
optimal_resize() {
  if (is_empty()) {
    return;
  }

  bool resized_any = false;
  bool success;
  do {
    success = false;
    nassertv(_x_size > 0 && _y_size > 0);

    // Try to cut it in half in both dimensions, one at a time.
    if (resize_image(_x_size, _y_size / 2)) {
      success = true;
      resized_any = true;
    }
    if (resize_image(_x_size / 2, _y_size)) {
      success = true;
      resized_any = true;
    }
  } while (success);

  if (resized_any) {
    nout << "Resizing " 
	 << FilenameUnifier::make_user_filename(get_filename()) << " to "
	 << _x_size << " " << _y_size << "\n";
  } 
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::resize_image
//       Access: Public
//  Description: Attempts to resize the palette image, and repack all
//               of the textures within the new size.  Returns true if
//               successful, false otherwise.  If this fails, it will
//               still result in repacking all the textures in the
//               original size.
////////////////////////////////////////////////////////////////////
bool PaletteImage::
resize_image(int x_size, int y_size) {
  // We already know we're going to be generating a new image from
  // scratch after this.
  _cleared_regions.clear();
  unlink();
  _new_image = true;

  // First, Save the current placement list, while simultaneously
  // clearing it.
  Placements saved;
  saved.swap(_placements);

  // Also save our current size.
  int saved_x_size = _x_size;
  int saved_y_size = _y_size;

  // Then, sort the textures to in order from biggest to smallest, as
  // an aid to optimal packing.
  sort(saved.begin(), saved.end(), SortPlacementBySize());

  // And while we're at it, we need to officially unplace each of
  // these.
  Placements::iterator pi;
  for (pi = saved.begin(); pi != saved.end(); ++pi) {
    (*pi)->force_replace();
  }

  // Finally, apply the new size and try to fit all the textures.
  _x_size = x_size;
  _y_size = y_size;

  bool packed = true;
  for (pi = saved.begin(); pi != saved.end() && packed; ++pi) {
    if (!place(*pi)) {
      packed = false;
    }
  }

  if (!packed) {
    // If it didn't work, phooey.  Put 'em all back.
    _x_size = saved_x_size;
    _y_size = saved_y_size;
    
    Placements remove;
    remove.swap(_placements);
    for (pi = remove.begin(); pi != remove.end(); ++pi) {
      (*pi)->force_replace();
    }

    bool all_packed = true;
    for (pi = saved.begin(); pi != saved.end(); ++pi) {
      if (!place(*pi)) {
	all_packed = false;
      }
    }
    nassertr(all_packed, false);
  }

  return packed;
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::write_placements
//       Access: Public
//  Description: Writes a list of the textures that have been placed
//               on this image to the indicated output stream, one per
//               line.
////////////////////////////////////////////////////////////////////
void PaletteImage::
write_placements(ostream &out, int indent_level) const {
  Placements::const_iterator pi;
  for (pi = _placements.begin(); pi != _placements.end(); ++pi) {
    TexturePlacement *placement = (*pi);
    placement->write_placed(out, indent_level);
  }
}
 
////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::reset_image
//       Access: Public
//  Description: Unpacks each texture that has been placed on this
//               image, resetting the image to empty.
////////////////////////////////////////////////////////////////////
void PaletteImage::
reset_image() {
  // We need a copy so we can modify this list as we traverse it.
  Placements copy_placements = _placements;
  Placements::const_iterator pi;
  for (pi = copy_placements.begin(); pi != copy_placements.end(); ++pi) {
    TexturePlacement *placement = (*pi);
    placement->force_replace();
  }

  _placements.clear();
  _cleared_regions.clear();
  unlink();
  _new_image = true;
}
 
////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::update_image
//       Access: Public
//  Description: If the palette has changed since it was last written
//               out, updates the image and writes out a new one.  If
//               redo_all is true, regenerates the image from scratch
//               and writes it out again, whether it needed it or not.
////////////////////////////////////////////////////////////////////
void PaletteImage::
update_image(bool redo_all) {
  if (is_empty() && pal->_aggressively_clean_mapdir) {
    // If the palette image is 'empty', ensure that it doesn't exist.
    // No need to clutter up the map directory.
    unlink();
    _new_image = true;
    return;
  }

  if (redo_all) {
    // If we're redoing everything, throw out the old image anyway.
    unlink();
    _new_image = true;
  }

  // Do we need to update?
  bool needs_update =
    _new_image || !exists() ||
    !_cleared_regions.empty();

  Placements::iterator pi;
  for (pi = _placements.begin(); 
       pi != _placements.end() && !needs_update; 
       ++pi) {
    TexturePlacement *placement = (*pi);

    if (!placement->is_filled()) {
      needs_update = true;

    } else {
      SourceTextureImage *source =
	placement->get_texture()->get_preferred_source();

      if (source != (SourceTextureImage *)NULL &&
	  source->get_filename().compare_timestamps(get_filename()) > 0) {
	// The source image is newer than the palette image; we need to
	// regenerate.
	placement->mark_unfilled();
	needs_update = true;
      }
    }
  }

  if (!needs_update) {
    // No sweat; nothing has changed.
    return;
  }

  get_image();

  // Set to black any parts of the image that we recently unplaced.
  ClearedRegions::iterator ci;
  for (ci = _cleared_regions.begin(); ci != _cleared_regions.end(); ++ci) {
    ClearedRegion &region = (*ci);
    region.clear(_image);
  }
  _cleared_regions.clear();

  // Now add the recent additions to the image.
  for (pi = _placements.begin(); pi != _placements.end(); ++pi) {
    TexturePlacement *placement = (*pi);
    if (!placement->is_filled()) {
      placement->fill_image(_image);
    }
  }

  write(_image);
}


////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::find_hole
//       Access: Private
//  Description: Searches for a hole of at least x_size by y_size
//               pixels somewhere within the PaletteImage.  If a
//               suitable hole is found, sets x and y to the top left
//               corner and returns true; otherwise, returns false.
////////////////////////////////////////////////////////////////////
bool PaletteImage::
find_hole(int &x, int &y, int x_size, int y_size) const {
  y = 0;
  while (y + y_size <= _y_size) {
    int next_y = _y_size;
    // Scan along the row at 'y'.
    x = 0;
    while (x + x_size <= _x_size) {
      int next_x = x;

      // Consider the spot at x, y.
      TexturePlacement *overlap = find_overlap(x, y, x_size, y_size);

      if (overlap == (TexturePlacement *)NULL) {
	// Hooray!
	return true;
      }

      next_x = overlap->get_placed_x() + overlap->get_placed_x_size();
      next_y = min(next_y, overlap->get_placed_y() + overlap->get_placed_y_size());
      nassertr(next_x > x, false);
      x = next_x;
    }

    nassertr(next_y > y, false);
    y = next_y;
  }

  // Nope, wouldn't fit anywhere.
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::find_overlap
//       Access: Private
//  Description: If the rectangle whose top left corner is x, y and
//               whose size is x_size, y_size describes an empty hole
//               that does not overlap any placed images, returns
//               NULL; otherwise, returns the first placed texture
//               that the image does overlap.  It is assumed the
//               rectangle lies completely within the boundaries of
//               the image itself.
////////////////////////////////////////////////////////////////////
TexturePlacement *PaletteImage::
find_overlap(int x, int y, int x_size, int y_size) const {
  Placements::const_iterator pi;
  for (pi = _placements.begin(); pi != _placements.end(); ++pi) {
    TexturePlacement *placement = (*pi);
    if (placement->is_placed() && 
	placement->intersects(x, y, x_size, y_size)) {
      return placement;
    }
  }

  return (TexturePlacement *)NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::get_image
//       Access: Public
//  Description: Reads or generates the PNMImage that corresponds to
//               the palette as it is known so far.
////////////////////////////////////////////////////////////////////
void PaletteImage::
get_image() {
  if (_got_image) {
    return;
  }

  if (!_new_image) {
    if (read(_image)) {
      _got_image = true;
      return;
    }
  }

  nout << "Generating new " << FilenameUnifier::make_user_filename(get_filename()) << "\n";

  // We won't be using this any more.
  _cleared_regions.clear();

  _image.clear(get_x_size(), get_y_size(), _properties.get_num_channels());
  _new_image = false;
  _got_image = true;

  // Now fill up the image.
  Placements::iterator pi;
  for (pi = _placements.begin(); pi != _placements.end(); ++pi) {
    TexturePlacement *placement = (*pi);
    placement->fill_image(_image);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::register_with_read_factory
//       Access: Public, Static
//  Description: Registers the current object as something that can be
//               read from a Bam file.
////////////////////////////////////////////////////////////////////
void PaletteImage::
register_with_read_factory() {
  BamReader::get_factory()->
    register_factory(get_class_type(), make_PaletteImage);
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::write_datagram
//       Access: Public, Virtual
//  Description: Fills the indicated datagram up with a binary
//               representation of the current object, in preparation
//               for writing to a Bam file.
////////////////////////////////////////////////////////////////////
void PaletteImage::
write_datagram(BamWriter *writer, Datagram &datagram) {
  ImageFile::write_datagram(writer, datagram);

  datagram.add_uint32(_cleared_regions.size());
  ClearedRegions::const_iterator ci;
  for (ci = _cleared_regions.begin(); ci != _cleared_regions.end(); ++ci) {
    (*ci).write_datagram(datagram);
  }

  datagram.add_uint32(_placements.size());
  Placements::const_iterator pi;
  for (pi = _placements.begin(); pi != _placements.end(); ++pi) {
    writer->write_pointer(datagram, (*pi));
  }

  writer->write_pointer(datagram, _page);
  datagram.add_uint32(_index);
  datagram.add_bool(_new_image);

  // We don't write _got_image or _image.  These are loaded
  // per-session.
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::complete_pointers
//       Access: Public, Virtual
//  Description: Called after the object is otherwise completely read
//               from a Bam file, this function's job is to store the
//               pointers that were retrieved from the Bam file for
//               each pointer object written.  The return value is the
//               number of pointers processed from the list.
////////////////////////////////////////////////////////////////////
int PaletteImage::
complete_pointers(vector_typedWriteable &plist, BamReader *manager) {
  nassertr((int)plist.size() >= _num_placements + 1, 0);
  int index = 0;

  int i;
  _placements.reserve(_num_placements);
  for (i = 0; i < _num_placements; i++) {
    TexturePlacement *placement;
    DCAST_INTO_R(placement, plist[index], index);
    _placements.push_back(placement);
    index++;
  }

  if (plist[index] != (TypedWriteable *)NULL) {
    DCAST_INTO_R(_page, plist[index], index);
  }
  index++;

  return index;
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::make_PaletteImage
//       Access: Protected
//  Description: This method is called by the BamReader when an object
//               of this type is encountered in a Bam file; it should
//               allocate and return a new object with all the data
//               read.
////////////////////////////////////////////////////////////////////
TypedWriteable* PaletteImage::
make_PaletteImage(const FactoryParams &params) {
  PaletteImage *me = new PaletteImage;
  BamReader *manager;
  Datagram packet;

  parse_params(params, manager, packet);
  DatagramIterator scan(packet);

  me->fillin(scan, manager);
  return me;
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteImage::fillin
//       Access: Protected
//  Description: Reads the binary data from the given datagram
//               iterator, which was written by a previous call to
//               write_datagram().
////////////////////////////////////////////////////////////////////
void PaletteImage::
fillin(DatagramIterator &scan, BamReader *manager) {
  ImageFile::fillin(scan, manager);

  int num_cleared_regions = scan.get_uint32();
  _cleared_regions.reserve(num_cleared_regions);
  for (int i = 0; i < num_cleared_regions; i++) {
    _cleared_regions.push_back(ClearedRegion());
    _cleared_regions.back().fillin(scan);
  }

  _num_placements = scan.get_uint32();
  manager->read_pointers(scan, this, _num_placements);

  manager->read_pointer(scan, this);  // _page

  _index = scan.get_uint32();
  _new_image = scan.get_bool();
}
