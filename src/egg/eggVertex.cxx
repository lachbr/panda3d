// Filename: eggVertex.cxx
// Created by:  drose (16Jan99)
// 
////////////////////////////////////////////////////////////////////

#include "eggVertex.h"
#include "eggVertexPool.h"
#include "eggParameters.h"
#include "eggGroup.h"
#include "eggMiscFuncs.h"
#include "eggPrimitive.h"

#include <indent.h>
#include <luse.h>
#include <lmatrix.h>
#include <pandabase.h>

#include <math.h>
#include <algorithm>

TypeHandle EggVertex::_type_handle;


////////////////////////////////////////////////////////////////////
//     Function: EggVertex::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
EggVertex::
EggVertex() {
  _pool = NULL;
  _index = -1;
  set_pos(LPoint3d(0.0, 0.0, 0.0));
  test_pref_integrity();
  test_gref_integrity();
}

////////////////////////////////////////////////////////////////////
//     Function: EggVertex::Copy constructor
//       Access: Public
//  Description: Copies all properties of the vertex except its vertex
//               pool and index number.
////////////////////////////////////////////////////////////////////
EggVertex::
EggVertex(const EggVertex &copy) 
  : EggObject(copy), EggAttributes(copy),
    _dxyzs(copy._dxyzs),
    _pos(copy._pos),
    _num_dimensions(copy._num_dimensions)
{
  _pool = NULL;
  _index = -1;
  test_pref_integrity();
  test_gref_integrity();
}


////////////////////////////////////////////////////////////////////
//     Function: EggVertex::Copy assignment operator
//       Access: Public
//  Description: Copies all properties of the vertex except its vertex
//               pool and index number.
////////////////////////////////////////////////////////////////////
EggVertex &EggVertex::
operator = (const EggVertex &copy) {
  EggObject::operator = (copy);
  EggAttributes::operator = (copy);
  _pos = copy._pos;
  _num_dimensions = copy._num_dimensions;
  _dxyzs = copy._dxyzs;

  test_pref_integrity();
  test_gref_integrity();

  return *this;
}

////////////////////////////////////////////////////////////////////
//     Function: EggVertex::Destructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
EggVertex::
~EggVertex() {
  // We should never destruct a vertex while it still thinks it
  // belongs to a VertexPool.  If we do, we've probably lost a
  // reference count somewhere.
  nassertv(_pool == NULL);

  // Also, a vertex shouldn't be destructed while it's being
  // referenced by a group or a primitive, for the same reason.
  nassertv(_gref.empty());
  nassertv(_pref.empty());
}



///////////////////////////////////////////////////////////////////
// 	 Class : GroupRefEntry
// Description : A temporary class used in EggVertex::write(), below,
//               to hold the groups that reference each vertex prior
//               to outputting them as a formatted list.
////////////////////////////////////////////////////////////////////
class GroupRefEntry {
public:
  GroupRefEntry(EggGroup *group, double membership)
    : _group(group), _membership(membership) { }

  bool operator < (const GroupRefEntry &other) const {
    return _group->get_name() < other._group->get_name();
  }
  void output(ostream &out) const {
    out << _group->get_name() << ":" << _membership;
  }

  EggGroup *_group;
  double _membership;
};

INLINE ostream &operator << (ostream &out, const GroupRefEntry &gre) {
  gre.output(out);
  return out;
}

////////////////////////////////////////////////////////////////////
//     Function: EggVertex::write
//       Access: Public
//  Description: Writes the vertex to the indicated output stream in
//               Egg format.
////////////////////////////////////////////////////////////////////
void EggVertex::
write(ostream &out, int indent_level) const {
  test_pref_integrity();
  test_gref_integrity();

  indent(out, indent_level)
    << "<Vertex> " << _index << " {\n";

  // Now output the position.  This might have any number of
  // dimensions up to 4.
  indent(out, indent_level+1);
  for (int i = 0; i < _num_dimensions; i++) {
    out << " " << _pos[i];
  }
  out << "\n";

  EggAttributes::write(out, indent_level+2);

  _dxyzs.write(out, indent_level+2);

  // If the vertex is referenced by one or more groups, write that as
  // a helpful comment.
  if (!_gref.empty()) {
    // We need to build a list of group entries.
    set<GroupRefEntry> gre;
    
    GroupRef::const_iterator gi;
    for (gi = _gref.begin(); gi != _gref.end(); ++gi) {
      gre.insert(GroupRefEntry(*gi, (*gi)->get_vertex_membership(this)));
    }

    // Now output the list.
    write_long_list(out, indent_level + 2, gre.begin(), gre.end(), "// ",
	"", 72);
  }

  indent(out, indent_level)
    << "}\n";
}


////////////////////////////////////////////////////////////////////
//     Function: EggVertex::sorts_less_than
//       Access: Public
//  Description: An ordering operator to compare two vertices for
//               sorting order.  This imposes an arbitrary ordering
//               useful to identify unique vertices.
////////////////////////////////////////////////////////////////////
bool EggVertex::
sorts_less_than(const EggVertex &other) const {
  if (_num_dimensions != other._num_dimensions) {
    return _num_dimensions < other._num_dimensions;
  }

  int compare =
    _pos.compare_to(other._pos, egg_parameters->_pos_threshold);
  if (compare != 0) {
    return compare < 0;
  }
  if (_dxyzs != other._dxyzs) {
    return _dxyzs < other._dxyzs;
  }

  return EggAttributes::sorts_less_than(other);
}

