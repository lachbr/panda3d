// Filename: paletteGroups.cxx
// Created by:  drose (30Nov00)
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

#include "paletteGroups.h"
#include "paletteGroup.h"

#include <indent.h>
#include <datagram.h>
#include <datagramIterator.h>
#include <bamReader.h>
#include <bamWriter.h>

TypeHandle PaletteGroups::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: PaletteGroups::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
PaletteGroups::
PaletteGroups() {
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteGroups::insert
//       Access: Public
//  Description: Inserts a new group to the set, if it is not already
//               there.
////////////////////////////////////////////////////////////////////
void PaletteGroups::
insert(PaletteGroup *group) {
  _groups.insert(group);
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteGroups::count
//       Access: Public
//  Description: Returns the number of times the given group appears
//               in the set.  This is either 1 if it appears at all,
//               or 0 if it does not appear.
////////////////////////////////////////////////////////////////////
PaletteGroups::size_type PaletteGroups::
count(PaletteGroup *group) const {
  return _groups.count(group);
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteGroups::make_complete
//       Access: Public
//  Description: Completes the set with the transitive closure of all
//               dependencies: for each PaletteGroup already in the
//               set a, all of the groups that it depends on are added
//               to the set, and so on.  The indicated set a may be
//               the same as this set.
////////////////////////////////////////////////////////////////////
void PaletteGroups::
make_complete(const PaletteGroups &a) {
  Groups result;

  Groups::const_iterator gi;
  for (gi = a._groups.begin(); gi != a._groups.end(); ++gi) {
    r_make_complete(result, *gi);
  }

  _groups.swap(result);
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteGroups::make_union
//       Access: Public
//  Description: Computes the union of PaletteGroups a and b, and
//               stores the result in this object.  The result may be
//               the same object as either a or b.
////////////////////////////////////////////////////////////////////
void PaletteGroups::
make_union(const PaletteGroups &a, const PaletteGroups &b) {
  Groups u;

  Groups::const_iterator ai, bi;
  ai = a._groups.begin();
  bi = b._groups.begin();

  while (ai != a._groups.end() && bi != b._groups.end()) {
    if ((*ai) < (*bi)) {
      u.insert(u.end(), *ai);
      ++ai;

    } else if ((*bi) < (*ai)) {
      u.insert(u.end(), *bi);
      ++bi;

    } else { // (*ai) == (*bi)
      u.insert(u.end(), *ai);
      ++ai;
      ++bi;
    }
  }

  while (ai != a._groups.end()) {
    u.insert(u.end(), *ai);
    ++ai;
  }

  while (bi != b._groups.end()) {
    u.insert(u.end(), *bi);
    ++bi;
  }

  _groups.swap(u);
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteGroups::make_intersection
//       Access: Public
//  Description: Computes the intersection of PaletteGroups a and b,
//               and stores the result in this object.  The result may
//               be the same object as either a or b.
////////////////////////////////////////////////////////////////////
void PaletteGroups::
make_intersection(const PaletteGroups &a, const PaletteGroups &b) {
  Groups i;

  Groups::const_iterator ai, bi;
  ai = a._groups.begin();
  bi = b._groups.begin();

  while (ai != a._groups.end() && bi != b._groups.end()) {
    if ((*ai) < (*bi)) {
      ++ai;

    } else if ((*bi) < (*ai)) {
      ++bi;

    } else { // (*ai) == (*bi)
      i.insert(i.end(), *ai);
      ++ai;
      ++bi;
    }
  }

  _groups.swap(i);
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteGroups::clear
//       Access: Public
//  Description: Empties the set.
////////////////////////////////////////////////////////////////////
void PaletteGroups::
clear() {
  _groups.clear();
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteGroups::empty
//       Access: Public
//  Description: Returns true if the set is empty, false otherwise.
////////////////////////////////////////////////////////////////////
bool PaletteGroups::
empty() const {
  return _groups.empty();
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteGroups::size
//       Access: Public
//  Description: Returns the number of elements in the set.
////////////////////////////////////////////////////////////////////
PaletteGroups::size_type PaletteGroups::
size() const {
  return _groups.size();
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteGroups::begin
//       Access: Public
//  Description: Returns an iterator suitable for traversing the set.
////////////////////////////////////////////////////////////////////
PaletteGroups::iterator PaletteGroups::
begin() const {
  return _groups.begin();
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteGroups::end
//       Access: Public
//  Description: Returns an iterator suitable for traversing the set.
////////////////////////////////////////////////////////////////////
PaletteGroups::iterator PaletteGroups::
end() const {
  return _groups.end();
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteGroups::output
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
void PaletteGroups::
output(ostream &out) const {
  if (!_groups.empty()) {
    Groups::const_iterator gi = _groups.begin();
    out << (*gi)->get_name();
    ++gi;
    while (gi != _groups.end()) {
      out << " " << (*gi)->get_name();
      ++gi;
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteGroups::write
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
void PaletteGroups::
write(ostream &out, int indent_level) const {
  Groups::const_iterator gi;
  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    indent(out, indent_level) << (*gi)->get_name() << "\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteGroups::r_make_complete
//       Access: Private
//  Description: The recursive implementation of make_complete(), this
//               adds the indicated group and all of its dependencies
//               to the set.
////////////////////////////////////////////////////////////////////
void PaletteGroups::
r_make_complete(PaletteGroups::Groups &result, PaletteGroup *group) {
  bool inserted = result.insert(group).second;

  if (inserted) {
    Groups::const_iterator gi;
    for (gi = group->_dependent._groups.begin();
         gi != group->_dependent._groups.end();
         ++gi) {
      r_make_complete(result, *gi);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteGroups::register_with_read_factory
//       Access: Public, Static
//  Description: Registers the current object as something that can be
//               read from a Bam file.
////////////////////////////////////////////////////////////////////
void PaletteGroups::
register_with_read_factory() {
  BamReader::get_factory()->
    register_factory(get_class_type(), make_PaletteGroups);
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteGroups::write_datagram
//       Access: Public, Virtual
//  Description: Fills the indicated datagram up with a binary
//               representation of the current object, in preparation
//               for writing to a Bam file.
////////////////////////////////////////////////////////////////////
void PaletteGroups::
write_datagram(BamWriter *writer, Datagram &datagram) {
  datagram.add_uint32(_groups.size());

  Groups::const_iterator gi;
  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    writer->write_pointer(datagram, *gi);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteGroups::complete_pointers
//       Access: Public, Virtual
//  Description: Called after the object is otherwise completely read
//               from a Bam file, this function's job is to store the
//               pointers that were retrieved from the Bam file for
//               each pointer object written.  The return value is the
//               number of pointers processed from the list.
////////////////////////////////////////////////////////////////////
int PaletteGroups::
complete_pointers(vector_typedWritable &plist, BamReader *manager) {
  nassertr(_num_groups == (int)plist.size(), 0);
  for (int i = 0; i < _num_groups; i++) {
    PaletteGroup *group;
    DCAST_INTO_R(group, plist[i], i);
    _groups.insert(group);
  }
  return 0;
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteGroups::make_PaletteGroups
//       Access: Protected
//  Description: This method is called by the BamReader when an object
//               of this type is encountered in a Bam file; it should
//               allocate and return a new object with all the data
//               read.
////////////////////////////////////////////////////////////////////
TypedWritable* PaletteGroups::
make_PaletteGroups(const FactoryParams &params) {
  PaletteGroups *me = new PaletteGroups;
  DatagramIterator scan;
  BamReader *manager;

  parse_params(params, scan, manager);
  me->fillin(scan, manager);
  return me;
}

////////////////////////////////////////////////////////////////////
//     Function: PaletteGroups::fillin
//       Access: Protected
//  Description: Reads the binary data from the given datagram
//               iterator, which was written by a previous call to
//               write_datagram().
////////////////////////////////////////////////////////////////////
void PaletteGroups::
fillin(DatagramIterator &scan, BamReader *manager) {
  _num_groups = scan.get_int32();
  manager->read_pointers(scan, this, _num_groups);
}
