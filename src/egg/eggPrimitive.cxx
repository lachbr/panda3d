// Filename: eggPrimitive.cxx
// Created by:  drose (16Jan99)
// 
////////////////////////////////////////////////////////////////////

#include "eggPrimitive.h"
#include "eggVertexPool.h"
#include "eggMiscFuncs.h"
#include "eggTextureCollection.h"

#include <indent.h>
#include <vector_int.h>

TypeHandle EggPrimitive::_type_handle;


////////////////////////////////////////////////////////////////////
//     Function: EggPrimitive::determine_alpha_mode
//       Access: Public, Virtual
//  Description: Walks back up the hierarchy, looking for an EggPrimitive
//               or EggPrimitive or some such object at this level or
//               above this primitive that has an alpha_mode other than
//               AM_unspecified.  Returns a valid EggRenderMode pointer
//               if one is found, or NULL otherwise.
////////////////////////////////////////////////////////////////////
EggRenderMode *EggPrimitive::
determine_alpha_mode() {
  if (get_alpha_mode() != AM_unspecified) {
    return this;
  }
  if (has_texture() && get_texture()->get_alpha_mode() != AM_unspecified) {
    return get_texture();
  }
  return EggNode::determine_alpha_mode();
}

////////////////////////////////////////////////////////////////////
//     Function: EggPrimitive::determine_depth_write_mode
//       Access: Public, Virtual
//  Description: Walks back up the hierarchy, looking for an EggGroup
//               or EggPrimitive or some such object at this level or
//               above this node that has a depth_write_mode other than
//               DWM_unspecified.  Returns a valid EggRenderMode pointer
//               if one is found, or NULL otherwise.
////////////////////////////////////////////////////////////////////
EggRenderMode *EggPrimitive::
determine_depth_write_mode() {
  if (get_depth_write_mode() != DWM_unspecified) {
    return this;
  }
  if (has_texture() && 
      get_texture()->get_depth_write_mode() != DWM_unspecified) {
    return get_texture();
  }
  return EggNode::determine_depth_write_mode();
}

////////////////////////////////////////////////////////////////////
//     Function: EggPrimitive::determine_depth_test_mode
//       Access: Public, Virtual
//  Description: Walks back up the hierarchy, looking for an EggGroup
//               or EggPrimitive or some such object at this level or
//               above this node that has a depth_test_mode other than
//               DTM_unspecified.  Returns a valid EggRenderMode pointer
//               if one is found, or NULL otherwise.
////////////////////////////////////////////////////////////////////
EggRenderMode *EggPrimitive::
determine_depth_test_mode() {
  if (get_depth_test_mode() != DTM_unspecified) {
    return this;
  }
  if (has_texture() && 
      get_texture()->get_depth_test_mode() != DTM_unspecified) {
    return get_texture();
  }
  return EggNode::determine_depth_test_mode();
}

////////////////////////////////////////////////////////////////////
//     Function: EggPrimitive::determine_draw_order
//       Access: Public, Virtual
//  Description: Walks back up the hierarchy, looking for an EggPrimitive
//               or EggPrimitive or some such object at this level or
//               above this primitive that has a draw_order specified.
//               Returns a valid EggRenderMode pointer if one is found,
//               or NULL otherwise.
////////////////////////////////////////////////////////////////////
EggRenderMode *EggPrimitive::
determine_draw_order() {
  if (has_draw_order()) {
    return this;
  }
  if (has_texture() && get_texture()->has_draw_order()) {
    return get_texture();
  }
  return EggNode::determine_draw_order();
}

////////////////////////////////////////////////////////////////////
//     Function: EggPrimitive::determine_bin
//       Access: Public, Virtual
//  Description: Walks back up the hierarchy, looking for an EggPrimitive
//               or EggPrimitive or some such object at this level or
//               above this primitive that has a bin specified.  Returns a
//               valid EggRenderMode pointer if one is found, or NULL
//               otherwise.
////////////////////////////////////////////////////////////////////
EggRenderMode *EggPrimitive::
determine_bin() {
  if (has_bin()) {
    return this;
  }
  if (has_texture() && get_texture()->has_bin()) {
    return get_texture();
  }
  return EggNode::determine_bin();
}

////////////////////////////////////////////////////////////////////
//     Function: EggPrimitive::reverse_vertex_ordering
//       Access: Public, Virtual
//  Description: Reverses the ordering of the vertices in this
//               primitive, if appropriate, in order to change the
//               direction the polygon appears to be facing.  Does not
//               adjust the surface normal, if any.
////////////////////////////////////////////////////////////////////
void EggPrimitive::
reverse_vertex_ordering() {
  // This really only makes sense for polygons.  Lights don't care
  // about vertex ordering, and NURBS surfaces have to do a bit more
  // work in addition to this.
  reverse(_vertices.begin(), _vertices.end());
}