////////////////////////////////////////////////////////////////////
//     Function: EggVertex::get_num_local_coord
//       Access: Public
//  Description: Returns the number of primitives that own this vertex
//               whose vertices are interpreted to be in a local
//               coordinate system.
////////////////////////////////////////////////////////////////////
int EggVertex::
get_num_local_coord() const {
  test_pref_integrity();

  PrimitiveRef::const_iterator pri;

  int count = 0;
  for (pri = pref_begin(); pri != pref_end(); ++pri) {
    EggPrimitive *prim = *pri;
    count += (prim->is_local_coord() ? 1 : 0);
  }
  return count;
}

////////////////////////////////////////////////////////////////////
//     Function: EggVertex::get_num_global_coord
//       Access: Public
//  Description: Returns the number of primitives that own this vertex
//               whose vertices are interpreted in the global
//               coordinate system.
////////////////////////////////////////////////////////////////////
int EggVertex::
get_num_global_coord() const {
  test_pref_integrity();

  PrimitiveRef::const_iterator pri;

  int count = 0;
  for (pri = pref_begin(); pri != pref_end(); ++pri) {
    EggPrimitive *prim = *pri;
    count += (prim->is_local_coord() ? 0 : 1);
  }
  return count;
}


////////////////////////////////////////////////////////////////////
//     Function: EggVertex::transform
//       Access: Public, Virtual
//  Description: Applies the indicated transformation matrix to the
//               vertex.
////////////////////////////////////////////////////////////////////
void EggVertex::
transform(const LMatrix4d &mat) {
  _pos = _pos * mat;

  EggMorphVertexList::iterator mi;
  for (mi = _dxyzs.begin(); mi != _dxyzs.end(); ++mi) {
    // We can safely cast the morph object to a non-const, because
    // we're not changing its name, which is the only thing the set
    // cares about preserving.
    EggMorphVertex &morph = (EggMorphVertex &)(*mi);

    morph.set_offset((*mi).get_offset() * mat);
  }

  EggAttributes::transform(mat);
}


////////////////////////////////////////////////////////////////////
//     Function: EggVertex::copy_grefs_from
//       Access: Public
//  Description: Copies all the group references from the other vertex
//               onto this one.  This assigns the current vertex to
//               exactly the same groups, with exactly the same
//               memberships, as the given one.
////////////////////////////////////////////////////////////////////
void EggVertex::
copy_grefs_from(const EggVertex &other) {
  if (&other == this) {
    return;
  }
  test_gref_integrity();
  other.test_gref_integrity();

  clear_grefs();
  test_gref_integrity();

  GroupRef::const_iterator gri;

  for (gri = other.gref_begin(); gri != other.gref_end(); ++gri) {
    EggGroup *group = *gri;
    nassertv(group != NULL);

    group->ref_vertex(this, group->get_vertex_membership(&other));
  }
}

////////////////////////////////////////////////////////////////////
//     Function: EggVertex::clear_grefs
//       Access: Public
//  Description: Removes all group references from the vertex, so that
//               it is not assigned to any group.
////////////////////////////////////////////////////////////////////
void EggVertex::
clear_grefs() {
  GroupRef gref_copy = _gref;
  GroupRef::const_iterator gri;
  for (gri = gref_copy.begin(); gri != gref_copy.end(); ++gri) {
    EggGroup *group = *gri;
    nassertv(group != NULL);
    group->unref_vertex(this);
  }

  // Now we should have no more refs.
  nassertv(_gref.empty());
}

#ifndef NDEBUG

////////////////////////////////////////////////////////////////////
//     Function: EggVertex::test_gref_integrity
//       Access: Public
//  Description: Verifies that the gref list is correct and that all
//               the groups included actually exist and do reference
//               the vertex.
////////////////////////////////////////////////////////////////////
void EggVertex::
test_gref_integrity() const {
  test_ref_count_integrity();

  GroupRef::const_iterator gri;

  for (gri = gref_begin(); gri != gref_end(); ++gri) {
    EggGroup *group = *gri;
    nassertv(group != NULL);
    group->test_ref_count_integrity();

    double membership = group->get_vertex_membership(this);
    nassertv(membership != 0.0);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: EggVertex::test_pref_integrity
//       Access: Public
//  Description: Verifies that the pref list is correct and that all
//               the primitives included actually exist and do
//               reference the vertex.
////////////////////////////////////////////////////////////////////
void EggVertex::
test_pref_integrity() const {
  test_ref_count_integrity();

  PrimitiveRef::const_iterator pri;

  for (pri = pref_begin(); pri != pref_end(); ++pri) {
    EggPrimitive *prim = *pri;
    nassertv(prim != NULL);
    prim->test_ref_count_integrity();

    EggPrimitive::iterator vi;
    vi = find(prim->begin(), prim->end(), this);
    nassertv(vi != prim->end());
  }
}

#endif  // NDEBUG

////////////////////////////////////////////////////////////////////
//     Function: EggVertex::output
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
void EggVertex::
output(ostream &out) const {
  if (get_pool() == NULL) {
    out << "(null):" << get_index();
  } else {
    out << get_pool()->get_name() << ":" << get_index();
  }
}
