// Filename: movingPartBase.cxx
// Created by:  drose (22Feb99)
// 
////////////////////////////////////////////////////////////////////

#include "movingPartBase.h"
#include "animControl.h"
#include "animChannelBase.h"

#include <indent.h>

TypeHandle MovingPartBase::_type_handle;


////////////////////////////////////////////////////////////////////
//     Function: MovingPartBase::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
MovingPartBase::
MovingPartBase(PartGroup *parent, const string &name)
  : PartGroup(parent, name) {
}

////////////////////////////////////////////////////////////////////
//     Function: MovingPartBase::Constructor
//       Access: Protected
//  Description: 
////////////////////////////////////////////////////////////////////
MovingPartBase::
MovingPartBase(void){
}

////////////////////////////////////////////////////////////////////
//     Function: MovingPartBase::write
//       Access: Public, Virtual
//  Description: Writes a brief description of the channel and all of
//               its descendants.
////////////////////////////////////////////////////////////////////
void MovingPartBase::
write(ostream &out, int indent_level) const {
  indent(out, indent_level) << get_value_type() << " " << get_name();
  if (_children.empty()) {
    out << "\n";
  } else {
    out << " {\n";
    write_descendants(out, indent_level + 2);
    indent(out, indent_level) << "}\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: MovingPartBase::write_with_value
//       Access: Public, Virtual
//  Description: Writes a brief description of the channel and all of
//               its descendants, along with their values.
////////////////////////////////////////////////////////////////////
void MovingPartBase::
write_with_value(ostream &out, int indent_level) const {
  indent(out, indent_level) << get_value_type() << " " << get_name() << "\n";
  indent(out, indent_level);
  output_value(out);

  if (_children.empty()) {
    out << "\n";
  } else {
    out << " {\n";
    write_descendants_with_value(out, indent_level + 2);
    indent(out, indent_level) << "}\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: MovingPartBase::do_update
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void MovingPartBase::
do_update(PartBundle *root, PartGroup *parent,
          bool parent_changed, bool anim_changed) {
  bool needs_update = anim_changed;

  // See if any of the channel values have changed since last time.

  PartBundle::control_iterator bci;
  for (bci = root->control_begin();
       !needs_update && bci != root->control_end();
       ++bci) {
    AnimControl *control = (*bci);
    int channel_index = control->get_channel_index();
    nassertv(channel_index >= 0 && channel_index < (int)_channels.size());
    AnimChannelBase *channel = _channels[channel_index];
    nassertv(channel != (AnimChannelBase*)0L);

    needs_update = control->channel_has_changed(channel);
  }

  if (needs_update) {
    // Ok, get the latest value.
    get_blend_value(root);
  }

  if (parent_changed || needs_update) {
    update_internals(parent, needs_update, parent_changed);
  }

  // Now recurse.
  Children::iterator ci;
  for (ci = _children.begin(); ci != _children.end(); ++ci) {
    (*ci)->do_update(root, this, parent_changed || needs_update,
                     anim_changed);
  }
}


////////////////////////////////////////////////////////////////////
//     Function: MovingPartBase::update_internals
//       Access: Public, Virtual
//  Description: This is called by do_update() whenever the part or
//               some ancestor has changed values.  It is a hook for
//               derived classes to update whatever cache they may
//               have that depends on these.
////////////////////////////////////////////////////////////////////
void MovingPartBase::
update_internals(PartGroup *, bool, bool) {
}

////////////////////////////////////////////////////////////////////
//     Function: MovingPartBase::pick_channel_index
//       Access: Protected
//  Description: Walks the part hierarchy, looking for a suitable
//               channel index number to use.  Available index numbers
//               are the elements of the holes set, as well as next to
//               infinity.
////////////////////////////////////////////////////////////////////
void MovingPartBase::
pick_channel_index(list<int> &holes, int &next) const {
  // Verify each of the holes.

  list<int>::iterator ii, ii_next;
  ii = holes.begin();
  while (ii != holes.end()) {
    ii_next = ii;
    ++ii_next;

    int hole = (*ii);
    nassertv(hole >= 0 && hole < next);
    if (hole < (int)_channels.size() || 
        _channels[hole] != (AnimChannelBase *)NULL) {
      // We can't accept this hole; we're using it!
      holes.erase(ii);
    }
    ii = ii_next;
  }

  // Now do we have any more to restrict?
  if (next < (int)_channels.size()) {
    int i;
    for (i = next; i < (int)_channels.size(); i++) {
      if (_channels[i] == (AnimChannelBase*)0L) {
        // Here's a hole we do have.
        holes.push_back(i);
      }
    }
    next = _channels.size();
  }

  PartGroup::pick_channel_index(holes, next);
}



////////////////////////////////////////////////////////////////////
//     Function: MovingPartBase::bind_hierarchy
//       Access: Protected, Virtual
//  Description: Binds the indicated anim hierarchy to the part
//               hierarchy, at the given channel index number.
////////////////////////////////////////////////////////////////////
void MovingPartBase::
bind_hierarchy(AnimGroup *anim, int channel_index) {
  while ((int)_channels.size() <= channel_index) {
    _channels.push_back((AnimChannelBase*)0L);
  }

  nassertv(_channels[channel_index] == (AnimChannelBase*)0L);

  if (anim == (AnimGroup*)0L) {
    // If we're binding to the NULL anim, it means actually to create
    // a default AnimChannel that just returns the part's initial
    // value.
    _channels[channel_index] = make_initial_channel();
  } else {
    _channels[channel_index] = DCAST(AnimChannelBase, anim);
  }

  PartGroup::bind_hierarchy(anim, channel_index);
}
