// Filename: depthTestAttrib.cxx
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

#include "depthTestAttrib.h"
#include "graphicsStateGuardianBase.h"
#include "dcast.h"
#include "bamReader.h"
#include "bamWriter.h"
#include "datagram.h"
#include "datagramIterator.h"

TypeHandle DepthTestAttrib::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: DepthTestAttrib::make
//       Access: Published, Static
//  Description: Constructs a new DepthTestAttrib object that specifies
//               how to cull geometry.  By Panda convention, vertices
//               are ordered counterclockwise when seen from the
//               front, so the M_cull_clockwise will cull backfacing
//               polygons.
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) DepthTestAttrib::
make(DepthTestAttrib::Mode mode) {
  DepthTestAttrib *attrib = new DepthTestAttrib(mode);
  return return_new(attrib);
}

////////////////////////////////////////////////////////////////////
//     Function: DepthTestAttrib::issue
//       Access: Public, Virtual
//  Description: Calls the appropriate method on the indicated GSG
//               to issue the graphics commands appropriate to the
//               given attribute.  This is normally called
//               (indirectly) only from
//               GraphicsStateGuardian::set_state() or modify_state().
////////////////////////////////////////////////////////////////////
void DepthTestAttrib::
issue(GraphicsStateGuardianBase *gsg) const {
  gsg->issue_depth_test(this);
}

////////////////////////////////////////////////////////////////////
//     Function: DepthTestAttrib::output
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void DepthTestAttrib::
output(ostream &out) const {
  out << get_type() << ":";
  switch (get_mode()) {
  case M_none:
    out << "none";
    break;

  case M_never:
    out << "never";
    break;

  case M_less:
    out << "less";
    break;

  case M_equal:
    out << "equal";
    break;

  case M_less_equal:
    out << "less_equal";
    break;

  case M_greater:
    out << "greater";
    break;

  case M_not_equal:
    out << "not_equal";
    break;

  case M_greater_equal:
    out << "greater_equal";
    break;

  case M_always:
    out << "always";
    break;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: DepthTestAttrib::compare_to_impl
//       Access: Protected, Virtual
//  Description: Intended to be overridden by derived DepthTestAttrib
//               types to return a unique number indicating whether
//               this DepthTestAttrib is equivalent to the other one.
//
//               This should return 0 if the two DepthTestAttrib objects
//               are equivalent, a number less than zero if this one
//               should be sorted before the other one, and a number
//               greater than zero otherwise.
//
//               This will only be called with two DepthTestAttrib
//               objects whose get_type() functions return the same.
////////////////////////////////////////////////////////////////////
int DepthTestAttrib::
compare_to_impl(const RenderAttrib *other) const {
  const DepthTestAttrib *ta;
  DCAST_INTO_R(ta, other, 0);
  return (int)_mode - (int)ta->_mode;
}

////////////////////////////////////////////////////////////////////
//     Function: DepthTestAttrib::make_default_impl
//       Access: Protected, Virtual
//  Description: Intended to be overridden by derived DepthTestAttrib
//               types to specify what the default property for a
//               DepthTestAttrib of this type should be.
//
//               This should return a newly-allocated DepthTestAttrib of
//               the same type that corresponds to whatever the
//               standard default for this kind of DepthTestAttrib is.
////////////////////////////////////////////////////////////////////
RenderAttrib *DepthTestAttrib::
make_default_impl() const {
  return new DepthTestAttrib;
}

////////////////////////////////////////////////////////////////////
//     Function: DepthTestAttrib::register_with_read_factory
//       Access: Public, Static
//  Description: Tells the BamReader how to create objects of type
//               DepthTestAttrib.
////////////////////////////////////////////////////////////////////
void DepthTestAttrib::
register_with_read_factory() {
  BamReader::get_factory()->register_factory(get_class_type(), make_from_bam);
}

////////////////////////////////////////////////////////////////////
//     Function: DepthTestAttrib::write_datagram
//       Access: Public, Virtual
//  Description: Writes the contents of this object to the datagram
//               for shipping out to a Bam file.
////////////////////////////////////////////////////////////////////
void DepthTestAttrib::
write_datagram(BamWriter *manager, Datagram &dg) {
  RenderAttrib::write_datagram(manager, dg);

  dg.add_int8(_mode);
}

////////////////////////////////////////////////////////////////////
//     Function: DepthTestAttrib::make_from_bam
//       Access: Protected, Static
//  Description: This function is called by the BamReader's factory
//               when a new object of type DepthTestAttrib is encountered
//               in the Bam file.  It should create the DepthTestAttrib
//               and extract its information from the file.
////////////////////////////////////////////////////////////////////
TypedWritable *DepthTestAttrib::
make_from_bam(const FactoryParams &params) {
  DepthTestAttrib *attrib = new DepthTestAttrib;
  DatagramIterator scan;
  BamReader *manager;

  parse_params(params, scan, manager);
  attrib->fillin(scan, manager);

  return attrib;
}

////////////////////////////////////////////////////////////////////
//     Function: DepthTestAttrib::fillin
//       Access: Protected
//  Description: This internal function is called by make_from_bam to
//               read in all of the relevant data from the BamFile for
//               the new DepthTestAttrib.
////////////////////////////////////////////////////////////////////
void DepthTestAttrib::
fillin(DatagramIterator &scan, BamReader *manager) {
  RenderAttrib::fillin(scan, manager);

  _mode = (Mode)scan.get_int8();
}
