// Filename: transparencyAttrib.cxx
// Created by:  drose (28Feb02)
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

#include "transparencyAttrib.h"
#include "graphicsStateGuardianBase.h"
#include "dcast.h"
#include "bamReader.h"
#include "bamWriter.h"
#include "datagram.h"
#include "datagramIterator.h"

TypeHandle TransparencyAttrib::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: TransparencyAttrib::make
//       Access: Published, Static
//  Description: Constructs a new TransparencyAttrib object.
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) TransparencyAttrib::
make(TransparencyAttrib::Mode mode) {
  TransparencyAttrib *attrib = new TransparencyAttrib(mode);
  return return_new(attrib);
}

////////////////////////////////////////////////////////////////////
//     Function: TransparencyAttrib::issue
//       Access: Public, Virtual
//  Description: Calls the appropriate method on the indicated GSG
//               to issue the graphics commands appropriate to the
//               given attribute.  This is normally called
//               (indirectly) only from
//               GraphicsStateGuardian::set_state() or modify_state().
////////////////////////////////////////////////////////////////////
void TransparencyAttrib::
issue(GraphicsStateGuardianBase *gsg) const {
  gsg->issue_transparency(this);
}

////////////////////////////////////////////////////////////////////
//     Function: TransparencyAttrib::output
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void TransparencyAttrib::
output(ostream &out) const {
  out << get_type() << ":";
  switch (get_mode()) {
  case M_none:
    out << "none";
    break;

  case M_alpha:
    out << "alpha";
    break;

  case M_alpha_sorted:
    out << "alpha sorted";
    break;

  case M_multisample:
    out << "multisample";
    break;

  case M_multisample_mask:
    out << "multisample mask";
    break;

  case M_binary:
    out << "binary";
    break;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TransparencyAttrib::compare_to_impl
//       Access: Protected, Virtual
//  Description: Intended to be overridden by derived TransparencyAttrib
//               types to return a unique number indicating whether
//               this TransparencyAttrib is equivalent to the other one.
//
//               This should return 0 if the two TransparencyAttrib objects
//               are equivalent, a number less than zero if this one
//               should be sorted before the other one, and a number
//               greater than zero otherwise.
//
//               This will only be called with two TransparencyAttrib
//               objects whose get_type() functions return the same.
////////////////////////////////////////////////////////////////////
int TransparencyAttrib::
compare_to_impl(const RenderAttrib *other) const {
  const TransparencyAttrib *ta;
  DCAST_INTO_R(ta, other, 0);
  return (int)_mode - (int)ta->_mode;
}

////////////////////////////////////////////////////////////////////
//     Function: TransparencyAttrib::make_default_impl
//       Access: Protected, Virtual
//  Description: Intended to be overridden by derived TransparencyAttrib
//               types to specify what the default property for a
//               TransparencyAttrib of this type should be.
//
//               This should return a newly-allocated TransparencyAttrib of
//               the same type that corresponds to whatever the
//               standard default for this kind of TransparencyAttrib is.
////////////////////////////////////////////////////////////////////
RenderAttrib *TransparencyAttrib::
make_default_impl() const {
  return new TransparencyAttrib;
}

////////////////////////////////////////////////////////////////////
//     Function: TransparencyAttrib::register_with_read_factory
//       Access: Public, Static
//  Description: Tells the BamReader how to create objects of type
//               TransparencyAttrib.
////////////////////////////////////////////////////////////////////
void TransparencyAttrib::
register_with_read_factory() {
  BamReader::get_factory()->register_factory(get_class_type(), make_from_bam);
}

////////////////////////////////////////////////////////////////////
//     Function: TransparencyAttrib::write_datagram
//       Access: Public, Virtual
//  Description: Writes the contents of this object to the datagram
//               for shipping out to a Bam file.
////////////////////////////////////////////////////////////////////
void TransparencyAttrib::
write_datagram(BamWriter *manager, Datagram &dg) {
  RenderAttrib::write_datagram(manager, dg);

  dg.add_int8(_mode);
}

////////////////////////////////////////////////////////////////////
//     Function: TransparencyAttrib::make_from_bam
//       Access: Protected, Static
//  Description: This function is called by the BamReader's factory
//               when a new object of type TransparencyAttrib is encountered
//               in the Bam file.  It should create the TransparencyAttrib
//               and extract its information from the file.
////////////////////////////////////////////////////////////////////
TypedWritable *TransparencyAttrib::
make_from_bam(const FactoryParams &params) {
  TransparencyAttrib *attrib = new TransparencyAttrib;
  DatagramIterator scan;
  BamReader *manager;

  parse_params(params, scan, manager);
  attrib->fillin(scan, manager);

  return attrib;
}

////////////////////////////////////////////////////////////////////
//     Function: TransparencyAttrib::fillin
//       Access: Protected
//  Description: This internal function is called by make_from_bam to
//               read in all of the relevant data from the BamFile for
//               the new TransparencyAttrib.
////////////////////////////////////////////////////////////////////
void TransparencyAttrib::
fillin(DatagramIterator &scan, BamReader *manager) {
  RenderAttrib::fillin(scan, manager);

  _mode = (Mode)scan.get_int8();
}
