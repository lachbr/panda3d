// Filename: animChannelBase.cxx
// Created by:  drose (19Feb99)
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

#include "chan_headers.h"
#pragma hdrstop

TypeHandle AnimChannelBase::_type_handle;


////////////////////////////////////////////////////////////////////
//     Function: AnimChannelBase::has_changed
//       Access: Public, Virtual
//  Description: Returns true if the value has changed since the last
//               call to has_changed().  last_frame is the frame
//               number of the last call; this_frame is the current
//               frame number.
////////////////////////////////////////////////////////////////////
bool AnimChannelBase::
has_changed(int, int) {
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: AnimChannelBase::output
//       Access: Public, Virtual
//  Description: Writes a one-line description of the channel.
////////////////////////////////////////////////////////////////////
void AnimChannelBase::
output(ostream &out) const {
  out << get_type() << "(" << get_value_type() << ") " << get_name();
}

////////////////////////////////////////////////////////////////////
//     Function: AnimChannelBase::write_datagram
//       Access: Public
//  Description: Function to write the important information in
//               the particular object to a Datagram
////////////////////////////////////////////////////////////////////
void AnimChannelBase::
write_datagram(BamWriter *manager, Datagram &me)
{
  AnimGroup::write_datagram(manager, me);
  me.add_uint16(_last_frame);
}

////////////////////////////////////////////////////////////////////
//     Function: AnimChannelBase::fillin
//       Access: Protected
//  Description: Function that reads out of the datagram (or asks
//               manager to read) all of the data that is needed to
//               re-create this object and stores it in the appropiate
//               place
////////////////////////////////////////////////////////////////////
void AnimChannelBase::
fillin(DatagramIterator& scan, BamReader* manager)
{
  AnimGroup::fillin(scan, manager);
  _last_frame = scan.get_uint16();
}


