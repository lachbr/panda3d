// Filename: renderState.cxx
// Created by:  drose (21Feb02)
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

#include "renderState.h"
#include "transparencyAttrib.h"
#include "cullBinAttrib.h"
#include "cullBinManager.h"
#include "fogAttrib.h"
#include "transparencyAttrib.h"
#include "config_pgraph.h"
#include "bamReader.h"
#include "bamWriter.h"
#include "datagramIterator.h"
#include "indent.h"
#include "compareTo.h"

RenderState::States *RenderState::_states = NULL;
CPT(RenderState) RenderState::_empty_state;
TypeHandle RenderState::_type_handle;


////////////////////////////////////////////////////////////////////
//     Function: RenderState::Constructor
//       Access: Protected
//  Description: Actually, this could be a private constructor, since
//               no one inherits from RenderState, but gcc gives us a
//               spurious warning if all constructors are private.
////////////////////////////////////////////////////////////////////
RenderState::
RenderState() {
  if (_states == (States *)NULL) {
    // Make sure the global _states map is allocated.  This only has
    // to be done once.  We could make this map static, but then we
    // run into problems if anyone creates a RenderState object at
    // static init time; it also seems to cause problems when the
    // Panda shared library is unloaded at application exit time.
    _states = new States;
  }
  _saved_entry = _states->end();
  _self_compose = (RenderState *)NULL;
  _flags = 0;
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::Copy Constructor
//       Access: Private
//  Description: RenderStates are not meant to be copied.
////////////////////////////////////////////////////////////////////
RenderState::
RenderState(const RenderState &) {
  nassertv(false);
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::Copy Assignment Operator
//       Access: Private
//  Description: RenderStates are not meant to be copied.
////////////////////////////////////////////////////////////////////
void RenderState::
operator = (const RenderState &) {
  nassertv(false);
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::Destructor
//       Access: Public, Virtual
//  Description: The destructor is responsible for removing the
//               RenderState from the global set if it is there.
////////////////////////////////////////////////////////////////////
RenderState::
~RenderState() {
  // We'd better not call the destructor twice on a particular object.
  nassertv(!is_destructing());
  set_destructing();

  if (_saved_entry != _states->end()) {
    nassertv(_states->find(this) == _saved_entry);
    _states->erase(_saved_entry);
    _saved_entry = _states->end();
  }

  // Now make sure we clean up all other floating pointers to the
  // RenderState.  These may be scattered around in the various
  // CompositionCaches from other RenderState objects.

  // Fortunately, since we added CompositionCache records in pairs, we
  // know exactly the set of RenderState objects that have us in their
  // cache: it's the same set of RenderState objects that we have in
  // our own cache.

  // We do need to put considerable thought into this loop, because as
  // we clear out cache entries we'll cause other RenderState
  // objects to destruct, which could cause things to get pulled out
  // of our own _composition_cache map.  We want to allow this (so
  // that we don't encounter any just-destructed pointers in our
  // cache), but we don't want to get bitten by this cascading effect.
  // Instead of walking through the map from beginning to end,
  // therefore, we just pull out the first one each time, and erase
  // it.

  // There are lots of ways to do this loop wrong.  Be very careful if
  // you need to modify it for any reason.
  while (!_composition_cache.empty()) {
    CompositionCache::iterator ci = _composition_cache.begin();

    // It is possible that the "other" RenderState object is
    // currently within its own destructor.  We therefore can't use a
    // PT() to hold its pointer; that could end up calling its
    // destructor twice.  Fortunately, we don't need to hold its
    // reference count to ensure it doesn't destruct while we process
    // this loop; as long as we ensure that no *other* RenderState
    // objects destruct, there will be no reason for that one to.
    RenderState *other = (RenderState *)(*ci).first;

    // We should never have a reflexive entry in this map.  If we
    // do, something got screwed up elsewhere.
    nassertv(other != this);

    // We hold a copy of the composition result to ensure that the
    // result RenderState object (if there is one) doesn't
    // destruct.
    Composition comp = (*ci).second;

    // Now we can remove the element from our cache.  We do this now,
    // rather than later, before any other RenderState objects have
    // had a chance to destruct, so we are confident that our iterator
    // is still valid.
    _composition_cache.erase(ci);

    CompositionCache::iterator oci = other->_composition_cache.find(this);

    // We may or may not still be listed in the other's cache (it
    // might be halfway through pulling entries out, from within its
    // own destructor).
    if (oci != other->_composition_cache.end()) {
      // Hold a copy of the other composition result, too.
      Composition ocomp = (*oci).second;
      
      // Now we're holding a reference count to both computed
      // results, so no objects will be tempted to destruct while we
      // erase the other cache entry.
      other->_composition_cache.erase(oci);
    }

    // It's finally safe to let our held pointers go away.  This may
    // have cascading effects as other RenderState objects are
    // destructed, but there will be no harm done if they destruct
    // now.
  }

  // A similar bit of code for the invert cache.
  while (!_invert_composition_cache.empty()) {
    CompositionCache::iterator ci = _invert_composition_cache.begin();
    RenderState *other = (RenderState *)(*ci).first;
    nassertv(other != this);
    Composition comp = (*ci).second;
    _invert_composition_cache.erase(ci);
    CompositionCache::iterator oci = 
      other->_invert_composition_cache.find(this);
    if (oci != other->_invert_composition_cache.end()) {
      Composition ocomp = (*oci).second;
      other->_invert_composition_cache.erase(oci);
    }
  }

  // Also, if we called compose(this) at some point and the return
  // value was something other than this, we need to decrement the
  // associated reference count.
  if (_self_compose != (RenderState *)NULL && _self_compose != this) {
    unref_delete((RenderState *)_self_compose);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::operator <
//       Access: Public
//  Description: Provides an arbitrary ordering among all unique
//               RenderStates, so we can store the essentially
//               different ones in a big set and throw away the rest.
//
//               This method is not needed outside of the RenderState
//               class because all equivalent RenderState objects are
//               guaranteed to share the same pointer; thus, a pointer
//               comparison is always sufficient.
////////////////////////////////////////////////////////////////////
bool RenderState::
operator < (const RenderState &other) const {
  // We must compare all the properties of the attributes, not just
  // the type; thus, we compare them one at a time using compare_to().
  return lexicographical_compare(_attributes.begin(), _attributes.end(),
                                 other._attributes.begin(), other._attributes.end(),
                                 CompareTo<Attribute>());
}


////////////////////////////////////////////////////////////////////
//     Function: RenderState::find_attrib
//       Access: Published
//  Description: Searches for an attribute with the indicated type in
//               the state, and returns its index if it is found, or
//               -1 if it is not.
////////////////////////////////////////////////////////////////////
int RenderState::
find_attrib(TypeHandle type) const {
  Attributes::const_iterator ai = _attributes.find(Attribute(type));
  if (ai == _attributes.end()) {
    return -1;
  }
  return ai - _attributes.begin();
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::make_empty
//       Access: Published, Static
//  Description: Returns a RenderState with no attributes set.
////////////////////////////////////////////////////////////////////
CPT(RenderState) RenderState::
make_empty() {
  // The empty state is asked for so often, we make it a special case
  // and store a pointer forever once we find it the first time.
  if (_empty_state == (RenderState *)NULL) {
    RenderState *state = new RenderState;
    _empty_state = return_new(state);
  }

  return _empty_state;
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::make
//       Access: Published, Static
//  Description: Returns a RenderState with one attribute set.
////////////////////////////////////////////////////////////////////
CPT(RenderState) RenderState::
make(const RenderAttrib *attrib, int override) {
  RenderState *state = new RenderState;
  state->_attributes.reserve(1);
  state->_attributes.insert(Attribute(attrib, override));
  return return_new(state);
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::make
//       Access: Published, Static
//  Description: Returns a RenderState with two attributes set.
////////////////////////////////////////////////////////////////////
CPT(RenderState) RenderState::
make(const RenderAttrib *attrib1,
     const RenderAttrib *attrib2, int override) {
  RenderState *state = new RenderState;
  state->_attributes.reserve(2);
  state->_attributes.push_back(Attribute(attrib1, override));
  state->_attributes.push_back(Attribute(attrib2, override));
  state->_attributes.sort();
  return return_new(state);
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::make
//       Access: Published, Static
//  Description: Returns a RenderState with three attributes set.
////////////////////////////////////////////////////////////////////
CPT(RenderState) RenderState::
make(const RenderAttrib *attrib1,
     const RenderAttrib *attrib2,
     const RenderAttrib *attrib3, int override) {
  RenderState *state = new RenderState;
  state->_attributes.reserve(2);
  state->_attributes.push_back(Attribute(attrib1, override));
  state->_attributes.push_back(Attribute(attrib2, override));
  state->_attributes.push_back(Attribute(attrib3, override));
  state->_attributes.sort();
  return return_new(state);
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::make
//       Access: Published, Static
//  Description: Returns a RenderState with four attributes set.
////////////////////////////////////////////////////////////////////
CPT(RenderState) RenderState::
make(const RenderAttrib *attrib1,
     const RenderAttrib *attrib2,
     const RenderAttrib *attrib3,
     const RenderAttrib *attrib4, int override) {
  RenderState *state = new RenderState;
  state->_attributes.reserve(2);
  state->_attributes.push_back(Attribute(attrib1, override));
  state->_attributes.push_back(Attribute(attrib2, override));
  state->_attributes.push_back(Attribute(attrib3, override));
  state->_attributes.push_back(Attribute(attrib4, override));
  state->_attributes.sort();
  return return_new(state);
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::compose
//       Access: Published
//  Description: Returns a new RenderState object that represents the
//               composition of this state with the other state.
//
//               The result of this operation is cached, and will be
//               retained as long as both this RenderState object and
//               the other RenderState object continue to exist.
//               Should one of them destruct, the cached entry will be
//               removed, and its pointer will be allowed to destruct
//               as well.
////////////////////////////////////////////////////////////////////
CPT(RenderState) RenderState::
compose(const RenderState *other) const {
  // This method isn't strictly const, because it updates the cache,
  // but we pretend that it is because it's only a cache which is
  // transparent to the rest of the interface.

  // We handle empty state (identity) as a trivial special case.
  if (is_empty()) {
    return other;
  }
  if (other->is_empty()) {
    return this;
  }

  if (other == this) {
    // compose(this) has to be handled as a special case, because the
    // caching problem is so different.
    if (_self_compose != (RenderState *)NULL) {
      return _self_compose;
    }
    CPT(RenderState) result = do_compose(this);
    ((RenderState *)this)->_self_compose = result;

    if (result != (const RenderState *)this) {
      // If the result of compose(this) is something other than this,
      // explicitly increment the reference count.  We have to be sure
      // to decrement it again later, in our destructor.
      _self_compose->ref();

      // (If the result was just this again, we still store the
      // result, but we don't increment the reference count, since
      // that would be a self-referential leak.  What a mess this is.)
    }
    return _self_compose;
  }

  // Is this composition already cached?
  CompositionCache::const_iterator ci = _composition_cache.find(other);
  if (ci != _composition_cache.end()) {
    const Composition &comp = (*ci).second;
    if (comp._result == (const RenderState *)NULL) {
      // Well, it wasn't cached already, but we already had an entry
      // (probably created for the reverse direction), so use the same
      // entry to store the new result.
      ((Composition &)comp)._result = do_compose(other);
    }
    // Here's the cache!
    return comp._result;
  }

  // We need to make a new cache entry, both in this object and in the
  // other object.  We make both records so the other RenderState
  // object will know to delete the entry from this object when it
  // destructs, and vice-versa.

  // The cache entry in this object is the only one that indicates the
  // result; the other will be NULL for now.
  CPT(RenderState) result = do_compose(other);
  // We store them in this order, on the off-chance that other is the
  // same as this, a degenerate case which is still worth supporting.
  ((RenderState *)other)->_composition_cache[this]._result = NULL;
  ((RenderState *)this)->_composition_cache[other]._result = result;

  return result;
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::invert_compose
//       Access: Published
//  Description: Returns a new RenderState object that represents the
//               composition of this state's inverse with the other
//               state.
//
//               This is similar to compose(), but is particularly
//               useful for computing the relative state of a node as
//               viewed from some other node.
////////////////////////////////////////////////////////////////////
CPT(RenderState) RenderState::
invert_compose(const RenderState *other) const {
  // This method isn't strictly const, because it updates the cache,
  // but we pretend that it is because it's only a cache which is
  // transparent to the rest of the interface.

  // We handle empty state (identity) as a trivial special case.
  if (is_empty()) {
    return other;
  }
  // Unlike compose(), the case of other->is_empty() is not quite as
  // trivial for invert_compose().

  if (other == this) {
    // a->invert_compose(a) always produces identity.
    return make_empty();
  }

  // Is this composition already cached?
  CompositionCache::const_iterator ci = _invert_composition_cache.find(other);
  if (ci != _invert_composition_cache.end()) {
    const Composition &comp = (*ci).second;
    if (comp._result == (const RenderState *)NULL) {
      // Well, it wasn't cached already, but we already had an entry
      // (probably created for the reverse direction), so use the same
      // entry to store the new result.
      ((Composition &)comp)._result = do_invert_compose(other);
    }
    // Here's the cache!
    return comp._result;
  }

  // We need to make a new cache entry, both in this object and in the
  // other object.  We make both records so the other RenderState
  // object will know to delete the entry from this object when it
  // destructs, and vice-versa.

  // The cache entry in this object is the only one that indicates the
  // result; the other will be NULL for now.
  CPT(RenderState) result = do_invert_compose(other);
  // We store them in this order, on the off-chance that other is the
  // same as this, a degenerate case which is still worth supporting.
  ((RenderState *)other)->_invert_composition_cache[this]._result = NULL;
  ((RenderState *)this)->_invert_composition_cache[other]._result = result;

  return result;
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::add_attrib
//       Access: Published
//  Description: Returns a new RenderState object that represents the
//               same as the source state, with the new RenderAttrib
//               added.  If there is already a RenderAttrib with the
//               same type, it is replaced.
////////////////////////////////////////////////////////////////////
CPT(RenderState) RenderState::
add_attrib(const RenderAttrib *attrib, int override) const {
  RenderState *new_state = new RenderState;
  back_insert_iterator<Attributes> result = 
    back_inserter(new_state->_attributes);

  Attribute new_attribute(attrib, override);
  Attributes::const_iterator ai = _attributes.begin();

  while (ai != _attributes.end() && (*ai) < new_attribute) {
    *result = *ai;
    ++ai;
    ++result;
  }
  *result = new_attribute;
  ++result;

  if (ai != _attributes.end() && !(new_attribute < (*ai))) {
    // At this point we know:
    // !((*ai) < new_attribute) && !(new_attribute < (*ai))
    // which means (*ai) == new_attribute--so we should leave it out,
    // to avoid duplicating attributes in the set.
    ++ai;
  }

  while (ai != _attributes.end()) {
    *result = *ai;
    ++ai;
    ++result;
  }

  return return_new(new_state);
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::remove_attrib
//       Access: Published
//  Description: Returns a new RenderState object that represents the
//               same as the source state, with the indicated
//               RenderAttrib removed.
////////////////////////////////////////////////////////////////////
CPT(RenderState) RenderState::
remove_attrib(TypeHandle type) const {
  RenderState *new_state = new RenderState;
  back_insert_iterator<Attributes> result = 
    back_inserter(new_state->_attributes);

  Attributes::const_iterator ai = _attributes.begin();

  while (ai != _attributes.end()) {
    if ((*ai)._type != type) {
      *result = *ai;
      ++result;
    }
    ++ai;
  }

  return return_new(new_state);
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::remove_attrib
//       Access: Published
//  Description: Returns a new RenderState object that represents the
//               same as the source state, with all attributes'
//               override values incremented (or decremented, if
//               negative) by the indicated amount.  If the override
//               would drop below zero, it is set to zero.
////////////////////////////////////////////////////////////////////
CPT(RenderState) RenderState::
adjust_all_priorities(int adjustment) const {
  RenderState *new_state = new RenderState;
  new_state->_attributes.reserve(_attributes.size());

  Attributes::const_iterator ai;
  for (ai = _attributes.begin(); ai != _attributes.end(); ++ai) {
    Attribute attrib = *ai;
    attrib._override = max(attrib._override + adjustment, 0);
    new_state->_attributes.push_back(attrib);
  }

  return return_new(new_state);
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::get_attrib
//       Access: Published, Virtual
//  Description: Looks for a RenderAttrib of the indicated type in the
//               state, and returns it if it is found, or NULL if it
//               is not.
////////////////////////////////////////////////////////////////////
const RenderAttrib *RenderState::
get_attrib(TypeHandle type) const {
  Attributes::const_iterator ai;
  ai = _attributes.find(Attribute(type));
  if (ai != _attributes.end()) {
    return (*ai)._attrib;
  }
  return NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::output
//       Access: Published, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void RenderState::
output(ostream &out) const {
  out << "S:";
  if (_attributes.empty()) {
    out << "(empty)";

  } else {
    Attributes::const_iterator ai = _attributes.begin();
    out << "(" << (*ai)._type;
    ++ai;
    while (ai != _attributes.end()) {
      out << " " << (*ai)._type;
      ++ai;
    }
    out << ")";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::write
//       Access: Published, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void RenderState::
write(ostream &out, int indent_level) const {
  indent(out, indent_level) << _attributes.size() << " attribs:\n";
  Attributes::const_iterator ai;
  for (ai = _attributes.begin(); ai != _attributes.end(); ++ai) {
    const Attribute &attribute = (*ai);
    attribute._attrib->write(out, indent_level + 2);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::issue_delta_modify
//       Access: Public
//  Description: This is intended to be called only from
//               GraphicsStateGuardian::modify_state().  It calls
//               issue() for each attribute given in the other state
//               that differs from the current state (which is assumed
//               to represent the GSG's current state).  Returns the
//               RenderState representing the newly composed result.
////////////////////////////////////////////////////////////////////
CPT(RenderState) RenderState::
issue_delta_modify(const RenderState *other, 
                   GraphicsStateGuardianBase *gsg) const {
  if (other->is_empty()) {
    // If the other state is empty, that's a trivial special case.
    return this;
  }

  // First, build a new Attributes member that represents the union of
  // this one and that one.
  Attributes::const_iterator ai = _attributes.begin();
  Attributes::const_iterator bi = other->_attributes.begin();

  // Create a new RenderState that will hold the result.
  RenderState *new_state = new RenderState;
  back_insert_iterator<Attributes> result = 
    back_inserter(new_state->_attributes);

  bool any_changed = false;

  while (ai != _attributes.end() && bi != other->_attributes.end()) {
    if ((*ai) < (*bi)) {
      // Here is an attribute that we have in the original, which is
      // not present in the secondary.  Leave it alone.
      *result = *ai;
      ++ai;
      ++result;
    } else if ((*bi) < (*ai)) {
      // Here is a new attribute we have in the secondary, that was
      // not present in the original.  Issue the new one, and save it.
      (*bi)._attrib->issue(gsg);
      *result = *bi;
      ++bi;
      ++result;
      any_changed = true;
    } else {
      // Here is an attribute we have in both.  Issue the new one if
      // it's different, and save it.
      if ((*ai)._attrib != (*bi)._attrib) {
        any_changed = true;
        (*bi)._attrib->issue(gsg);
      }
      *result = *bi;
      ++ai;
      ++bi;
      ++result;
    }
  }

  while (ai != _attributes.end()) {
    *result = *ai;
    ++ai;
    ++result;
  }

  while (bi != other->_attributes.end()) {
    (*bi)._attrib->issue(gsg);
    *result = *bi;
    ++bi;
    ++result;
    any_changed = true;
  }

  if (any_changed) {
    return return_new(new_state);
  } else {
    delete new_state;
    return other;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::issue_delta_set
//       Access: Public
//  Description: This is intended to be called only from
//               GraphicsStateGuardian::set_state().  It calls issue()
//               for each attribute given in the other state that
//               differs from the current state (which is assumed to
//               represent the GSG's current state).  Returns the
//               RenderState representing the newly composed result
//               (which will be the same as other).
////////////////////////////////////////////////////////////////////
CPT(RenderState) RenderState::
issue_delta_set(const RenderState *other, 
                GraphicsStateGuardianBase *gsg) const {
  if (other == this) {
    // If the state doesn't change, that's a trivial special case.
    return other;
  }

  // We don't need to build a new RenderState, because the return
  // value will be exactly other.

  Attributes::const_iterator ai = _attributes.begin();
  Attributes::const_iterator bi = other->_attributes.begin();

  while (ai != _attributes.end() && bi != other->_attributes.end()) {
    if ((*ai) < (*bi)) {
      // Here is an attribute that we have in the original, which is
      // not present in the secondary.  Issue the default state instead.
      (*ai)._attrib->make_default()->issue(gsg);
      ++ai;

    } else if ((*bi) < (*ai)) {
      // Here is a new attribute we have in the secondary, that was
      // not present in the original.  Issue the new one.
      (*bi)._attrib->issue(gsg);
      ++bi;

    } else {
      // Here is an attribute we have in both.  Issue the new one if
      // it's different.
      if ((*ai)._attrib != (*bi)._attrib) {
        (*bi)._attrib->issue(gsg);
      }
      ++ai;
      ++bi;
    }
  }

  while (ai != _attributes.end()) {
    (*ai)._attrib->make_default()->issue(gsg);
    ++ai;
  }

  while (bi != other->_attributes.end()) {
    (*bi)._attrib->issue(gsg);
    ++bi;
  }

  return other;
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::bin_removed
//       Access: Public, Static
//  Description: Intended to be called by
//               CullBinManager::remove_bin(), this informs all the
//               RenderStates in the world to remove the indicated
//               bin_index from their cache if it has been cached.
////////////////////////////////////////////////////////////////////
void RenderState::
bin_removed(int bin_index) {
  // Do something here.
  nassertv(false);
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::return_new
//       Access: Private, Static
//  Description: This function is used to share a common RenderState
//               pointer for all equivalent RenderState objects.
//
//               See the similar logic in RenderAttrib.  The idea is
//               to create a new RenderState object and pass it
//               through this function, which will share the pointer
//               with a previously-created RenderState object if it is
//               equivalent.
////////////////////////////////////////////////////////////////////
CPT(RenderState) RenderState::
return_new(RenderState *state) {
  nassertr(state != (RenderState *)NULL, state);

  // This should be a newly allocated pointer, not one that was used
  // for anything else.
  nassertr(state->_saved_entry == _states->end(), state);

  // Save the state in a local PointerTo so that it will be freed at
  // the end of this function if no one else uses it.
  CPT(RenderState) pt_state = state;

  pair<States::iterator, bool> result = _states->insert(state);

  if (result.second) {
    // The state was inserted; save the iterator and return the
    // input state.
    state->_saved_entry = result.first;
    return pt_state;
  }
  
  // The state was not inserted; there must be an equivalent one
  // already in the set.  Return that one.
  return *(result.first);
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::do_compose
//       Access: Private
//  Description: The private implemention of compose(); this actually
//               composes two RenderStates, without bothering with the
//               cache.
////////////////////////////////////////////////////////////////////
CPT(RenderState) RenderState::
do_compose(const RenderState *other) const {
  // First, build a new Attributes member that represents the union of
  // this one and that one.
  Attributes::const_iterator ai = _attributes.begin();
  Attributes::const_iterator bi = other->_attributes.begin();

  // Create a new RenderState that will hold the result.
  RenderState *new_state = new RenderState;
  back_insert_iterator<Attributes> result = 
    back_inserter(new_state->_attributes);

  while (ai != _attributes.end() && bi != other->_attributes.end()) {
    if ((*ai) < (*bi)) {
      // Here is an attribute that we have in the original, which is
      // not present in the secondary.
      *result = *ai;
      ++ai;
      ++result;
    } else if ((*bi) < (*ai)) {
      // Here is a new attribute we have in the secondary, that was
      // not present in the original.
      *result = *bi;
      ++bi;
      ++result;
    } else {
      // Here is an attribute we have in both.  Does one override the
      // other?
      const Attribute &a = (*ai);
      const Attribute &b = (*bi);
      if (a._override < b._override) {
        // B overrides.
        *result = *bi;

      } else if (b._override < a._override) {
        // A overrides.
        *result = *ai;

      } else {
        // No, they're equivalent, so compose them.
        *result = Attribute(a._attrib->compose(b._attrib), b._override);
      }
      ++ai;
      ++bi;
      ++result;
    }
  }

  while (ai != _attributes.end()) {
    *result = *ai;
    ++ai;
    ++result;
  }

  while (bi != other->_attributes.end()) {
    *result = *bi;
    ++bi;
    ++result;
  }

  return return_new(new_state);
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::do_invert_compose
//       Access: Private
//  Description: The private implemention of invert_compose().
////////////////////////////////////////////////////////////////////
CPT(RenderState) RenderState::
do_invert_compose(const RenderState *other) const {
  Attributes::const_iterator ai = _attributes.begin();
  Attributes::const_iterator bi = other->_attributes.begin();

  // Create a new RenderState that will hold the result.
  RenderState *new_state = new RenderState;
  back_insert_iterator<Attributes> result = 
    back_inserter(new_state->_attributes);

  while (ai != _attributes.end() && bi != other->_attributes.end()) {
    if ((*ai) < (*bi)) {
      // Here is an attribute that we have in the original, which is
      // not present in the secondary.
      *result = Attribute((*ai)._attrib->invert_compose((*ai)._attrib->make_default()), 0);
      ++ai;
      ++result;
    } else if ((*bi) < (*ai)) {
      // Here is a new attribute we have in the secondary, that was
      // not present in the original.
      *result = *bi;
      ++bi;
      ++result;
    } else {
      // Here is an attribute we have in both.  In this case, override
      // is meaningless.
      *result = Attribute((*ai)._attrib->invert_compose((*bi)._attrib), (*bi)._override);
      ++ai;
      ++bi;
      ++result;
    }
  }

  while (ai != _attributes.end()) {
    *result = Attribute((*ai)._attrib->invert_compose((*ai)._attrib->make_default()), 0);
    ++ai;
    ++result;
  }

  while (bi != other->_attributes.end()) {
    *result = *bi;
    ++bi;
    ++result;
  }

  return return_new(new_state);
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::determine_bin_index
//       Access: Private
//  Description: This is the private implementation of
//               get_bin_index() and get_draw_order().
////////////////////////////////////////////////////////////////////
void RenderState::
determine_bin_index() {
  string bin_name;
  _draw_order = 0;

  const RenderAttrib *attrib = get_attrib(CullBinAttrib::get_class_type());
  if (attrib != (const RenderAttrib *)NULL) {
    const CullBinAttrib *bin_attrib = DCAST(CullBinAttrib, attrib);
    bin_name = bin_attrib->get_bin_name();
    _draw_order = bin_attrib->get_draw_order();
  }

  if (bin_name.empty()) {
    // No explicit bin is specified; put in the in the default bin,
    // either opaque or transparent, based on the transparency
    // setting.
    bin_name = "opaque";
    const RenderAttrib *attrib = get_attrib(TransparencyAttrib::get_class_type());
    if (attrib != (const RenderAttrib *)NULL) {
      const TransparencyAttrib *trans = DCAST(TransparencyAttrib, attrib);
      switch (trans->get_mode()) {
      case TransparencyAttrib::M_alpha:
      case TransparencyAttrib::M_alpha_sorted:
      case TransparencyAttrib::M_binary:
        // These transparency modes require special back-to-front sorting.
        bin_name = "transparent";
        break;

      default:
        break;
      }
    }
  }

  CullBinManager *bin_manager = CullBinManager::get_global_ptr();
  _bin_index = bin_manager->find_bin(bin_name);
  if (_bin_index == -1) {
    pgraph_cat.warning()
      << "No bin named " << bin_name << "; creating default bin.\n";
    _bin_index = bin_manager->add_bin(bin_name, CullBinManager::BT_unsorted, 0);
  }
  _flags |= F_checked_bin_index;
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::determine_fog
//       Access: Private
//  Description: This is the private implementation of get_fog().
////////////////////////////////////////////////////////////////////
void RenderState::
determine_fog() {
  const RenderAttrib *attrib = get_attrib(FogAttrib::get_class_type());
  _fog = (const FogAttrib *)NULL;
  if (attrib != (const RenderAttrib *)NULL) {
    _fog = DCAST(FogAttrib, attrib);
  }
  _flags |= F_checked_fog;
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::determine_transparency
//       Access: Private
//  Description: This is the private implementation of get_transparency().
////////////////////////////////////////////////////////////////////
void RenderState::
determine_transparency() {
  const RenderAttrib *attrib = 
    get_attrib(TransparencyAttrib::get_class_type());
  _transparency = (const TransparencyAttrib *)NULL;
  if (attrib != (const RenderAttrib *)NULL) {
    _transparency = DCAST(TransparencyAttrib, attrib);
  }
  _flags |= F_checked_transparency;
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::register_with_read_factory
//       Access: Public, Static
//  Description: Tells the BamReader how to create objects of type
//               RenderState.
////////////////////////////////////////////////////////////////////
void RenderState::
register_with_read_factory() {
  BamReader::get_factory()->register_factory(get_class_type(), make_from_bam);
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::write_datagram
//       Access: Public, Virtual
//  Description: Writes the contents of this object to the datagram
//               for shipping out to a Bam file.
////////////////////////////////////////////////////////////////////
void RenderState::
write_datagram(BamWriter *manager, Datagram &dg) {
  TypedWritable::write_datagram(manager, dg);

  int num_attribs = _attributes.size();
  nassertv(num_attribs == (int)(PN_uint16)num_attribs);
  dg.add_uint16(num_attribs);

  // **** We should smarten up the writing of the override
  // number--most of the time these will all be zero.
  Attributes::const_iterator ai;
  for (ai = _attributes.begin(); ai != _attributes.end(); ++ai) {
    const Attribute &attribute = (*ai);

    manager->write_pointer(dg, attribute._attrib);
    dg.add_int32(attribute._override);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::complete_pointers
//       Access: Public, Virtual
//  Description: Receives an array of pointers, one for each time
//               manager->read_pointer() was called in fillin().
//               Returns the number of pointers processed.
////////////////////////////////////////////////////////////////////
int RenderState::
complete_pointers(TypedWritable **p_list, BamReader *manager) {
  int pi = TypedWritable::complete_pointers(p_list, manager);

  // Get the attribute pointers.
  Attributes::iterator ai;
  for (ai = _attributes.begin(); ai != _attributes.end(); ++ai) {
    Attribute &attribute = (*ai);

    attribute._attrib = DCAST(RenderAttrib, p_list[pi++]);
    nassertr(attribute._attrib != (RenderAttrib *)NULL, pi);
    attribute._type = attribute._attrib->get_type();
  }

  // Now make sure the array is properly sorted.  (It won't
  // necessarily preserve its correct sort after being read from bam,
  // because the sort is based on TypeHandle indices and raw pointers,
  // both of which can change from session to session.)
  _attributes.sort();

  return pi;
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::change_this
//       Access: Public, Static
//  Description: Called immediately after complete_pointers(), this
//               gives the object a chance to adjust its own pointer
//               if desired.  Most objects don't change pointers after
//               completion, but some need to.
//
//               Once this function has been called, the old pointer
//               will no longer be accessed.
////////////////////////////////////////////////////////////////////
TypedWritable *RenderState::
change_this(TypedWritable *old_ptr, BamReader *manager) {
  // First, uniquify the pointer.
  RenderState *state = DCAST(RenderState, old_ptr);
  CPT(RenderState) pointer = return_new(state);

  // But now we have a problem, since we have to hold the reference
  // count and there's no way to return a TypedWritable while still
  // holding the reference count!  We work around this by explicitly
  // upping the count, and also setting a finalize() callback to down
  // it later.
  if (pointer == state) {
    pointer->ref();
    manager->register_finalize(state);
  }
  
  // We have to cast the pointer back to non-const, because the bam
  // reader expects that.
  return (RenderState *)pointer.p();
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::finalize
//       Access: Public, Virtual
//  Description: Called by the BamReader to perform any final actions
//               needed for setting up the object after all objects
//               have been read and all pointers have been completed.
////////////////////////////////////////////////////////////////////
void RenderState::
finalize() {
  // Unref the pointer that we explicitly reffed in make_from_bam().
  unref();

  // We should never get back to zero after unreffing our own count,
  // because we expect to have been stored in a pointer somewhere.  If
  // we do get to zero, it's a memory leak; the way to avoid this is
  // to call unref_delete() above instead of unref(), but this is
  // dangerous to do from within a virtual function.
  nassertv(get_ref_count() != 0);
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::make_from_bam
//       Access: Protected, Static
//  Description: This function is called by the BamReader's factory
//               when a new object of type RenderState is encountered
//               in the Bam file.  It should create the RenderState
//               and extract its information from the file.
////////////////////////////////////////////////////////////////////
TypedWritable *RenderState::
make_from_bam(const FactoryParams &params) {
  RenderState *state = new RenderState;
  DatagramIterator scan;
  BamReader *manager;

  parse_params(params, scan, manager);
  state->fillin(scan, manager);
  manager->register_change_this(change_this, state);

  return state;
}

////////////////////////////////////////////////////////////////////
//     Function: RenderState::fillin
//       Access: Protected
//  Description: This internal function is called by make_from_bam to
//               read in all of the relevant data from the BamFile for
//               the new RenderState.
////////////////////////////////////////////////////////////////////
void RenderState::
fillin(DatagramIterator &scan, BamReader *manager) {
  TypedWritable::fillin(scan, manager);

  int num_attribs = scan.get_uint16();

  // Push back a NULL pointer for each attribute for now, until we get
  // the actual list of pointers later in complete_pointers().
  _attributes.reserve(num_attribs);
  for (int i = 0; i < num_attribs; i++) {
    manager->read_pointer(scan);
    int override = scan.get_int32();
    _attributes.push_back(Attribute(override));
  }
}