////////////////////////////////////////////////////////////////////
//     Function: EggPrimitive::cleanup
//       Access: Public, Virtual
//  Description: Cleans up modeling errors in whatever context this
//               makes sense.  For instance, for a polygon, this calls
//               remove_doubled_verts(true).  For a point, it calls
//               remove_nonunique_verts().  Returns true if the
//               primitive is valid, or false if it is degenerate.
////////////////////////////////////////////////////////////////////
bool EggPrimitive::
cleanup() {
  return !empty();
}

////////////////////////////////////////////////////////////////////
//     Function: EggPrimitive::remove_doubled_verts
//       Access: Public
//  Description: Certain kinds of primitives, particularly polygons,
//               don't like to have the same vertex repeated
//               consecutively.  Unfortunately, some modeling programs
//               (like MultiGen) make this an easy mistake to make.
//
//               It's handy to have a function to remove these
//               redundant vertices.  If closed is true, it also
//               checks that the first and last vertices are not the
//               same.
//
//               This function identifies repeated vertices by pointer
//               only; it does not remove consecutive equivalent but
//               different vertices.
////////////////////////////////////////////////////////////////////
void EggPrimitive::
remove_doubled_verts(bool closed) {
  if (!_vertices.empty()) {
    Vertices new_vertices;
    Vertices::iterator vi, vlast;
    vi = _vertices.begin();
    new_vertices.push_back(*vi);

    vlast = vi;
    ++vi;
    while (vi != _vertices.end()) {
      if ((*vi) != (*vlast)) {
	new_vertices.push_back(*vi);
      } else {
	prepare_remove_vertex(*vi);
      }
      vlast = vi;
      ++vi;
    }
    _vertices.swap(new_vertices);
  }

  if (closed) {
    // Then, if this is a polygon (which will be closed anyway),
    // remove the vertex from the end if it's a repeat of the
    // beginning.
    while (_vertices.size() > 1 && _vertices.back() == _vertices.front()) {
      prepare_remove_vertex(_vertices.back());
      _vertices.pop_back();
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: EggPrimitive::remove_nonunique_verts
//       Access: Public
//  Description: Removes any multiple appearances of the same vertex
//               from the primitive.  This primarily makes sense for a
//               point primitive, which is really a collection of
//               points and which doesn't make sense to include the
//               same point twice, in any order.
////////////////////////////////////////////////////////////////////
void EggPrimitive::
remove_nonunique_verts() {
  Vertices::iterator vi, vj;
  Vertices new_vertices;

  for (vi = _vertices.begin(); vi != _vertices.end(); ++vi) {
    bool okflag = true;
    for (vj = _vertices.begin(); vj != vi && okflag; ++vj) {
      okflag = ((*vi) != (*vj));
    }
    if (okflag) {
      new_vertices.push_back(*vi);
    } else {
      prepare_remove_vertex(*vi);
    }
  }

  _vertices.swap(new_vertices);
}


////////////////////////////////////////////////////////////////////
//     Function: EggPrimitive::erase
//       Access: Public
//  Description: Part of the implementaion of the EggPrimitive as an
//               STL container.  Most of the rest of these functions
//               are inline and declared in EggPrimitive.I.
////////////////////////////////////////////////////////////////////
EggPrimitive::iterator EggPrimitive::
erase(iterator first, iterator last) {
  iterator i;
  for (i = first; i != last; ++i) {
    prepare_remove_vertex(*i);
  }
  iterator result = _vertices.erase((Vertices::iterator &)first, 
				    (Vertices::iterator &)last);
  test_vref_integrity();
  return result;
}


////////////////////////////////////////////////////////////////////
//     Function: EggPrimitive::add_vertex
//       Access: Public
//  Description: Adds the indicated vertex to the end of the
//               primitive's list of vertices, and returns it.
////////////////////////////////////////////////////////////////////
EggVertex *EggPrimitive::
add_vertex(EggVertex *vertex) {
  prepare_add_vertex(vertex);
  _vertices.push_back(vertex);

  vertex->test_pref_integrity();
  test_vref_integrity();

  return vertex;
}

////////////////////////////////////////////////////////////////////
//     Function: EggPrimitive::remove_vertex
//       Access: Public
//  Description: Removes the indicated vertex vertex from the
//               primitive and returns it.  If the vertex was not
//               already in the primitive, does nothing and returns
//               NULL.
////////////////////////////////////////////////////////////////////
EggVertex *EggPrimitive::
remove_vertex(EggVertex *vertex) {
  PT_EggVertex vpt = vertex;
  iterator i = find(begin(), end(), vpt);
  if (i == end()) {
    return PT_EggVertex();
  } else {
    // erase() calls prepare_remove_vertex().
    erase(i);

    vertex->test_pref_integrity();
    test_vref_integrity();

    return vertex;
  }
}


////////////////////////////////////////////////////////////////////
//     Function: EggPrimitive::copy_vertices
//       Access: Public
//  Description: Replaces the current primitive's list of vertices
//               with a copy of the list of vertices on the other
//               primitive.
////////////////////////////////////////////////////////////////////
void EggPrimitive::
copy_vertices(const EggPrimitive &other) {
  clear();
  _vertices.reserve(other.size());

  iterator vi;
  for (vi = other.begin(); vi != other.end(); ++vi) {
    add_vertex(*vi);
  }

  test_vref_integrity();
  other.test_vref_integrity();
}

#ifndef NDEBUG

////////////////////////////////////////////////////////////////////
//     Function: EggPrimitive::test_vref_integrity
//       Access: Public
//  Description: Verifies that each vertex in the primitive exists and
//               that it knows it is referenced by the primitive.
////////////////////////////////////////////////////////////////////
void EggPrimitive::
test_vref_integrity() const {
  test_ref_count_integrity();

  // First, we need to know how many times each vertex appears.
  // Usually, this will be only one, but it's possible for a vertex to
  // appear more than once.
  typedef map<const EggVertex *, int> VertexCount;
  VertexCount _count;

  // Now count up the vertices.
  iterator vi;
  for (vi = begin(); vi != end(); ++vi) {
    const EggVertex *vert = *vi;
    vert->test_ref_count_integrity();

    VertexCount::iterator vci = _count.find(vert);
    if (vci == _count.end()) {
      _count[vert] = 1;
    } else {
      (*vci).second++;
    }
  }

  // Ok, now walk through the vertices found and make sure the vertex
  // has the proper number of entries of this primitive in its pref.
  VertexCount::iterator vci;
  for (vci = _count.begin(); vci != _count.end(); ++vci) {
    const EggVertex *vert = (*vci).first;

    int count = (*vci).second;
    int vert_count = vert->has_pref(this);

    nassertv(count == vert_count);
  }
}

#endif  // NDEBUG

////////////////////////////////////////////////////////////////////
//     Function: EggPrimitive::prepare_add_vertex
//       Access: Private
//  Description: Marks the vertex as belonging to the primitive.  This
//               is an internal function called by the STL-like
//               functions push_back() and insert(), in preparation
//               for actually adding the vertex.
////////////////////////////////////////////////////////////////////
void EggPrimitive::
prepare_add_vertex(EggVertex *vertex) {
  // We can't test integrity within this function, because it might be
  // called when the primitive is in an incomplete state.

  // The vertex must have the same vertex pool as the vertices already
  // added.
  nassertv(empty() || vertex->get_pool() == get_pool());

  // Since a given vertex might appear more than once in a particular
  // primitive, we can't conclude anything about data integrity by
  // inspecting the return value of insert().  (In fact, the vertex's
  // pref is a multiset, so the insert() will always succeed.)

  vertex->_pref.insert(this);
}


////////////////////////////////////////////////////////////////////
//     Function: EggPrimitive::prepare_remove_vertex
//       Access: Private
//  Description: Marks the vertex as removed from the primitive.  This
//               is an internal function called by the STL-like
//               functions pop_back() and erase(), in preparation for
//               actually doing the removal.
//
//               It is an error to attempt to remove a vertex that is
//               not already a vertex of this primitive.
////////////////////////////////////////////////////////////////////
void EggPrimitive::
prepare_remove_vertex(EggVertex *vertex) {
  // We can't test integrity within this function, because it might be
  // called when the primitive is in an incomplete state.

  // Now we must remove the primitive from the vertex's pref.  We
  // can't just use the simple erase() function, since that will
  // remove all instances of this primitive from the pref; instead, we
  // must find one instance and remove that.
  
  EggVertex::PrimitiveRef::iterator pri = vertex->_pref.find(this);

  // We should have found the primitive in the vertex's pref.  If we
  // did not, something's out of sync internally.
  nassertv(pri != vertex->_pref.end());

  vertex->_pref.erase(pri);
}

////////////////////////////////////////////////////////////////////
//     Function: EggPrimitive::write_body
//       Access: Protected
//  Description: Writes the attributes and the vertices referenced by
//               the primitive to the indicated output stream in Egg
//               format.
////////////////////////////////////////////////////////////////////
void EggPrimitive::
write_body(ostream &out, int indent_level) const {
  test_vref_integrity();

  EggAttributes::write(out, indent_level);
  EggRenderMode::write(out, indent_level);

  if (has_texture()) {
    EggTexture *texture = get_texture();
    
    // Make sure the texture is named.
    nassertv(texture->has_name());

    indent(out, indent_level) << "<TRef> { ";
    enquote_string(out, texture->get_name())
      << " }\n";
  }

  if (has_material()) {
    EggMaterial *material = get_material();
    
    // Make sure the material is named.
    nassertv(material->has_name());

    indent(out, indent_level) << "<MRef> { ";
    enquote_string(out, material->get_name())
      << " }\n";
  }

  if (get_bface_flag()) {
    indent(out, indent_level) << "<BFace> { 1 }\n";
  }

  if (!empty()) {
    EggVertexPool *pool = get_pool();

    // Make sure the vertices belong to some vertex pool.
    nassertv(pool != NULL);

    // Make sure the vertex pool is named.
    nassertv(pool->has_name());

    if ((int)size() < 10) {
      // A simple primitive gets all its vertex indices written on one
      // line.
      indent(out, indent_level) << "<VertexRef> {";
      const_iterator i;
      for (i = begin(); i != end(); ++i) {
	EggVertex *vert = *i;
	vert->test_pref_integrity();
	
	// Make sure each vertex belongs to the same pool.
	nassertv(vert->get_pool() == pool);
	
	out << " " << vert->get_index();
      }
      out << " <Ref> { ";
      enquote_string(out, pool->get_name()) << " } }\n";

    } else {

      // A larger primitive gets its vertex indices written as
      // multiple lines.
      vector_int indices;
      const_iterator i;
      for (i = begin(); i != end(); ++i) {
	EggVertex *vert = *i;
	vert->test_pref_integrity();
	
	// Make sure each vertex belongs to the same pool.
	nassertv(vert->get_pool() == pool);
	
	indices.push_back(vert->get_index());
      }
      
      indent(out, indent_level) << "<VertexRef> {\n";
      write_long_list(out, indent_level+2, indices.begin(), indices.end(),
		"", "", 72);
      indent(out, indent_level+2) << "<Ref> { ";
      enquote_string(out, pool->get_name()) << " }\n";
      indent(out, indent_level) << "}\n";
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: EggPrimitive::r_transform
//       Access: Protected, Virtual
//  Description: This is called from within the egg code by
//               transform().  It applies a transformation matrix
//               to the current node in some sensible way, then
//               continues down the tree.
//
//               The first matrix is the transformation to apply; the
//               second is its inverse.  The third parameter is the
//               coordinate system we are changing to, or CS_default
//               if we are not changing coordinate systems.
////////////////////////////////////////////////////////////////////
void EggPrimitive::
r_transform(const LMatrix4d &mat, const LMatrix4d &, CoordinateSystem) {
  EggAttributes::transform(mat);
}

////////////////////////////////////////////////////////////////////
//     Function: EggPrimitive::r_flatten_transforms
//       Access: Protected, Virtual
//  Description: The recursive implementation of flatten_transforms().
////////////////////////////////////////////////////////////////////
void EggPrimitive::
r_flatten_transforms() {
  if (is_local_coord()) {
    LMatrix4d mat = get_vertex_frame();
    EggAttributes::transform(mat);

    // Transform each vertex by duplicating it in the vertex pool.
    size_t num_vertices = size();
    for (size_t i = 0; i < num_vertices; i++) {
      EggVertex *vertex = get_vertex(i);
      EggVertexPool *pool = vertex->get_pool();
      
      EggVertex new_vertex(*vertex);
      new_vertex.transform(mat);
      EggVertex *unique = pool->create_unique_vertex(new_vertex);
      unique->copy_grefs_from(*vertex);
	
      set_vertex(i, unique);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: EggPrimitive::r_apply_texmats
//       Access: Protected, Virtual
//  Description: The recursive implementation of apply_texmats().
////////////////////////////////////////////////////////////////////
void EggPrimitive::
r_apply_texmats(EggTextureCollection &textures) {
  if (has_texture()) {
    EggTexture *texture = get_texture();
    if (texture->has_transform()) {
      if (texture->transform_is_identity()) {
	// Now, what's the point of a texture with an identity
	// transform?
	texture->clear_transform();
	return;
      }

      // We've got a texture with a matrix applied.  Save the matrix,
      // and get a new texture without the matrix.
      LMatrix3d mat = texture->get_transform();
      EggTexture new_texture(*texture);
      new_texture.clear_transform();
      EggTexture *unique = textures.create_unique_texture(new_texture, ~0);

      set_texture(unique);

      // Now apply the matrix to the vertex UV's.  Create new vertices
      // as necessary.
      size_t num_vertices = size();
      for (size_t i = 0; i < num_vertices; i++) {
	EggVertex *vertex = get_vertex(i);

	if (vertex->has_uv()) {
	  EggVertexPool *pool = vertex->get_pool();

	  EggVertex new_vertex(*vertex);
	  new_vertex.set_uv(vertex->get_uv() * mat);
	  EggVertex *unique = pool->create_unique_vertex(new_vertex);
	  unique->copy_grefs_from(*vertex);
	
	  set_vertex(i, unique);
	}
      }
    }
  }
}
