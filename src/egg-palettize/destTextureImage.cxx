// Filename: destTextureImage.cxx
// Created by:  drose (05Dec00)
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

#include "destTextureImage.h"
#include "sourceTextureImage.h"
#include "texturePlacement.h"
#include "textureImage.h"
#include "palettizer.h"

#include <datagram.h>
#include <datagramIterator.h>
#include <bamReader.h>
#include <bamWriter.h>

TypeHandle DestTextureImage::_type_handle;


////////////////////////////////////////////////////////////////////
//     Function: DestTextureImage::Default Constructor
//       Access: Private
//  Description: The default constructor is only for the convenience
//               of the Bam reader.
////////////////////////////////////////////////////////////////////
DestTextureImage::
DestTextureImage() {
}

////////////////////////////////////////////////////////////////////
//     Function: DestTextureImage::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
DestTextureImage::
DestTextureImage(TexturePlacement *placement) {
  TextureImage *texture = placement->get_texture();
  _properties = texture->get_properties();
  _size_known = true;
  _x_size = texture->get_x_size();
  _y_size = texture->get_y_size();

  if (pal->_force_power_2) {
    _x_size = to_power_2(_x_size);
    _y_size = to_power_2(_y_size);
  }

  set_filename(placement->get_group(), texture->get_name());
}

////////////////////////////////////////////////////////////////////
//     Function: DestTextureImage::copy
//       Access: Public
//  Description: Unconditionally copies the source texture into the
//               appropriate filename.
////////////////////////////////////////////////////////////////////
void DestTextureImage::
copy(TextureImage *texture) {
  const PNMImage &source_image = texture->read_source_image();
  PNMImage dest_image(_x_size, _y_size, texture->get_num_channels(),
                      source_image.get_maxval());
  dest_image.quick_filter_from(source_image);
  write(dest_image);
}

////////////////////////////////////////////////////////////////////
//     Function: DestTextureImage::copy_if_stale
//       Access: Public
//  Description: Copies the source texture into the appropriate
//               filename only if the indicated old reference, which
//               represents the way it was last copied, is now
//               out-of-date.
////////////////////////////////////////////////////////////////////
void DestTextureImage::
copy_if_stale(const DestTextureImage *other, TextureImage *texture) {
  if (other->get_x_size() != get_x_size() ||
      other->get_y_size() != get_y_size() ||
      other->get_num_channels() != get_num_channels()) {
    copy(texture);

  } else {
    // Also check the timestamps.
    SourceTextureImage *source = texture->get_preferred_source();

    if (source != (SourceTextureImage *)NULL &&
        source->get_filename().compare_timestamps(get_filename()) > 0) {
      copy(texture);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: DestTextureImage::to_power_2
//       Access: Private, Static
//  Description: Returns the largest power of 2 less than or equal to
//               value.
////////////////////////////////////////////////////////////////////
int DestTextureImage::
to_power_2(int value) {
  int x = 1;
  while ((x << 1) <= value) {
    x = (x << 1);
  }
  return x;
}

////////////////////////////////////////////////////////////////////
//     Function: DestTextureImage::register_with_read_factory
//       Access: Public, Static
//  Description: Registers the current object as something that can be
//               read from a Bam file.
////////////////////////////////////////////////////////////////////
void DestTextureImage::
register_with_read_factory() {
  BamReader::get_factory()->
    register_factory(get_class_type(), make_DestTextureImage);
}

////////////////////////////////////////////////////////////////////
//     Function: DestTextureImage::write_datagram
//       Access: Public, Virtual
//  Description: Fills the indicated datagram up with a binary
//               representation of the current object, in preparation
//               for writing to a Bam file.
////////////////////////////////////////////////////////////////////
void DestTextureImage::
write_datagram(BamWriter *writer, Datagram &datagram) {
  ImageFile::write_datagram(writer, datagram);
}

////////////////////////////////////////////////////////////////////
//     Function: DestTextureImage::make_DestTextureImage
//       Access: Protected
//  Description: This method is called by the BamReader when an object
//               of this type is encountered in a Bam file; it should
//               allocate and return a new object with all the data
//               read.
////////////////////////////////////////////////////////////////////
TypedWritable* DestTextureImage::
make_DestTextureImage(const FactoryParams &params) {
  DestTextureImage *me = new DestTextureImage;
  DatagramIterator scan;
  BamReader *manager;

  parse_params(params, scan, manager);
  me->fillin(scan, manager);
  return me;
}

////////////////////////////////////////////////////////////////////
//     Function: DestTextureImage::fillin
//       Access: Protected
//  Description: Reads the binary data from the given datagram
//               iterator, which was written by a previous call to
//               write_datagram().
////////////////////////////////////////////////////////////////////
void DestTextureImage::
fillin(DatagramIterator &scan, BamReader *manager) {
  ImageFile::fillin(scan, manager);
}
