// Filename: geomPrimitive.cxx
// Created by:  drose (06Mar05)
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

#include "geomPrimitive.h"
#include "geom.h"
#include "geomVertexData.h"
#include "geomVertexArrayFormat.h"
#include "geomVertexColumn.h"
#include "geomVertexReader.h"
#include "geomVertexWriter.h"
#include "geomVertexRewriter.h"
#include "preparedGraphicsObjects.h"
#include "internalName.h"
#include "bamReader.h"
#include "bamWriter.h"
#include "ioPtaDatagramInt.h"
#include "indent.h"
#include "pStatTimer.h"

TypeHandle GeomPrimitive::_type_handle;

PStatCollector GeomPrimitive::_decompose_pcollector("*:Munge:Decompose");
PStatCollector GeomPrimitive::_rotate_pcollector("*:Munge:Rotate");

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::Default Constructor
//       Access: Protected
//  Description: Constructs an invalid object.  Only used when reading
//               from bam.
////////////////////////////////////////////////////////////////////
GeomPrimitive::
GeomPrimitive() {
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::Constructor
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
GeomPrimitive::
GeomPrimitive(GeomPrimitive::UsageHint usage_hint) {
  CDWriter cdata(_cycler, true);
  cdata->_usage_hint = usage_hint;
}
 
////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::Copy Constructor
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
GeomPrimitive::
GeomPrimitive(const GeomPrimitive &copy) :
  TypedWritableReferenceCount(copy),
  _cycler(copy._cycler)
{
}
  
////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::Copy Assignment Operator
//       Access: Published
//  Description: The copy assignment operator is not pipeline-safe.
//               This will completely obliterate all stages of the
//               pipeline, so don't do it for a GeomPrimitive that is
//               actively being used for rendering.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
operator = (const GeomPrimitive &copy) {
  TypedWritableReferenceCount::operator = (copy);
  _cycler = copy._cycler;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::Destructor
//       Access: Published, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
GeomPrimitive::
~GeomPrimitive() {
  release_all();
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::get_geom_rendering
//       Access: Published, Virtual
//  Description: Returns the set of GeomRendering bits that represent
//               the rendering properties required to properly render
//               this primitive.
////////////////////////////////////////////////////////////////////
int GeomPrimitive::
get_geom_rendering() const {
  if (is_indexed()) {
    return GR_indexed_other;
  } else {
    return 0;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::set_usage_hint
//       Access: Published
//  Description: Changes the UsageHint hint for this primitive.  See
//               get_usage_hint().
//
//               Don't call this in a downstream thread unless you
//               don't mind it blowing away other changes you might
//               have recently made in an upstream thread.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
set_usage_hint(GeomPrimitive::UsageHint usage_hint) {
  CDWriter cdata(_cycler, true);
  cdata->_usage_hint = usage_hint;

  if (cdata->_vertices != (GeomVertexArrayData *)NULL) {
    if (cdata->_vertices->get_ref_count() > 1) {
      cdata->_vertices = new GeomVertexArrayData(*cdata->_vertices);
    }
    
    cdata->_modified = Geom::get_next_modified();
    cdata->_usage_hint = usage_hint;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::set_index_type
//       Access: Published
//  Description: Changes the numeric type of the index column.
//               Normally, this should be either NT_uint16 or
//               NT_uint32.
//
//               Don't call this in a downstream thread unless you
//               don't mind it blowing away other changes you might
//               have recently made in an upstream thread.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
set_index_type(GeomPrimitive::NumericType index_type) {
  CDWriter cdata(_cycler, true);
  cdata->_index_type = index_type;

  if (cdata->_vertices != (GeomVertexArrayData *)NULL) {
    CPT(GeomVertexArrayFormat) new_format = get_index_format();
    
    if (cdata->_vertices->get_array_format() != new_format) {
      PT(GeomVertexArrayData) new_vertices = make_index_data();
      new_vertices->set_num_rows(cdata->_vertices->get_num_rows());

      GeomVertexReader from(cdata->_vertices, 0);
      GeomVertexWriter to(new_vertices, 0);
      
      while (!from.is_at_end()) {
        to.set_data1i(from.get_data1i());
      }
      cdata->_vertices = new_vertices;
      cdata->_got_minmax = false;
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::add_vertex
//       Access: Published
//  Description: Adds the indicated vertex to the list of vertex
//               indices used by the graphics primitive type.  To
//               define primitive, you must call add_vertex() for each
//               vertex of the new primitve, and then call
//               close_primitive() after you have specified the last
//               vertex of each primitive.
//
//               Don't call this in a downstream thread unless you
//               don't mind it blowing away other changes you might
//               have recently made in an upstream thread.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
add_vertex(int vertex) {
  CDWriter cdata(_cycler, true);

  int num_primitives = get_num_primitives();
  if (num_primitives > 0 &&
      requires_unused_vertices() && 
      get_num_vertices() == get_primitive_end(num_primitives - 1)) {
    // If we are beginning a new primitive, give the derived class a
    // chance to insert some degenerate vertices.
    if (cdata->_vertices == (GeomVertexArrayData *)NULL) {
      do_make_indexed(cdata);
    }
    append_unused_vertices(cdata->_vertices, vertex);
  }

  if (cdata->_vertices == (GeomVertexArrayData *)NULL) {
    // The nonindexed case.  We can keep the primitive nonindexed only
    // if the vertex number happens to be the next available vertex.
    if (cdata->_num_vertices == 0) {
      cdata->_first_vertex = vertex;
      cdata->_num_vertices = 1;
      cdata->_modified = Geom::get_next_modified();
      cdata->_got_minmax = false;
      return;

    } else if (vertex == cdata->_first_vertex + cdata->_num_vertices) {
      ++cdata->_num_vertices;
      cdata->_modified = Geom::get_next_modified();
      cdata->_got_minmax = false;
      return;
    }
    
    // Otherwise, we need to suddenly become an indexed primitive.
    do_make_indexed(cdata);
  }

  GeomVertexWriter index(cdata->_vertices, 0);
  index.set_row(cdata->_vertices->get_num_rows());

  index.add_data1i(vertex);

  cdata->_modified = Geom::get_next_modified();
  cdata->_got_minmax = false;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::add_consecutive_vertices
//       Access: Published
//  Description: Adds a consecutive sequence of vertices, beginning at
//               start, to the primitive.
//
//               Don't call this in a downstream thread unless you
//               don't mind it blowing away other changes you might
//               have recently made in an upstream thread.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
add_consecutive_vertices(int start, int num_vertices) {
  if (num_vertices == 0) {
    return;
  }
  int end = (start + num_vertices) - 1;

  CDWriter cdata(_cycler, true);

  int num_primitives = get_num_primitives();
  if (num_primitives > 0 &&
      get_num_vertices() == get_primitive_end(num_primitives - 1)) {
    // If we are beginning a new primitive, give the derived class a
    // chance to insert some degenerate vertices.
    if (cdata->_vertices == (GeomVertexArrayData *)NULL) {
      do_make_indexed(cdata);
    }
    append_unused_vertices(cdata->_vertices, start);
  }

  if (cdata->_vertices == (GeomVertexArrayData *)NULL) {
    // The nonindexed case.  We can keep the primitive nonindexed only
    // if the vertex number happens to be the next available vertex.
    if (cdata->_num_vertices == 0) {
      cdata->_first_vertex = start;
      cdata->_num_vertices = num_vertices;
      cdata->_modified = Geom::get_next_modified();
      cdata->_got_minmax = false;
      return;

    } else if (start == cdata->_first_vertex + cdata->_num_vertices) {
      cdata->_num_vertices += num_vertices;
      cdata->_modified = Geom::get_next_modified();
      cdata->_got_minmax = false;
      return;
    }
    
    // Otherwise, we need to suddenly become an indexed primitive.
    do_make_indexed(cdata);
  }

  GeomVertexWriter index(cdata->_vertices, 0);
  index.set_row(cdata->_vertices->get_num_rows());

  for (int v = start; v <= end; ++v) {
    index.add_data1i(v);
  }

  cdata->_modified = Geom::get_next_modified();
  cdata->_got_minmax = false;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::add_next_vertices
//       Access: Published
//  Description: Adds the next n vertices in sequence, beginning from
//               the last vertex added to the primitive + 1.
//
//               This is most useful when you are building up a
//               primitive and a GeomVertexData at the same time, and
//               you just want the primitive to reference the first n
//               vertices from the data, then the next n, and so on.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
add_next_vertices(int num_vertices) {
  if (get_num_vertices() == 0) {
    add_consecutive_vertices(0, num_vertices);
  } else {
    add_consecutive_vertices(get_vertex(get_num_vertices() - 1) + 1, num_vertices);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::close_primitive
//       Access: Published
//  Description: Indicates that the previous n calls to add_vertex(),
//               since the last call to close_primitive(), have fully
//               defined a new primitive.  Returns true if successful,
//               false otherwise.
//
//               Don't call this in a downstream thread unless you
//               don't mind it blowing away other changes you might
//               have recently made in an upstream thread.
////////////////////////////////////////////////////////////////////
bool GeomPrimitive::
close_primitive() {
  int num_vertices_per_primitive = get_num_vertices_per_primitive();

  CDWriter cdata(_cycler, true);
  if (num_vertices_per_primitive == 0) {
    // This is a complex primitive type like a triangle strip: each
    // primitive uses a different number of vertices.
#ifndef NDEBUG
    int num_added;
    if (cdata->_ends.empty()) {
      num_added = get_num_vertices();
    } else {
      num_added = get_num_vertices() - cdata->_ends.back();
      num_added -= get_num_unused_vertices_per_primitive();
    }
    nassertr(num_added >= get_min_num_vertices_per_primitive(), false);
#endif
    if (cdata->_ends.get_ref_count() > 1) {
      PTA_int new_ends;
      new_ends.v() = cdata->_ends.v();
      cdata->_ends = new_ends;
    }
    cdata->_ends.push_back(get_num_vertices());

  } else {
#ifndef NDEBUG
    // This is a simple primitive type like a triangle: each primitive
    // uses the same number of vertices.  Assert that we added the
    // correct number of vertices.
    int num_vertices_per_primitive = get_num_vertices_per_primitive();
    int num_unused_vertices_per_primitive = get_num_unused_vertices_per_primitive();

    int num_vertices = get_num_vertices();
    nassertr((num_vertices + num_unused_vertices_per_primitive) % (num_vertices_per_primitive + num_unused_vertices_per_primitive) == 0, false)
#endif
  }

  cdata->_modified = Geom::get_next_modified();

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::clear_vertices
//       Access: Published
//  Description: Removes all of the vertices and primitives from the
//               object, so they can be re-added.
//
//               Don't call this in a downstream thread unless you
//               don't mind it blowing away other changes you might
//               have recently made in an upstream thread.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
clear_vertices() {
  CDWriter cdata(_cycler, true);
  cdata->_first_vertex = 0;
  cdata->_num_vertices = 0;
  cdata->_vertices.clear();
  cdata->_ends.clear();
  cdata->_mins.clear();
  cdata->_maxs.clear();
  cdata->_modified = Geom::get_next_modified();
  cdata->_got_minmax = false;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::offset_vertices
//       Access: Published
//  Description: Adds the indicated offset to all vertices used by the
//               primitive.
//
//               Don't call this in a downstream thread unless you
//               don't mind it blowing away other changes you might
//               have recently made in an upstream thread.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
offset_vertices(int offset) {
  if (is_indexed()) {
    GeomVertexRewriter index(modify_vertices(), 0);
    while (!index.is_at_end()) {
      index.set_data1i(index.get_data1i() + offset);
    }

  } else {
    CDWriter cdata(_cycler, true);
    cdata->_first_vertex += offset;
    cdata->_modified = Geom::get_next_modified();
    cdata->_got_minmax = false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::make_nonindexed
//       Access: Published
//  Description: Converts the primitive from indexed to nonindexed by
//               duplicating vertices as necessary into the indicated
//               dest GeomVertexData.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
make_nonindexed(GeomVertexData *dest, const GeomVertexData *source) {
  Thread *current_thread = Thread::get_current_thread();
  int num_vertices = get_num_vertices();
  int dest_start = dest->get_num_rows();

  dest->set_num_rows(dest_start + num_vertices);
  for (int i = 0; i < num_vertices; ++i) {
    int v = get_vertex(i);
    dest->copy_row_from(dest_start + i, source, v, current_thread);
  }

  set_nonindexed_vertices(dest_start, num_vertices);
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::pack_vertices
//       Access: Published
//  Description: Packs the vertices used by the primitive from the
//               indicated source array onto the end of the indicated
//               destination array.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
pack_vertices(GeomVertexData *dest, const GeomVertexData *source) {
  Thread *current_thread = Thread::get_current_thread();
  if (!is_indexed()) {
    // If the primitive is nonindexed, packing is the same as
    // converting (again) to nonindexed.
    make_nonindexed(dest, source);

  } else {
    // The indexed case: build up a new index as we go.
    CPT(GeomVertexArrayData) orig_vertices = get_vertices();
    PT(GeomVertexArrayData) new_vertices = make_index_data();
    GeomVertexWriter index(new_vertices, 0);
    typedef pmap<int, int> CopiedIndices;
    CopiedIndices copied_indices;

    int num_vertices = get_num_vertices();
    int dest_start = dest->get_num_rows();

    for (int i = 0; i < num_vertices; ++i) {
      int v = get_vertex(i);

      // Try to add the relation { v : size() }.  If that succeeds,
      // great; if it doesn't, look up whatever we previously added
      // for v.
      pair<CopiedIndices::iterator, bool> result = 
        copied_indices.insert(CopiedIndices::value_type(v, (int)copied_indices.size()));
      int v2 = (*result.first).second + dest_start;
      index.add_data1i(v2);

      if (result.second) {
        // This is the first time we've seen vertex v.
        dest->copy_row_from(v2, source, v, current_thread);
      }
    }
    
    set_vertices(new_vertices);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::make_indexed
//       Access: Published
//  Description: Converts the primitive from nonindexed form to
//               indexed form.  This will simply create an index table
//               that is numbered consecutively from
//               get_first_vertex(); it does not automatically
//               collapse together identical vertices that may have
//               been split apart by a previous call to
//               make_nonindexed().
//
//               Don't call this in a downstream thread unless you
//               don't mind it blowing away other changes you might
//               have recently made in an upstream thread.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
make_indexed() {
  CDWriter cdata(_cycler, true);
  do_make_indexed(cdata);
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::get_primitive_start
//       Access: Published
//  Description: Returns the element within the _vertices list at which
//               the nth primitive starts.  
//
//               If i is one more than the highest valid primitive
//               vertex, the return value will be one more than the
//               last valid vertex.  Thus, it is generally true that
//               the vertices used by a particular primitive i are the
//               set get_primitive_start(n) <= vi <
//               get_primitive_start(n + 1) (although this range also
//               includes the unused vertices between primitives).
////////////////////////////////////////////////////////////////////
int GeomPrimitive::
get_primitive_start(int n) const {
  int num_vertices_per_primitive = get_num_vertices_per_primitive();
  int num_unused_vertices_per_primitive = get_num_unused_vertices_per_primitive();

  if (num_vertices_per_primitive == 0) {
    // This is a complex primitive type like a triangle strip: each
    // primitive uses a different number of vertices.
    CDReader cdata(_cycler);
    nassertr(n >= 0 && n <= (int)cdata->_ends.size(), -1);
    if (n == 0) {
      return 0;
    } else {
      return cdata->_ends[n - 1] + num_unused_vertices_per_primitive;
    }

  } else {
    // This is a simple primitive type like a triangle: each primitive
    // uses the same number of vertices.
    return n * (num_vertices_per_primitive + num_unused_vertices_per_primitive);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::get_primitive_end
//       Access: Published
//  Description: Returns the element within the _vertices list at which
//               the nth primitive ends.  This is one past the last
//               valid element for the nth primitive.
////////////////////////////////////////////////////////////////////
int GeomPrimitive::
get_primitive_end(int n) const {
  int num_vertices_per_primitive = get_num_vertices_per_primitive();

  if (num_vertices_per_primitive == 0) {
    // This is a complex primitive type like a triangle strip: each
    // primitive uses a different number of vertices.
    CDReader cdata(_cycler);
    nassertr(n >= 0 && n < (int)cdata->_ends.size(), -1);
    return cdata->_ends[n];

  } else {
    // This is a simple primitive type like a triangle: each primitive
    // uses the same number of vertices.
    int num_unused_vertices_per_primitive = get_num_unused_vertices_per_primitive();
    return n * (num_vertices_per_primitive + num_unused_vertices_per_primitive) + num_vertices_per_primitive;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::get_primitive_num_vertices
//       Access: Published
//  Description: Returns the number of vertices used by the nth
//               primitive.  This is the same thing as
//               get_primitive_end(n) - get_primitive_start(n).
////////////////////////////////////////////////////////////////////
int GeomPrimitive::
get_primitive_num_vertices(int n) const {
  int num_vertices_per_primitive = get_num_vertices_per_primitive();

  if (num_vertices_per_primitive == 0) {
    // This is a complex primitive type like a triangle strip: each
    // primitive uses a different number of vertices.
    CDReader cdata(_cycler);
    nassertr(n >= 0 && n < (int)cdata->_ends.size(), 0);
    if (n == 0) {
      return cdata->_ends[0];
    } else {
      int num_unused_vertices_per_primitive = get_num_unused_vertices_per_primitive();
      return cdata->_ends[n] - cdata->_ends[n - 1] - num_unused_vertices_per_primitive;
    }      

  } else {
    // This is a simple primitive type like a triangle: each primitive
    // uses the same number of vertices.
    return num_vertices_per_primitive;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::get_primitive_min_vertex
//       Access: Published
//  Description: Returns the minimum vertex index number used by the
//               nth primitive in this object.
////////////////////////////////////////////////////////////////////
int GeomPrimitive::
get_primitive_min_vertex(int n) const {
  if (is_indexed()) {
    CPT(GeomVertexArrayData) mins = get_mins();
    nassertr(n >= 0 && n < mins->get_num_rows(), -1);

    GeomVertexReader index(mins, 0);
    index.set_row(n);
    return index.get_data1i();
  } else {
    return get_primitive_start(n);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::get_primitive_max_vertex
//       Access: Published
//  Description: Returns the maximum vertex index number used by the
//               nth primitive in this object.
////////////////////////////////////////////////////////////////////
int GeomPrimitive::
get_primitive_max_vertex(int n) const {
  if (is_indexed()) {
    CPT(GeomVertexArrayData) maxs = get_maxs();
    nassertr(n >= 0 && n < maxs->get_num_rows(), -1);

    GeomVertexReader index(maxs, 0);
    index.set_row(n);
    return index.get_data1i();
  } else {
    return get_primitive_end(n) - 1;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::decompose
//       Access: Published
//  Description: Decomposes a complex primitive type into a simpler
//               primitive type, for instance triangle strips to
//               triangles, and returns a pointer to the new primitive
//               definition.  If the decomposition cannot be
//               performed, this might return the original object.
//
//               This method is useful for application code that wants
//               to iterate through the set of triangles on the
//               primitive without having to write handlers for each
//               possible kind of primitive type.
////////////////////////////////////////////////////////////////////
CPT(GeomPrimitive) GeomPrimitive::
decompose() const {
  if (gobj_cat.is_debug()) {
    gobj_cat.debug()
      << "Decomposing " << get_type() << ": " << (void *)this << "\n";
  }

  PStatTimer timer(_decompose_pcollector);
  return decompose_impl();
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::rotate
//       Access: Published
//  Description: Returns a new primitive with the shade_model reversed
//               (if it is flat shaded), if possible.  If the
//               primitive type cannot be rotated, returns the
//               original primitive, unrotated.
//
//               If the current shade_model indicates
//               flat_vertex_last, this should bring the last vertex
//               to the first position; if it indicates
//               flat_vertex_first, this should bring the first vertex
//               to the last position.
////////////////////////////////////////////////////////////////////
CPT(GeomPrimitive) GeomPrimitive::
rotate() const {
  if (gobj_cat.is_debug()) {
    gobj_cat.debug()
      << "Rotating " << get_type() << ": " << (void *)this << "\n";
  }

  PStatTimer timer(_rotate_pcollector);
  CPT(GeomVertexArrayData) rotated_vertices = rotate_impl();

  if (rotated_vertices == (GeomVertexArrayData *)NULL) {
    // This primitive type can't be rotated.
    return this;
  }

  PT(GeomPrimitive) new_prim = make_copy();
  new_prim->set_vertices(rotated_vertices);

  switch (get_shade_model()) {
  case SM_flat_first_vertex:
    new_prim->set_shade_model(SM_flat_last_vertex);
    break;

  case SM_flat_last_vertex:
    new_prim->set_shade_model(SM_flat_first_vertex);
    break;

  default:
    break;
  }

  return new_prim;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::match_shade_model
//       Access: Published
//  Description: Returns a new primitive that is compatible with the
//               indicated shade model, if possible, or NULL if this
//               is not possible.
//
//               In most cases, this will return either NULL or the
//               original primitive.  In the case of a
//               SM_flat_first_vertex vs. a SM_flat_last_vertex (or
//               vice-versa), however, it will return a rotated
//               primitive.
////////////////////////////////////////////////////////////////////
CPT(GeomPrimitive) GeomPrimitive::
match_shade_model(GeomPrimitive::ShadeModel shade_model) const {
  ShadeModel this_shade_model = get_shade_model();
  if (this_shade_model == shade_model) {
    // Trivially compatible.
    return this;
  }

  if (this_shade_model == SM_uniform || shade_model == SM_uniform) {
    // SM_uniform is compatible with anything.
    return this;
  }

  if ((this_shade_model == SM_flat_first_vertex && shade_model == SM_flat_last_vertex) ||
      (this_shade_model == SM_flat_last_vertex && shade_model == SM_flat_first_vertex)) {
    // Needs to be rotated.
    CPT(GeomPrimitive) rotated = rotate();
    if (rotated.p() == this) {
      // Oops, can't be rotated, sorry.
      return NULL;
    }
    return rotated;
  }

  // Not compatible, sorry.
  return NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::get_num_bytes
//       Access: Published
//  Description: Returns the number of bytes consumed by the primitive
//               and its index table(s).
////////////////////////////////////////////////////////////////////
int GeomPrimitive::
get_num_bytes() const {
  CDReader cdata(_cycler);
  int num_bytes = cdata->_ends.size() * sizeof(int) + sizeof(GeomPrimitive);
  if (cdata->_vertices != (GeomVertexArrayData *)NULL) {
    num_bytes += cdata->_vertices->get_data_size_bytes();
  }

  return num_bytes;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::output
//       Access: Published, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
output(ostream &out) const {
  out << get_type() << ", " << get_num_primitives()
      << ", " << get_num_vertices();
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::write
//       Access: Published, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
write(ostream &out, int indent_level) const {
  indent(out, indent_level)
    << get_type();
  if (is_indexed()) {
    out << " (indexed)";
  } else {
    out << " (nonindexed)";
  }
  out << ":\n";
  int num_primitives = get_num_primitives();
  int num_vertices = get_num_vertices();
  int num_unused_vertices_per_primitive = get_num_unused_vertices_per_primitive();
  for (int i = 0; i < num_primitives; ++i) {
    indent(out, indent_level + 2)
      << "[";
    int begin = get_primitive_start(i);
    int end = get_primitive_end(i);
    for (int vi = begin; vi < end; vi++) {
      out << " " << get_vertex(vi);
    }
    out << " ]";
    if (end < num_vertices) {
      for (int ui = 0; ui < num_unused_vertices_per_primitive; ++ui) {
        if (end + ui < num_vertices) {
          out << " " << get_vertex(end + ui);
        } else {
          out << " ?";
        }
      }
    }
    out << "\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::modify_vertices
//       Access: Public
//  Description: Returns a modifiable pointer to the vertex index
//               list, so application code can directly fiddle with
//               this data.  Use with caution, since there are no
//               checks that the data will be left in a stable state.
//
//               If this is called on a nonindexed primitive, it will
//               implicitly be converted to an indexed primitive.
//
//               Don't call this in a downstream thread unless you
//               don't mind it blowing away other changes you might
//               have recently made in an upstream thread.
////////////////////////////////////////////////////////////////////
GeomVertexArrayData *GeomPrimitive::
modify_vertices() {
  CDWriter cdata(_cycler, true);

  if (cdata->_vertices == (GeomVertexArrayData *)NULL) {
    do_make_indexed(cdata);
  }

  if (cdata->_vertices->get_ref_count() > 1) {
    cdata->_vertices = new GeomVertexArrayData(*cdata->_vertices);
  }

  cdata->_modified = Geom::get_next_modified();
  cdata->_got_minmax = false;
  return cdata->_vertices;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::set_vertices
//       Access: Public
//  Description: Completely replaces the vertex index list with a new
//               table.  Chances are good that you should also replace
//               the ends list with set_ends() at the same time.
//
//               Don't call this in a downstream thread unless you
//               don't mind it blowing away other changes you might
//               have recently made in an upstream thread.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
set_vertices(const GeomVertexArrayData *vertices) {
  CDWriter cdata(_cycler, true);
  cdata->_vertices = (GeomVertexArrayData *)vertices;

  cdata->_modified = Geom::get_next_modified();
  cdata->_got_minmax = false;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::set_nonindexed_vertices
//       Access: Public
//  Description: Sets the primitive up as a nonindexed primitive,
//               using the indicated vertex range.
//
//               Don't call this in a downstream thread unless you
//               don't mind it blowing away other changes you might
//               have recently made in an upstream thread.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
set_nonindexed_vertices(int first_vertex, int num_vertices) {
  CDWriter cdata(_cycler, true);
  cdata->_vertices = (GeomVertexArrayData *)NULL;
  cdata->_first_vertex = first_vertex;
  cdata->_num_vertices = num_vertices;

  cdata->_modified = Geom::get_next_modified();
  cdata->_got_minmax = false;

  // Force the minmax to be recomputed.
  recompute_minmax(cdata);
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::modify_ends
//       Access: Public
//  Description: Returns a modifiable pointer to the primitive ends
//               array, so application code can directly fiddle with
//               this data.  Use with caution, since there are no
//               checks that the data will be left in a stable state.
//
//               Note that simple primitive types, like triangles, do
//               not have a ends array: since all the primitives
//               have the same number of vertices, it is not needed.
//
//               Don't call this in a downstream thread unless you
//               don't mind it blowing away other changes you might
//               have recently made in an upstream thread.
////////////////////////////////////////////////////////////////////
PTA_int GeomPrimitive::
modify_ends() {
  CDWriter cdata(_cycler, true);

  cdata->_modified = Geom::get_next_modified();
  cdata->_got_minmax = false;
  return cdata->_ends;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::set_ends
//       Access: Public
//  Description: Completely replaces the primitive ends array with
//               a new table.  Chances are good that you should also
//               replace the vertices list with set_vertices() at the
//               same time.
//
//               Note that simple primitive types, like triangles, do
//               not have a ends array: since all the primitives
//               have the same number of vertices, it is not needed.
//
//               Don't call this in a downstream thread unless you
//               don't mind it blowing away other changes you might
//               have recently made in an upstream thread.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
set_ends(CPTA_int ends) {
  CDWriter cdata(_cycler, true);
  cdata->_ends = (PTA_int &)ends;

  cdata->_modified = Geom::get_next_modified();
  cdata->_got_minmax = false;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::get_num_vertices_per_primitive
//       Access: Public, Virtual
//  Description: If the primitive type is a simple type in which all
//               primitives have the same number of vertices, like
//               triangles, returns the number of vertices per
//               primitive.  If the primitive type is a more complex
//               type in which different primitives might have
//               different numbers of vertices, for instance a
//               triangle strip, returns 0.
////////////////////////////////////////////////////////////////////
int GeomPrimitive::
get_num_vertices_per_primitive() const {
  return 0;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::get_min_num_vertices_per_primitive
//       Access: Public, Virtual
//  Description: Returns the minimum number of vertices that must be
//               added before close_primitive() may legally be called.
////////////////////////////////////////////////////////////////////
int GeomPrimitive::
get_min_num_vertices_per_primitive() const {
  return 3;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::get_num_unused_vertices_per_primitive
//       Access: Public, Virtual
//  Description: Returns the number of vertices that are added between
//               primitives that aren't, strictly speaking, part of
//               the primitives themselves.  This is used, for
//               instance, to define degenerate triangles to connect
//               otherwise disconnected triangle strips.
////////////////////////////////////////////////////////////////////
int GeomPrimitive::
get_num_unused_vertices_per_primitive() const {
  return 0;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::prepare
//       Access: Public
//  Description: Indicates that the data should be enqueued to be
//               prepared in the indicated prepared_objects at the
//               beginning of the next frame.  This will ensure the
//               data is already loaded into the GSG if it is expected
//               to be rendered soon.
//
//               Use this function instead of prepare_now() to preload
//               datas from a user interface standpoint.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
prepare(PreparedGraphicsObjects *prepared_objects) {
  prepared_objects->enqueue_index_buffer(this);
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::prepare_now
//       Access: Public
//  Description: Creates a context for the data on the particular
//               GSG, if it does not already exist.  Returns the new
//               (or old) IndexBufferContext.  This assumes that the
//               GraphicsStateGuardian is the currently active
//               rendering context and that it is ready to accept new
//               datas.  If this is not necessarily the case, you
//               should use prepare() instead.
//
//               Normally, this is not called directly except by the
//               GraphicsStateGuardian; a data does not need to be
//               explicitly prepared by the user before it may be
//               rendered.
////////////////////////////////////////////////////////////////////
IndexBufferContext *GeomPrimitive::
prepare_now(PreparedGraphicsObjects *prepared_objects, 
            GraphicsStateGuardianBase *gsg) {
  Contexts::const_iterator ci;
  ci = _contexts.find(prepared_objects);
  if (ci != _contexts.end()) {
    return (*ci).second;
  }

  IndexBufferContext *ibc = prepared_objects->prepare_index_buffer_now(this, gsg);
  if (ibc != (IndexBufferContext *)NULL) {
    _contexts[prepared_objects] = ibc;
  }
  return ibc;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::release
//       Access: Public
//  Description: Frees the data context only on the indicated object,
//               if it exists there.  Returns true if it was released,
//               false if it had not been prepared.
////////////////////////////////////////////////////////////////////
bool GeomPrimitive::
release(PreparedGraphicsObjects *prepared_objects) {
  Contexts::iterator ci;
  ci = _contexts.find(prepared_objects);
  if (ci != _contexts.end()) {
    IndexBufferContext *ibc = (*ci).second;
    prepared_objects->release_index_buffer(ibc);
    return true;
  }

  // Maybe it wasn't prepared yet, but it's about to be.
  return prepared_objects->dequeue_index_buffer(this);
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::release_all
//       Access: Public
//  Description: Frees the context allocated on all objects for which
//               the data has been declared.  Returns the number of
//               contexts which have been freed.
////////////////////////////////////////////////////////////////////
int GeomPrimitive::
release_all() {
  // We have to traverse a copy of the _contexts list, because the
  // PreparedGraphicsObjects object will call clear_prepared() in response
  // to each release_index_buffer(), and we don't want to be modifying the
  // _contexts list while we're traversing it.
  Contexts temp = _contexts;
  int num_freed = (int)_contexts.size();

  Contexts::const_iterator ci;
  for (ci = temp.begin(); ci != temp.end(); ++ci) {
    PreparedGraphicsObjects *prepared_objects = (*ci).first;
    IndexBufferContext *ibc = (*ci).second;
    prepared_objects->release_index_buffer(ibc);
  }

  // Now that we've called release_index_buffer() on every known context,
  // the _contexts list should have completely emptied itself.
  nassertr(_contexts.empty(), num_freed);

  return num_freed;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::clear_prepared
//       Access: Private
//  Description: Removes the indicated PreparedGraphicsObjects table
//               from the data array's table, without actually
//               releasing the data array.  This is intended to be
//               called only from
//               PreparedGraphicsObjects::release_index_buffer(); it should
//               never be called by user code.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
clear_prepared(PreparedGraphicsObjects *prepared_objects) {
  Contexts::iterator ci;
  ci = _contexts.find(prepared_objects);
  if (ci != _contexts.end()) {
    _contexts.erase(ci);
  } else {
    // If this assertion fails, clear_prepared() was given a
    // prepared_objects which the data array didn't know about.
    nassertv(false);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::calc_tight_bounds
//       Access: Public, Virtual
//  Description: Expands min_point and max_point to include all of the
//               vertices in the Geom, if any.  found_any is set true
//               if any points are found.  It is the caller's
//               responsibility to initialize min_point, max_point,
//               and found_any before calling this function.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
calc_tight_bounds(LPoint3f &min_point, LPoint3f &max_point,
                  bool &found_any, 
                  const GeomVertexData *vertex_data,
                  bool got_mat, const LMatrix4f &mat,
                  Thread *current_thread) const {
  GeomVertexReader reader(vertex_data, InternalName::get_vertex(),
                          current_thread);
  if (!reader.has_column()) {
    // No vertex data.
    return;
  }

  CDReader cdata(_cycler, current_thread);

  if (cdata->_vertices == (GeomVertexArrayData *)NULL) {
    // Nonindexed case.
    if (got_mat) {
      for (int i = 0; i < cdata->_num_vertices; i++) {
        reader.set_row(cdata->_first_vertex + i);
        LPoint3f vertex = mat.xform_point(reader.get_data3f());
        
        if (found_any) {
          min_point.set(min(min_point[0], vertex[0]),
                        min(min_point[1], vertex[1]),
                        min(min_point[2], vertex[2]));
          max_point.set(max(max_point[0], vertex[0]),
                        max(max_point[1], vertex[1]),
                        max(max_point[2], vertex[2]));
        } else {
          min_point = vertex;
          max_point = vertex;
          found_any = true;
        }
      }
    } else {
      for (int i = 0; i < cdata->_num_vertices; i++) {
        reader.set_row(cdata->_first_vertex + i);
        const LVecBase3f &vertex = reader.get_data3f();
        
        if (found_any) {
          min_point.set(min(min_point[0], vertex[0]),
                        min(min_point[1], vertex[1]),
                        min(min_point[2], vertex[2]));
          max_point.set(max(max_point[0], vertex[0]),
                        max(max_point[1], vertex[1]),
                        max(max_point[2], vertex[2]));
        } else {
          min_point = vertex;
          max_point = vertex;
          found_any = true;
        }
      }
    }

  } else {
    // Indexed case.
    GeomVertexReader index(cdata->_vertices, 0, current_thread);

    if (got_mat) {
      while (!index.is_at_end()) {
        reader.set_row(index.get_data1i());
        LPoint3f vertex = mat.xform_point(reader.get_data3f());
        
        if (found_any) {
          min_point.set(min(min_point[0], vertex[0]),
                        min(min_point[1], vertex[1]),
                        min(min_point[2], vertex[2]));
          max_point.set(max(max_point[0], vertex[0]),
                        max(max_point[1], vertex[1]),
                        max(max_point[2], vertex[2]));
        } else {
          min_point = vertex;
          max_point = vertex;
          found_any = true;
        }
      }
    } else {
      while (!index.is_at_end()) {
        reader.set_row(index.get_data1i());
        const LVecBase3f &vertex = reader.get_data3f();
        
        if (found_any) {
          min_point.set(min(min_point[0], vertex[0]),
                        min(min_point[1], vertex[1]),
                        min(min_point[2], vertex[2]));
          max_point.set(max(max_point[0], vertex[0]),
                        max(max_point[1], vertex[1]),
                        max(max_point[2], vertex[2]));
        } else {
          min_point = vertex;
          max_point = vertex;
          found_any = true;
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::decompose_impl
//       Access: Protected, Virtual
//  Description: Decomposes a complex primitive type into a simpler
//               primitive type, for instance triangle strips to
//               triangles, and returns a pointer to the new primitive
//               definition.  If the decomposition cannot be
//               performed, this might return the original object.
//
//               This method is useful for application code that wants
//               to iterate through the set of triangles on the
//               primitive without having to write handlers for each
//               possible kind of primitive type.
////////////////////////////////////////////////////////////////////
CPT(GeomPrimitive) GeomPrimitive::
decompose_impl() const {
  return this;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::rotate_impl
//       Access: Protected, Virtual
//  Description: The virtual implementation of rotate().
////////////////////////////////////////////////////////////////////
CPT(GeomVertexArrayData) GeomPrimitive::
rotate_impl() const {
  // The default implementation doesn't even try to do anything.
  nassertr(false, NULL);
  return NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::requires_unused_vertices
//       Access: Protected, Virtual
//  Description: Should be redefined to return true in any primitive
//               that implements append_unused_vertices().
////////////////////////////////////////////////////////////////////
bool GeomPrimitive::
requires_unused_vertices() const {
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::append_unused_vertices
//       Access: Protected, Virtual
//  Description: Called when a new primitive is begun (other than the
//               first primitive), this should add some degenerate
//               vertices between primitives, if the primitive type
//               requires that.  The second parameter is the first
//               vertex that begins the new primitive.
//
//               This method is only called if
//               requires_unused_vertices(), above, returns true.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
append_unused_vertices(GeomVertexArrayData *, int) {
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::recompute_minmax
//       Access: Private
//  Description: Recomputes the _min_vertex and _max_vertex values if
//               necessary.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
recompute_minmax(GeomPrimitive::CData *cdata) {
  if (cdata->_vertices == (GeomVertexArrayData *)NULL) {
    // In the nonindexed case, we don't need to do much (the
    // minmax is trivial).
    cdata->_min_vertex = cdata->_first_vertex;
    cdata->_max_vertex = cdata->_first_vertex + cdata->_num_vertices - 1;
    cdata->_mins.clear();
    cdata->_maxs.clear();

  } else if (get_num_vertices() == 0) {
    // Or if we don't have any vertices, the minmax is also trivial.
    cdata->_min_vertex = 0;
    cdata->_max_vertex = 0;
    cdata->_mins.clear();
    cdata->_maxs.clear();

  } else if (get_num_vertices_per_primitive() == 0) {
    // This is a complex primitive type like a triangle strip; compute
    // the minmax of each primitive (as well as the overall minmax).
    GeomVertexReader index(cdata->_vertices, 0);

    cdata->_mins = make_index_data();
    cdata->_maxs = make_index_data();

    GeomVertexWriter mins(cdata->_mins, 0);
    GeomVertexWriter maxs(cdata->_maxs, 0);

    int pi = 0;
    int vi = 0;
    
    unsigned int vertex = index.get_data1i();
    cdata->_min_vertex = vertex;
    cdata->_max_vertex = vertex;
    unsigned int min_prim = vertex;
    unsigned int max_prim = vertex;
    
    ++vi;
    while (!index.is_at_end()) {
      unsigned int vertex = index.get_data1i();
      cdata->_min_vertex = min(cdata->_min_vertex, vertex);
      cdata->_max_vertex = max(cdata->_max_vertex, vertex);

      if (vi == cdata->_ends[pi]) {
        mins.add_data1i(min_prim);
        maxs.add_data1i(max_prim);
        min_prim = vertex;
        max_prim = vertex;
        ++pi;

      } else {
        min_prim = min(min_prim, vertex);
        max_prim = max(max_prim, vertex);
      }
      
      ++vi;
    }
    mins.add_data1i(min_prim);
    maxs.add_data1i(max_prim);
    nassertv(cdata->_mins->get_num_rows() == (int)cdata->_ends.size());

  } else {
    // This is a simple primitive type like a triangle; just compute
    // the overall minmax.
    GeomVertexReader index(cdata->_vertices, 0);

    cdata->_mins.clear();
    cdata->_maxs.clear();

    unsigned int vertex = index.get_data1i();
    cdata->_min_vertex = vertex;
    cdata->_max_vertex = vertex;

    while (!index.is_at_end()) {
      unsigned int vertex = index.get_data1i();
      cdata->_min_vertex = min(cdata->_min_vertex, vertex);
      cdata->_max_vertex = max(cdata->_max_vertex, vertex);
    }
  }

  cdata->_got_minmax = true;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::do_make_indexed
//       Access: Private
//  Description: The private implementation of make_indexed().
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
do_make_indexed(CData *cdata) {
  if (cdata->_vertices == (GeomVertexArrayData *)NULL) {
    cdata->_vertices = make_index_data();
    GeomVertexWriter index(cdata->_vertices, 0);
    for (int i = 0; i < cdata->_num_vertices; ++i) {
      index.add_data1i(i + cdata->_first_vertex);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::write_datagram
//       Access: Public, Virtual
//  Description: Writes the contents of this object to the datagram
//               for shipping out to a Bam file.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
write_datagram(BamWriter *manager, Datagram &dg) {
  TypedWritable::write_datagram(manager, dg);

  manager->write_cdata(dg, _cycler);
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::finalize
//       Access: Public, Virtual
//  Description: Called by the BamReader to perform any final actions
//               needed for setting up the object after all objects
//               have been read and all pointers have been completed.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
finalize(BamReader *manager) {
  const GeomVertexArrayData *vertices = get_vertices();
  if (vertices != (GeomVertexArrayData *)NULL) {
    set_usage_hint(vertices->get_usage_hint());
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::fillin
//       Access: Protected
//  Description: This internal function is called by make_from_bam to
//               read in all of the relevant data from the BamFile for
//               the new GeomPrimitive.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::
fillin(DatagramIterator &scan, BamReader *manager) {
  TypedWritable::fillin(scan, manager);

  manager->read_cdata(scan, _cycler);
  manager->register_finalize(this);
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::CData::make_copy
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
CycleData *GeomPrimitive::CData::
make_copy() const {
  return new CData(*this);
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::CData::write_datagram
//       Access: Public, Virtual
//  Description: Writes the contents of this object to the datagram
//               for shipping out to a Bam file.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::CData::
write_datagram(BamWriter *manager, Datagram &dg) const {
  dg.add_uint8(_shade_model);
  dg.add_uint32(_first_vertex);
  dg.add_uint32(_num_vertices);
  dg.add_uint8(_index_type);
  dg.add_uint8(_usage_hint);

  manager->write_pointer(dg, _vertices);
  WRITE_PTA(manager, dg, IPD_int::write_datagram, _ends);
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::CData::complete_pointers
//       Access: Public, Virtual
//  Description: Receives an array of pointers, one for each time
//               manager->read_pointer() was called in fillin().
//               Returns the number of pointers processed.
////////////////////////////////////////////////////////////////////
int GeomPrimitive::CData::
complete_pointers(TypedWritable **p_list, BamReader *manager) {
  int pi = CycleData::complete_pointers(p_list, manager);

  _vertices = DCAST(GeomVertexArrayData, p_list[pi++]);    

  return pi;
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitive::CData::fillin
//       Access: Public, Virtual
//  Description: This internal function is called by make_from_bam to
//               read in all of the relevant data from the BamFile for
//               the new GeomPrimitive.
////////////////////////////////////////////////////////////////////
void GeomPrimitive::CData::
fillin(DatagramIterator &scan, BamReader *manager) {
  _shade_model = (ShadeModel)scan.get_uint8();
  _first_vertex = scan.get_uint32();
  _num_vertices = scan.get_uint32();
  _index_type = (NumericType)scan.get_uint8();
  _usage_hint = (UsageHint)scan.get_uint8();

  manager->read_pointer(scan);
  READ_PTA(manager, scan, IPD_int::read_datagram, _ends);

  _modified = Geom::get_next_modified();
  _got_minmax = false;
}


////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitivePipelineReader::get_first_vertex
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
int GeomPrimitivePipelineReader::
get_first_vertex() const {
  if (_cdata->_vertices == (GeomVertexArrayData *)NULL) {
    return _cdata->_first_vertex;
  } else if (_vertices_reader->get_num_rows() == 0) {
    return 0;
  } else {
    GeomVertexReader index(_cdata->_vertices, 0);
    return index.get_data1i();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitivePipelineReader::get_vertex
//       Access: Public
//  Description: Returns the ith vertex index in the table.
////////////////////////////////////////////////////////////////////
int GeomPrimitivePipelineReader::
get_vertex(int i) const {
  if (_cdata->_vertices != (GeomVertexArrayData *)NULL) {
    // The indexed case.
    nassertr(i >= 0 && i < _vertices_reader->get_num_rows(), -1);

    GeomVertexReader index(_cdata->_vertices, 0);
    index.set_row(i);
    return index.get_data1i();

  } else {
    // The nonindexed case.
    return _cdata->_first_vertex + i;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitivePipelineReader::get_num_primitives
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
int GeomPrimitivePipelineReader::
get_num_primitives() const {
  int num_vertices_per_primitive = _object->get_num_vertices_per_primitive();

  if (num_vertices_per_primitive == 0) {
    // This is a complex primitive type like a triangle strip: each
    // primitive uses a different number of vertices.
    return _cdata->_ends.size();

  } else {
    // This is a simple primitive type like a triangle: each primitive
    // uses the same number of vertices.
    return (get_num_vertices() / num_vertices_per_primitive);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GeomPrimitivePipelineReader::check_valid
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
bool GeomPrimitivePipelineReader::
check_valid(const GeomVertexDataPipelineReader *data_reader) const {
  return get_num_vertices() == 0 ||
    get_max_vertex() < data_reader->get_num_rows();
}
