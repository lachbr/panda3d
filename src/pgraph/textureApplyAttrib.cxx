// Filename: textureApplyAttrib.cxx
// Created by:  drose (04Mar02)
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

#include "textureApplyAttrib.h"
#include "graphicsStateGuardianBase.h"
#include "dcast.h"
#include "bamReader.h"
#include "bamWriter.h"
#include "datagram.h"
#include "datagramIterator.h"

TypeHandle TextureApplyAttrib::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: TextureApplyAttrib::make
//       Access: Published, Static
//  Description: Constructs a new TextureApplyAttrib object that specifies
//               how to cull geometry.  By Panda convention, vertices
//               are ordered counterclockwise when seen from the
//               front, so the M_cull_clockwise will cull backfacing
//               polygons.
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) TextureApplyAttrib::
make(TextureApplyAttrib::Mode mode) {
  TextureApplyAttrib *attrib = new TextureApplyAttrib(mode);
  return return_new(attrib);
}

////////////////////////////////////////////////////////////////////
//     Function: TextureApplyAttrib::issue
//       Access: Public, Virtual
//  Description: Calls the appropriate method on the indicated GSG
//               to issue the graphics commands appropriate to the
//               given attribute.  This is normally called
//               (indirectly) only from
//               GraphicsStateGuardian::set_state() or modify_state().
////////////////////////////////////////////////////////////////////
void TextureApplyAttrib::
issue(GraphicsStateGuardianBase *gsg) const {
  gsg->issue_texture_apply(this);
}

////////////////////////////////////////////////////////////////////
//     Function: TextureApplyAttrib::output
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void TextureApplyAttrib::
output(ostream &out) const {
  out << get_type() << ":";
  switch (get_mode()) {
  case M_modulate:
    out << "modulate";
    break;

  case M_decal:
    out << "decal";
    break;

  case M_blend:
    out << "blend";
    break;

  case M_replace:
    out << "replace";
    break;

  case M_add:
    out << "add";
    break;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TextureApplyAttrib::compare_to_impl
//       Access: Protected, Virtual
//  Description: Intended to be overridden by derived TextureApplyAttrib
//               types to return a unique number indicating whether
//               this TextureApplyAttrib is equivalent to the other one.
//
//               This should return 0 if the two TextureApplyAttrib objects
//               are equivalent, a number less than zero if this one
//               should be sorted before the other one, and a number
//               greater than zero otherwise.
//
//               This will only be called with two TextureApplyAttrib
//               objects whose get_type() functions return the same.
////////////////////////////////////////////////////////////////////
int TextureApplyAttrib::
compare_to_impl(const RenderAttrib *other) const {
  const TextureApplyAttrib *ta;
  DCAST_INTO_R(ta, other, 0);
  return (int)_mode - (int)ta->_mode;
}

////////////////////////////////////////////////////////////////////
//     Function: TextureApplyAttrib::make_default_impl
//       Access: Protected, Virtual
//  Description: Intended to be overridden by derived TextureApplyAttrib
//               types to specify what the default property for a
//               TextureApplyAttrib of this type should be.
//
//               This should return a newly-allocated TextureApplyAttrib of
//               the same type that corresponds to whatever the
//               standard default for this kind of TextureApplyAttrib is.
////////////////////////////////////////////////////////////////////
RenderAttrib *TextureApplyAttrib::
make_default_impl() const {
  return new TextureApplyAttrib;
}

////////////////////////////////////////////////////////////////////
//     Function: TextureApplyAttrib::register_with_read_factory
//       Access: Public, Static
//  Description: Tells the BamReader how to create objects of type
//               TextureApplyAttrib.
////////////////////////////////////////////////////////////////////
void TextureApplyAttrib::
register_with_read_factory() {
  BamReader::get_factory()->register_factory(get_class_type(), make_from_bam);
}

////////////////////////////////////////////////////////////////////
//     Function: TextureApplyAttrib::write_datagram
//       Access: Public, Virtual
//  Description: Writes the contents of this object to the datagram
//               for shipping out to a Bam file.
////////////////////////////////////////////////////////////////////
void TextureApplyAttrib::
write_datagram(BamWriter *manager, Datagram &dg) {
  RenderAttrib::write_datagram(manager, dg);

  dg.add_int8(_mode);
}

////////////////////////////////////////////////////////////////////
//     Function: TextureApplyAttrib::make_from_bam
//       Access: Protected, Static
//  Description: This function is called by the BamReader's factory
//               when a new object of type TextureApplyAttrib is encountered
//               in the Bam file.  It should create the TextureApplyAttrib
//               and extract its information from the file.
////////////////////////////////////////////////////////////////////
TypedWritable *TextureApplyAttrib::
make_from_bam(const FactoryParams &params) {
  TextureApplyAttrib *attrib = new TextureApplyAttrib;
  DatagramIterator scan;
  BamReader *manager;

  parse_params(params, scan, manager);
  attrib->fillin(scan, manager);

  return attrib;
}

////////////////////////////////////////////////////////////////////
//     Function: TextureApplyAttrib::fillin
//       Access: Protected
//  Description: This internal function is called by make_from_bam to
//               read in all of the relevant data from the BamFile for
//               the new TextureApplyAttrib.
////////////////////////////////////////////////////////////////////
void TextureApplyAttrib::
fillin(DatagramIterator &scan, BamReader *manager) {
  RenderAttrib::fillin(scan, manager);

  _mode = (Mode)scan.get_int8();
}
