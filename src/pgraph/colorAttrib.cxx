// Filename: colorAttrib.cxx
// Created by:  drose (22Feb02)
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

#include "colorAttrib.h"
#include "graphicsStateGuardianBase.h"
#include "dcast.h"
#include "bamReader.h"
#include "bamWriter.h"
#include "datagram.h"
#include "datagramIterator.h"

TypeHandle ColorAttrib::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: ColorAttrib::make_vertex
//       Access: Published, Static
//  Description: Constructs a new ColorAttrib object that indicates
//               geometry should be rendered according to its own
//               vertex color.
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ColorAttrib::
make_vertex() {
  ColorAttrib *attrib = new ColorAttrib(T_vertex);
  return return_new(attrib);
}

////////////////////////////////////////////////////////////////////
//     Function: ColorAttrib::make_flat
//       Access: Published, Static
//  Description: Constructs a new ColorAttrib object that indicates
//               geometry should be rendered in the indicated color.
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ColorAttrib::
make_flat(const Colorf &color) {
  ColorAttrib *attrib = new ColorAttrib(T_flat, color);
  return return_new(attrib);
}

////////////////////////////////////////////////////////////////////
//     Function: ColorAttrib::make_off
//       Access: Published, Static
//  Description: Constructs a new ColorAttrib object that indicates
//               geometry should be rendered without any color
//               commands at all.
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ColorAttrib::
make_off() {
  ColorAttrib *attrib = new ColorAttrib(T_off);
  return return_new(attrib);
}

////////////////////////////////////////////////////////////////////
//     Function: ColorAttrib::issue
//       Access: Public, Virtual
//  Description: Calls the appropriate method on the indicated GSG
//               to issue the graphics commands appropriate to the
//               given attribute.  This is normally called
//               (indirectly) only from
//               GraphicsStateGuardian::set_state() or modify_state().
////////////////////////////////////////////////////////////////////
void ColorAttrib::
issue(GraphicsStateGuardianBase *gsg) const {
  gsg->issue_color(this);
}

////////////////////////////////////////////////////////////////////
//     Function: ColorAttrib::output
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void ColorAttrib::
output(ostream &out) const {
  out << get_type() << ":";
  switch (get_color_type()) {
  case T_vertex:
    out << "vertex";
    break;

  case T_flat:
    out << "(" << get_color() << ")";
    break;

  case T_off:
    out << "off";
    break;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: ColorAttrib::compare_to_impl
//       Access: Protected, Virtual
//  Description: Intended to be overridden by derived ColorAttrib
//               types to return a unique number indicating whether
//               this ColorAttrib is equivalent to the other one.
//
//               This should return 0 if the two ColorAttrib objects
//               are equivalent, a number less than zero if this one
//               should be sorted before the other one, and a number
//               greater than zero otherwise.
//
//               This will only be called with two ColorAttrib
//               objects whose get_type() functions return the same.
////////////////////////////////////////////////////////////////////
int ColorAttrib::
compare_to_impl(const RenderAttrib *other) const {
  const ColorAttrib *ta;
  DCAST_INTO_R(ta, other, 0);
  if (_type != ta->_type) {
    return (int)_type - (int)ta->_type;
  }
  if (_type == T_flat) {
    return _color.compare_to(ta->_color);
  }
  return 0;
}

////////////////////////////////////////////////////////////////////
//     Function: ColorAttrib::make_default_impl
//       Access: Protected, Virtual
//  Description: Intended to be overridden by derived ColorAttrib
//               types to specify what the default property for a
//               ColorAttrib of this type should be.
//
//               This should return a newly-allocated ColorAttrib of
//               the same type that corresponds to whatever the
//               standard default for this kind of ColorAttrib is.
////////////////////////////////////////////////////////////////////
RenderAttrib *ColorAttrib::
make_default_impl() const {
  return new ColorAttrib;
}

////////////////////////////////////////////////////////////////////
//     Function: ColorAttrib::register_with_read_factory
//       Access: Public, Static
//  Description: Tells the BamReader how to create objects of type
//               ColorAttrib.
////////////////////////////////////////////////////////////////////
void ColorAttrib::
register_with_read_factory() {
  BamReader::get_factory()->register_factory(get_class_type(), make_from_bam);
}

////////////////////////////////////////////////////////////////////
//     Function: ColorAttrib::write_datagram
//       Access: Public, Virtual
//  Description: Writes the contents of this object to the datagram
//               for shipping out to a Bam file.
////////////////////////////////////////////////////////////////////
void ColorAttrib::
write_datagram(BamWriter *manager, Datagram &dg) {
  RenderAttrib::write_datagram(manager, dg);

  dg.add_int8(_type);
  _color.write_datagram(dg);
}

////////////////////////////////////////////////////////////////////
//     Function: ColorAttrib::make_from_bam
//       Access: Protected, Static
//  Description: This function is called by the BamReader's factory
//               when a new object of type ColorAttrib is encountered
//               in the Bam file.  It should create the ColorAttrib
//               and extract its information from the file.
////////////////////////////////////////////////////////////////////
TypedWritable *ColorAttrib::
make_from_bam(const FactoryParams &params) {
  ColorAttrib *attrib = new ColorAttrib;
  DatagramIterator scan;
  BamReader *manager;

  parse_params(params, scan, manager);
  attrib->fillin(scan, manager);

  return attrib;
}

////////////////////////////////////////////////////////////////////
//     Function: ColorAttrib::fillin
//       Access: Protected
//  Description: This internal function is called by make_from_bam to
//               read in all of the relevant data from the BamFile for
//               the new ColorAttrib.
////////////////////////////////////////////////////////////////////
void ColorAttrib::
fillin(DatagramIterator &scan, BamReader *manager) {
  RenderAttrib::fillin(scan, manager);

  _type = (Type)scan.get_int8();
  _color.read_datagram(scan);
}
