// Filename: pgItem.cxx
// Created by:  drose (13Mar02)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001 - 2004, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://etc.cmu.edu/panda3d/docs/license/ .
//
// To contact the maintainers of this program write to
// panda3d-general@lists.sourceforge.net .
//
////////////////////////////////////////////////////////////////////

#include "pgItem.h"
#include "pgMouseWatcherParameter.h"
#include "pgCullTraverser.h"
#include "config_pgui.h"

#include "pandaNode.h"
#include "sceneGraphReducer.h"
#include "throw_event.h"
#include "string_utils.h"
#include "nodePath.h"
#include "cullTraverser.h"
#include "cullTraverserData.h"
#include "cullBinManager.h"
#include "dcast.h"

#ifdef HAVE_AUDIO
#include "audioSound.h"
#endif

TypeHandle PGItem::_type_handle;
PT(TextNode) PGItem::_text_node;
PGItem *PGItem::_focus_item = (PGItem *)NULL;
PGItem::BackgroundFocus PGItem::_background_focus;

////////////////////////////////////////////////////////////////////
//     Function: PGItem::Constructor
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
PGItem::
PGItem(const string &name) : 
  PandaNode(name)
{
  _has_frame = false;
  _frame.set(0, 0, 0, 0);
  _region = new PGMouseWatcherRegion(this);
  _state = 0;
  _flags = 0;
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::Destructor
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
PGItem::
~PGItem() {
  nassertv(_region->_item == this);
  _region->_item = (PGItem *)NULL;

  set_background_focus(false);
  if (_focus_item == this) {
    _focus_item = (PGItem *)NULL;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::Copy Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
PGItem::
PGItem(const PGItem &copy) :
  PandaNode(copy),
  _has_frame(copy._has_frame),
  _frame(copy._frame),
  _state(copy._state),
  _flags(copy._flags)
{
  _region = new PGMouseWatcherRegion(this);
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::make_copy
//       Access: Protected, Virtual
//  Description: Returns a newly-allocated Node that is a shallow copy
//               of this one.  It will be a different Node pointer,
//               but its internal data may or may not be shared with
//               that of the original Node.
////////////////////////////////////////////////////////////////////
PandaNode *PGItem::
make_copy() const {
  return new PGItem(*this);
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::xform
//       Access: Protected, Virtual
//  Description: Transforms the contents of this node by the indicated
//               matrix, if it means anything to do so.  For most
//               kinds of nodes, this does nothing.
////////////////////////////////////////////////////////////////////
void PGItem::
xform(const LMatrix4f &mat) {
  // Transform the frame.
  LPoint3f ll(_frame[0], 0.0f, _frame[2]);
  LPoint3f ur(_frame[1], 0.0f, _frame[3]);
  ll = ll * mat;
  ur = ur * mat;
  _frame.set(ll[0], ur[0], ll[2], ur[2]);

  // Transform the individual states and their frame styles.
  StateDefs::iterator di;
  for (di = _state_defs.begin(); di != _state_defs.end(); ++di) {
    NodePath &root = (*di)._root;
    // Apply the matrix to the previous transform.
    root.set_transform(root.get_transform()->compose(TransformState::make_mat(mat)));

    // Now flatten the transform into the subgraph.
    SceneGraphReducer gr;
    gr.apply_attribs(root.node());

    // Transform the frame style too.
    if ((*di)._frame_style.xform(mat)) {
      (*di)._frame_stale = true;
    }
  }
  mark_bound_stale();
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::has_cull_callback
//       Access: Protected, Virtual
//  Description: Should be overridden by derived classes to return
//               true if cull_callback() has been defined.  Otherwise,
//               returns false to indicate cull_callback() does not
//               need to be called for this node during the cull
//               traversal.
////////////////////////////////////////////////////////////////////
bool PGItem::
has_cull_callback() const {
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::cull_callback
//       Access: Protected, Virtual
//  Description: If has_cull_callback() returns true, this function
//               will be called during the cull traversal to perform
//               any additional operations that should be performed at
//               cull time.  This may include additional manipulation
//               of render state or additional visible/invisible
//               decisions, or any other arbitrary operation.
//
//               By the time this function is called, the node has
//               already passed the bounding-volume test for the
//               viewing frustum, and the node's transform and state
//               have already been applied to the indicated
//               CullTraverserData object.
//
//               The return value is true if this node should be
//               visible, or false if it should be culled.
////////////////////////////////////////////////////////////////////
bool PGItem::
cull_callback(CullTraverser *trav, CullTraverserData &data) {
  if (has_frame() && get_active()) {
    // The item has a frame, so we want to generate a region for it
    // and update the MouseWatcher.

    // We can only do this if our traverser is a PGCullTraverser
    // (which will be the case if this node was parented somewhere
    // under a PGTop node).
    if (trav->is_exact_type(PGCullTraverser::get_class_type())) {
      PGCullTraverser *pg_trav;
      DCAST_INTO_R(pg_trav, trav, true);

      const LMatrix4f &transform = data._net_transform->get_mat();

      // Consider the cull bin this object is in.  Since the binning
      // affects the render order, we want bins that render later to
      // get higher sort values.
      int bin_index = data._state->get_bin_index();
      int sort;

      CullBinManager *bin_manager = CullBinManager::get_global_ptr();
      CullBinManager::BinType bin_type = bin_manager->get_bin_type(bin_index);
      if (bin_type == CullBinManager::BT_fixed) {
        // If the bin is a "fixed" type bin, our local sort is based
        // on the fixed order.
        sort = data._state->get_draw_order();

      } else if (bin_type == CullBinManager::BT_unsorted) {
        // If the bin is an "unsorted" type bin, we base the local
        // sort on the scene graph order.
        sort = pg_trav->_sort_index;
        pg_trav->_sort_index++;

      } else {
        // Otherwise, the local sort is irrelevant.
        sort = 0;
      }

      // Now what order does this bin sort relative to the other bins?
      // This becomes the high-order part of the final sort count.
      int bin_sort = bin_manager->get_bin_sort(data._state->get_bin_index());

      // Combine the two sorts into a single int.  This assumes we
      // only need 16 bits for each sort number, possibly an erroneous
      // assumption.  We should really provide two separate sort
      // values, both ints, in the MouseWatcherRegion; but in the
      // interest of expediency we work within the existing interface
      // which only provides one.
      sort = (bin_sort << 16) | ((sort + 0x8000) & 0xffff);

      activate_region(transform, sort);
      pg_trav->_top->add_region(get_region());
    }
  }

  if (has_state_def(get_state())) {
    // This item has a current state definition that we should use
    // to render the item.
    NodePath &root = get_state_def(get_state());
    CullTraverserData next_data(data, root.node());
    trav->traverse(next_data);
  }

  // Now continue to render everything else below this node.
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::recompute_internal_bound
//       Access: Protected, Virtual
//  Description: Called when needed to recompute the node's
//               _internal_bound object.  Nodes that contain anything
//               of substance should redefine this to do the right
//               thing.
////////////////////////////////////////////////////////////////////
BoundingVolume *PGItem::
recompute_internal_bound() {
  // First, get ourselves a fresh, empty bounding volume.
  BoundingVolume *bound = PandaNode::recompute_internal_bound();
  nassertr(bound != (BoundingVolume *)NULL, bound);

  // Now actually compute the bounding volume by putting it around all
  // of our states' bounding volumes.
  pvector<const BoundingVolume *> child_volumes;

  // We walk through the list of state defs indirectly, calling
  // get_state_def() on each one, to ensure that the frames are
  // updated correctly before we measure their bounding volumes.
  for (int i = 0; i < (int)_state_defs.size(); i++) {
    NodePath &root = get_state_def(i);
    if (!root.is_empty()) {
      child_volumes.push_back(&root.node()->get_bound());
    }
  }

  const BoundingVolume **child_begin = &child_volumes[0];
  const BoundingVolume **child_end = child_begin + child_volumes.size();

  bound->around(child_begin, child_end);
  return bound;
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::activate_region
//       Access: Public
//  Description: Applies the indicated scene graph transform and order
//               as determined by the traversal from PGTop.
////////////////////////////////////////////////////////////////////
void PGItem::
activate_region(const LMatrix4f &transform, int sort) {
  // Transform all four vertices, and get the new bounding box.  This
  // way the region works (mostly) even if has been rotated.
  LPoint3f ll(_frame[0], 0.0f, _frame[2]);
  LPoint3f lr(_frame[1], 0.0f, _frame[2]);
  LPoint3f ul(_frame[0], 0.0f, _frame[3]);
  LPoint3f ur(_frame[1], 0.0f, _frame[3]);
  ll = ll * transform;
  lr = lr * transform;
  ul = ul * transform;
  ur = ur * transform;
  _region->set_frame(min(min(ll[0], lr[0]), min(ul[0], ur[0])),
                     max(max(ll[0], lr[0]), max(ul[0], ur[0])),
                     min(min(ll[2], lr[2]), min(ul[2], ur[2])),
                     max(max(ll[2], lr[2]), max(ul[2], ur[2])));
                     
  _region->set_sort(sort);
  _region->set_active(true);
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::enter
//       Access: Public, Virtual
//  Description: This is a callback hook function, called whenever the
//               mouse enters the region.  The mouse is only
//               considered to be "entered" in one region at a time;
//               in the case of nested regions, it exits the outer
//               region before entering the inner one.
////////////////////////////////////////////////////////////////////
void PGItem::
enter(const MouseWatcherParameter &param) {
  PGMouseWatcherParameter *ep = new PGMouseWatcherParameter(param);
  string event = get_enter_event();
  play_sound(event);
  throw_event(event, EventParameter(ep));
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::exit
//       Access: Public, Virtual
//  Description: This is a callback hook function, called whenever the
//               mouse exits the region.  The mouse is only considered
//               to be "entered" in one region at a time; in the case
//               of nested regions, it exits the outer region before
//               entering the inner one.
////////////////////////////////////////////////////////////////////
void PGItem::
exit(const MouseWatcherParameter &param) {
  PGMouseWatcherParameter *ep = new PGMouseWatcherParameter(param);
  string event = get_exit_event();
  play_sound(event);
  throw_event(event, EventParameter(ep));
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::within
//       Access: Public, Virtual
//  Description: This is a callback hook function, called whenever the
//               mouse moves within the boundaries of the region, even
//               if it is also within the boundaries of a nested
//               region.  This is different from "enter", which is
//               only called whenever the mouse is within only that
//               region.
////////////////////////////////////////////////////////////////////
void PGItem::
within(const MouseWatcherParameter &param) {
  PGMouseWatcherParameter *ep = new PGMouseWatcherParameter(param);
  string event = get_within_event();
  play_sound(event);
  throw_event(event, EventParameter(ep));
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::without
//       Access: Public, Virtual
//  Description: This is a callback hook function, called whenever the
//               mouse moves completely outside the boundaries of the
//               region.  See within().
////////////////////////////////////////////////////////////////////
void PGItem::
without(const MouseWatcherParameter &param) {
  PGMouseWatcherParameter *ep = new PGMouseWatcherParameter(param);
  string event = get_without_event();
  play_sound(event);
  throw_event(event, EventParameter(ep));
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::focus_in
//       Access: Public, Virtual
//  Description: This is a callback hook function, called whenever the
//               widget gets the keyboard focus.
////////////////////////////////////////////////////////////////////
void PGItem::
focus_in() {
  string event = get_focus_in_event();
  play_sound(event);
  throw_event(event);
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::focus_out
//       Access: Public, Virtual
//  Description: This is a callback hook function, called whenever the
//               widget loses the keyboard focus.
////////////////////////////////////////////////////////////////////
void PGItem::
focus_out() {
  string event = get_focus_out_event();
  play_sound(event);
  throw_event(event);
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::press
//       Access: Public, Virtual
//  Description: This is a callback hook function, called whenever a
//               mouse or keyboard button is depressed while the mouse
//               is within the region.
////////////////////////////////////////////////////////////////////
void PGItem::
press(const MouseWatcherParameter &param, bool background) {
  if (!background) {
    PGMouseWatcherParameter *ep = new PGMouseWatcherParameter(param);
    string event = get_press_event(param.get_button());
    play_sound(event);
    throw_event(event, EventParameter(ep));
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::release
//       Access: Public, Virtual
//  Description: This is a callback hook function, called whenever a
//               mouse or keyboard button previously depressed with
//               press() is released.
////////////////////////////////////////////////////////////////////
void PGItem::
release(const MouseWatcherParameter &param, bool background) {
  if (!background) {
    PGMouseWatcherParameter *ep = new PGMouseWatcherParameter(param);
    string event = get_release_event(param.get_button());
    play_sound(event);
    throw_event(event, EventParameter(ep));
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::keystroke
//       Access: Public, Virtual
//  Description: This is a callback hook function, called whenever
//               the user presses a key.
////////////////////////////////////////////////////////////////////
void PGItem::
keystroke(const MouseWatcherParameter &param, bool background) {
  if (!background) {
    PGMouseWatcherParameter *ep = new PGMouseWatcherParameter(param);
    string event = get_keystroke_event();
    play_sound(event);
    throw_event(event, EventParameter(ep));
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::keystroke
//       Access: Public, Virtual
//  Description: This is a callback hook function, called whenever
//               the user highlights an option in the IME window.
////////////////////////////////////////////////////////////////////
void PGItem::
candidate(const MouseWatcherParameter &param, bool background) {
  // We don't throw sound events for candidate selections for now.
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::background_press
//       Access: Public, Static
//  Description: Calls press() on all the PGItems with background
//               focus.
////////////////////////////////////////////////////////////////////
void PGItem::
background_press(const MouseWatcherParameter &param) {
  BackgroundFocus::const_iterator fi;
  for (fi = _background_focus.begin(); fi != _background_focus.end(); ++fi) {
    PGItem *item = *fi;
    if (!item->get_focus()) {
      item->press(param, true);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::background_release
//       Access: Public, Static
//  Description: Calls release() on all the PGItems with background
//               focus.
////////////////////////////////////////////////////////////////////
void PGItem::
background_release(const MouseWatcherParameter &param) {
  BackgroundFocus::const_iterator fi;
  for (fi = _background_focus.begin(); fi != _background_focus.end(); ++fi) {
    PGItem *item = *fi;
    if (!item->get_focus()) {
      item->release(param, true);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::background_keystroke
//       Access: Public, Static
//  Description: Calls keystroke() on all the PGItems with background
//               focus.
////////////////////////////////////////////////////////////////////
void PGItem::
background_keystroke(const MouseWatcherParameter &param) {
  BackgroundFocus::const_iterator fi;
  for (fi = _background_focus.begin(); fi != _background_focus.end(); ++fi) {
    PGItem *item = *fi;
    if (!item->get_focus()) {
      item->keystroke(param, true);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::background_candidate
//       Access: Public, Static
//  Description: Calls candidate() on all the PGItems with background
//               focus.
////////////////////////////////////////////////////////////////////
void PGItem::
background_candidate(const MouseWatcherParameter &param) {
  BackgroundFocus::const_iterator fi;
  for (fi = _background_focus.begin(); fi != _background_focus.end(); ++fi) {
    PGItem *item = *fi;
    if (!item->get_focus()) {
      item->candidate(param, true);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::set_active
//       Access: Published, Virtual
//  Description: Sets whether the PGItem is active for mouse watching.
//               This is not necessarily related to the
//               active/inactive appearance of the item, which is
//               controlled by set_state(), but it does affect whether
//               it responds to mouse events.
////////////////////////////////////////////////////////////////////
void PGItem::
set_active(bool active) {
  if (active) {
    _flags |= F_active;
  } else {
    _flags &= ~F_active;
    // Deactivating the item automatically defocuses it too.
    if (get_focus()) {
      set_focus(false);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::set_focus
//       Access: Published, Virtual
//  Description: Sets whether the PGItem currently has keyboard focus.
//               This simply means that the item may respond to
//               keyboard events as well as to mouse events; precisely
//               what this means is up to the individual item.  
//
//               Only one PGItem in the world is allowed to have focus
//               at any given time.  Setting the focus on any other
//               item automatically disables the focus from the
//               previous item.
////////////////////////////////////////////////////////////////////
void PGItem::
set_focus(bool focus) {
  if (focus) {
    if (!get_active()) {
      // Cannot set focus on an inactive item.
      return;
    }

    // Set the keyboard focus to this item.
    if (_focus_item != this) {
      if (_focus_item != (PGItem *)NULL) {
        // Clear the focus from whatever item currently has it.
        _focus_item->set_focus(false);
      }
      _focus_item = this;
    }
    if (!get_focus()) {
      focus_in();
      _flags |= F_focus;
    }

  } else {
    if (_focus_item == this) {
      // Remove this item from the focus.
      _focus_item = (PGItem *)NULL;
    }

    if (get_focus()) {
      focus_out();
      _flags &= ~F_focus;
    }
  }
  _region->set_keyboard(focus);
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::set_background_focus
//       Access: Published
//  Description: Sets the background_focus flag for this item.  When
//               background_focus is enabled, the item will receive
//               keypress events even if it is not in focus; in fact,
//               even if it is not onscreen.  Unlike normal focus,
//               many items may have background_focus simultaneously.
////////////////////////////////////////////////////////////////////
void PGItem::
set_background_focus(bool focus) {
  if (focus != get_background_focus()) {
    if (focus) {
      // Activate background focus.
      _flags |= F_background_focus;
      bool inserted = _background_focus.insert(this).second;
      nassertv(inserted);

    } else {
      // Deactivate background focus.
      _flags &= ~F_background_focus;
      size_t num_erased = _background_focus.erase(this);
      nassertv(num_erased == 1);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::get_num_state_defs
//       Access: Published
//  Description: Returns one more than the highest-numbered state def
//               that was ever assigned to the PGItem.  The complete
//               set of state defs assigned may then be retrieved by
//               indexing from 0 to (get_num_state_defs() - 1).
//
//               This is only an upper limit on the actual number of
//               state defs, since there may be holes in the list.
////////////////////////////////////////////////////////////////////
int PGItem::
get_num_state_defs() const {
  return _state_defs.size();
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::has_state_def
//       Access: Published
//  Description: Returns true if get_state_def() has ever been called
//               for the indicated state (thus defining a render
//               subgraph for this state index), false otherwise.
////////////////////////////////////////////////////////////////////
bool PGItem::
has_state_def(int state) const {
  if (state < 0 || state >= (int)_state_defs.size()) {
    return false;
  }
  return (!_state_defs[state]._root.is_empty());
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::clear_state_def
//       Access: Published
//  Description: Resets the NodePath assigned to the indicated state
//               to its initial default, with only a frame
//               representation if appropriate.
////////////////////////////////////////////////////////////////////
void PGItem::
clear_state_def(int state) {
  if (state < 0 || state >= (int)_state_defs.size()) {
    return;
  }

  _state_defs[state]._root = NodePath();
  _state_defs[state]._frame = NodePath();
  _state_defs[state]._frame_stale = true;

  mark_bound_stale();
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::get_state_def
//       Access: Published
//  Description: Returns the Node that is the root of the subgraph
//               that will be drawn when the PGItem is in the
//               indicated state.  The first time this is called for a
//               particular state index, it may create the Node.
////////////////////////////////////////////////////////////////////
NodePath &PGItem::
get_state_def(int state) {
  nassertr(state >= 0 && state < 1000, get_state_def(0));  // Sanity check.
  slot_state_def(state);

  if (_state_defs[state]._root.is_empty()) {
    // Create a new node.
    _state_defs[state]._root = NodePath("state_" + format_string(state));
    _state_defs[state]._frame_stale = true;
  }

  if (_state_defs[state]._frame_stale) {
    update_frame(state);
  }

  return _state_defs[state]._root;
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::instance_to_state_def
//       Access: Published
//  Description: Parents an instance of the bottom node of the
//               indicated NodePath to the indicated state index.
////////////////////////////////////////////////////////////////////
NodePath PGItem::
instance_to_state_def(int state, const NodePath &path) {
  if (path.is_empty()) {
    // If the source is empty, quietly do nothing.
    return NodePath();
  }

  mark_bound_stale();

  return path.instance_to(get_state_def(state));
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::get_frame_style
//       Access: Published
//  Description: Returns the kind of frame that will be drawn behind
//               the item when it is in the indicated state.
////////////////////////////////////////////////////////////////////
PGFrameStyle PGItem::
get_frame_style(int state) {
  if (state < 0 || state >= (int)_state_defs.size()) {
    return PGFrameStyle();
  }
  return _state_defs[state]._frame_style;
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::set_frame_style
//       Access: Published
//  Description: Changes the kind of frame that will be drawn behind
//               the item when it is in the indicated state.
////////////////////////////////////////////////////////////////////
void PGItem::
set_frame_style(int state, const PGFrameStyle &style) {
  // Get the state def node, mainly to ensure that this state is
  // slotted and listed as having been defined.
  NodePath &root = get_state_def(state);
  nassertv(!root.is_empty());

  _state_defs[state]._frame_style = style;
  _state_defs[state]._frame_stale = true;

  mark_bound_stale();
}

#ifdef HAVE_AUDIO
////////////////////////////////////////////////////////////////////
//     Function: PGItem::set_sound
//       Access: Published
//  Description: Sets the sound that will be played whenever the
//               indicated event occurs.
////////////////////////////////////////////////////////////////////
void PGItem::
set_sound(const string &event, AudioSound *sound) {
  _sounds[event] = sound;
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::clear_sound
//       Access: Published
//  Description: Removes the sound associated with the indicated
//               event.
////////////////////////////////////////////////////////////////////
void PGItem::
clear_sound(const string &event) {
  _sounds.erase(event);
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::get_sound
//       Access: Published
//  Description: Returns the sound associated with the indicated
//               event, or NULL if there is no associated sound.
////////////////////////////////////////////////////////////////////
AudioSound *PGItem::
get_sound(const string &event) const {
  Sounds::const_iterator si = _sounds.find(event);
  if (si != _sounds.end()) {
    return (*si).second;
  }
  return (AudioSound *)NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::has_sound
//       Access: Published
//  Description: Returns true if there is a sound associated with the
//               indicated event, or false otherwise.
////////////////////////////////////////////////////////////////////
bool PGItem::
has_sound(const string &event) const {
  return (_sounds.count(event) != 0);
}
#endif  // HAVE_AUDIO

////////////////////////////////////////////////////////////////////
//     Function: PGItem::get_text_node
//       Access: Published, Static
//  Description: Returns the TextNode object that will be used by all
//               PGItems to generate default labels given a string.
//               This can be loaded with the default font, etc.
////////////////////////////////////////////////////////////////////
TextNode *PGItem::
get_text_node() {
  if (_text_node == (TextNode *)NULL) {
    _text_node = new TextNode("pguiText");
    _text_node->set_text_color(0.0f, 0.0f, 0.0f, 1.0f);

    // The default TextNode is aligned to the left, for the
    // convenience of PGEntry.
    _text_node->set_align(TextNode::A_left);
  }
  return _text_node;
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::play_sound
//       Access: Protected
//  Description: Plays the sound associated with the indicated event,
//               if there is one.
////////////////////////////////////////////////////////////////////
void PGItem::
play_sound(const string &event) {
#ifdef HAVE_AUDIO
  Sounds::const_iterator si = _sounds.find(event);
  if (si != _sounds.end()) {
    AudioSound *sound = (*si).second;
    sound->play();
  }
#endif  // HAVE_AUDIO
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::slot_state_def
//       Access: Private
//  Description: Ensures there is a slot in the array for the given
//               state definition.
////////////////////////////////////////////////////////////////////
void PGItem::
slot_state_def(int state) {
  while (state >= (int)_state_defs.size()) {
    StateDef def;
    def._frame_stale = true;
    _state_defs.push_back(def);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::update_frame
//       Access: Private
//  Description: Generates a new instance of the frame geometry for
//               the indicated state.
////////////////////////////////////////////////////////////////////
void PGItem::
update_frame(int state) {
  // First, remove the old frame geometry, if any.
  if (state >= 0 && state < (int)_state_defs.size()) {
    _state_defs[state]._frame.remove_node();
  }

  // We must turn off the stale flag first, before we call
  // get_state_def(), to prevent get_state_def() from being a
  // recursive call.
  _state_defs[state]._frame_stale = false;

  // Now create new frame geometry.
  if (has_frame()) {
    NodePath &root = get_state_def(state);
    _state_defs[state]._frame = 
      _state_defs[state]._frame_style.generate_into(root, _frame);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PGItem::mark_frames_stale
//       Access: Private
//  Description: Marks all the frames in all states stale, so that
//               they will be regenerated the next time each state is
//               requested.
////////////////////////////////////////////////////////////////////
void PGItem::
mark_frames_stale() {
  StateDefs::iterator di;
  for (di = _state_defs.begin(); di != _state_defs.end(); ++di) {
    // Remove the old frame, if any.
    (*di)._frame.remove_node();
    (*di)._frame_stale = true;
  }
  mark_bound_stale();
}
