// Filename: eggTextureCollection.cxx
// Created by:  drose (15Feb00)
// 
////////////////////////////////////////////////////////////////////

#include "eggTextureCollection.h"
#include "eggGroupNode.h"
#include "eggPrimitive.h"
#include "eggTexture.h"

#include <nameUniquifier.h>

#include <algorithm>

////////////////////////////////////////////////////////////////////
//     Function: EggTextureCollection::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
EggTextureCollection::
EggTextureCollection() {
}

////////////////////////////////////////////////////////////////////
//     Function: EggTextureCollection::Copy Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
EggTextureCollection::
EggTextureCollection(const EggTextureCollection &copy) :
  _textures(copy._textures),
  _ordered_textures(copy._ordered_textures)
{
}

////////////////////////////////////////////////////////////////////
//     Function: EggTextureCollection::Copy Assignment Operator
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
EggTextureCollection &EggTextureCollection::
operator = (const EggTextureCollection &copy) {
  _textures = copy._textures;
  _ordered_textures = copy._ordered_textures;
  return *this;
}

////////////////////////////////////////////////////////////////////
//     Function: EggTextureCollection::clear
//       Access: Public
//  Description: Removes all textures from the collection.
////////////////////////////////////////////////////////////////////
void EggTextureCollection::
clear() {
  _textures.clear();
  _ordered_textures.clear();
}

////////////////////////////////////////////////////////////////////
//     Function: EggTextureCollection::extract_textures
//       Access: Public
//  Description: Walks the egg hierarchy beginning at the indicated
//               node, and removes any EggTextures encountered in the
//               hierarchy, adding them to the collection.  Returns
//               the number of EggTextures encountered.
////////////////////////////////////////////////////////////////////
int EggTextureCollection::
extract_textures(EggGroupNode *node) {
  // Since this traversal is destructive, we'll handle it within the
  // EggGroupNode code.
  return node->find_textures(this);
}

////////////////////////////////////////////////////////////////////
//     Function: EggTextureCollection::insert_textures
//       Access: Public
//  Description: Adds a series of EggTexture nodes to the beginning of
//               the indicated node to reflect each of the textures in
//               the collection.  Returns the number of texture nodes
//               added.
////////////////////////////////////////////////////////////////////
int EggTextureCollection::
insert_textures(EggGroupNode *node) {
  return insert_textures(node, node->begin());
}

////////////////////////////////////////////////////////////////////
//     Function: EggTextureCollection::insert_textures
//       Access: Public
//  Description: Adds a series of EggTexture nodes to the beginning of
//               the indicated node to reflect each of the textures in
//               the collection.  Returns the number of texture nodes
//               added.
////////////////////////////////////////////////////////////////////
int EggTextureCollection::
insert_textures(EggGroupNode *node, EggGroupNode::iterator position) {
  OrderedTextures::iterator oti;
  for (oti = _ordered_textures.begin(); 
       oti != _ordered_textures.end(); 
       ++oti) {
    node->insert(position, (*oti).p());
  }

  return size();
}

////////////////////////////////////////////////////////////////////
//     Function: EggTextureCollection::find_used_textures
//       Access: Public
//  Description: Walks the egg hierarchy beginning at the indicated
//               node, looking for textures that are referenced by
//               primitives but are not already members of the
//               collection, adding them to the collection.
//
//               If this is called following extract_textures(), it
//               can be used to pick up any additional texture
//               references that appeared in the egg hierarchy (but
//               whose EggTexture node was not actually part of the
//               hierarchy).
//
//               If this is called in lieu of extract_textures(), it
//               will fill up the collection with all of the
//               referenced textures (and only the referenced
//               textures), without destructively removing the
//               EggTextures from the hierarchy.
//
//               This also has the side effect of incrementing the
//               internal usage count for a texture in the collection
//               each time a texture reference is encountered.  This
//               side effect is taken advantage of by
//               remove_unused_textures().
////////////////////////////////////////////////////////////////////
int EggTextureCollection::
find_used_textures(EggGroupNode *node) {
  int num_found = 0;

  EggGroupNode::iterator ci;
  for (ci = node->begin();
       ci != node->end();
       ++ci) {
    EggNode *child = *ci;
    if (child->is_of_type(EggPrimitive::get_class_type())) {
      EggPrimitive *primitive = DCAST(EggPrimitive, child);
      if (primitive->has_texture()) {
	EggTexture *tex = primitive->get_texture();
	Textures::iterator ti = _textures.find(tex);
	if (ti == _textures.end()) {
	  // Here's a new texture!
	  num_found++;
	  _textures.insert(Textures::value_type(tex, 1));
	  _ordered_textures.push_back(tex);
	} else {
	  // Here's a texture we'd already known about.  Increment its
	  // usage count.
	  (*ti).second++;
	}
      }

    } else if (child->is_of_type(EggGroupNode::get_class_type())) {
      EggGroupNode *group_child = DCAST(EggGroupNode, child);
      num_found += find_used_textures(group_child);
    }
  }

  return num_found;
}

////////////////////////////////////////////////////////////////////
//     Function: EggTextureCollection::remove_unused_textures
//       Access: Public
//  Description: Removes any textures from the collection that aren't
//               referenced by any primitives in the indicated egg
//               hierarchy.  This also, incidentally, adds textures to
//               the collection that had been referenced by primitives
//               but had not previously appeared in the collection.
////////////////////////////////////////////////////////////////////
void EggTextureCollection::
remove_unused_textures(EggGroupNode *node) {
  // We'll do this the easy way: First, we'll remove *all* the
  // textures from the collection, and then we'll add back only those
  // that appear in the hierarchy.
  clear();
  find_used_textures(node);
}

////////////////////////////////////////////////////////////////////
//     Function: EggTextureCollection::collapse_equivalent_textures
//       Access: Public
//  Description: Walks through the collection and collapses together
//               any separate textures that are equivalent according
//               to the indicated equivalence factor, eq (see
//               EggTexture::is_equivalent_to()).  The return value is
//               the number of textures removed.
//
//               This flavor of collapse_equivalent_textures()
//               automatically adjusts all the primitives in the egg
//               hierarchy to refer to the new texture pointers.
////////////////////////////////////////////////////////////////////
int EggTextureCollection::
collapse_equivalent_textures(int eq, EggGroupNode *node) {
  TextureReplacement removed;
  int num_collapsed = collapse_equivalent_textures(eq, removed);

  // And now walk the egg hierarchy and replace any references to a
  // removed texture with its replacement.
  replace_textures(node, removed);

  return num_collapsed;
}

////////////////////////////////////////////////////////////////////
//     Function: EggTextureCollection::collapse_equivalent_textures
//       Access: Public
//  Description: Walks through the collection and collapses together
//               any separate textures that are equivalent according
//               to the indicated equivalence factor, eq (see
//               EggTexture::is_equivalent_to()).  The return value is
//               the number of textures removed.
//
//               This flavor of collapse_equivalent_textures() does
//               not adjust any primitives in the egg hierarchy;
//               instead, it fills up the 'removed' map with an entry
//               for each removed texture, mapping it back to the
//               equivalent retained texture.  It's up to the user to
//               then call replace_textures() with this map, if
//               desired, to apply these changes to the egg hierarchy.
////////////////////////////////////////////////////////////////////
int EggTextureCollection::
collapse_equivalent_textures(int eq, EggTextureCollection::TextureReplacement &removed) {
  int num_collapsed = 0;

  typedef set<PT(EggTexture), UniqueEggTextures> Collapser;
  UniqueEggTextures uet(eq);
  Collapser collapser(uet);

  // First, put all of the textures into the Collapser structure, to
  // find out the unique textures.
  OrderedTextures::const_iterator oti;
  for (oti = _ordered_textures.begin(); 
       oti != _ordered_textures.end(); 
       ++oti) {
    EggTexture *tex = (*oti);

    pair<Collapser::const_iterator, bool> result = collapser.insert(tex);
    if (!result.second) {
      // This texture is non-unique; another one was already there.
      EggTexture *first = *(result.first);
      removed.insert(TextureReplacement::value_type(tex, first));
      num_collapsed++;
    }
  }

  // Now record all of the unique textures only.
  clear();
  Collapser::const_iterator ci;
  for (ci = collapser.begin(); ci != collapser.end(); ++ci) {
    add_texture(*ci);
  }

  return num_collapsed;
}

////////////////////////////////////////////////////////////////////
//     Function: EggTextureCollection::replace_textures
//       Access: Public, Static
//  Description: Walks the egg hierarchy, changing out any reference
//               to a texture appearing on the left side of the map
//               with its corresponding texture on the right side.
//               This is most often done following a call to
//               collapse_equivalent_textures().  It does not directly
//               affect the Collection.
////////////////////////////////////////////////////////////////////
void EggTextureCollection::
replace_textures(EggGroupNode *node,
		 const EggTextureCollection::TextureReplacement &replace) {
  EggGroupNode::iterator ci;
  for (ci = node->begin();
       ci != node->end();
       ++ci) {
    EggNode *child = *ci;
    if (child->is_of_type(EggPrimitive::get_class_type())) {
      EggPrimitive *primitive = DCAST(EggPrimitive, child);
      if (primitive->has_texture()) {
	PT(EggTexture) tex = primitive->get_texture();
	TextureReplacement::const_iterator ri;
	ri = replace.find(tex);
	if (ri != replace.end()) {
	  // Here's a texture we want to replace.
	  primitive->set_texture((*ri).second);
	}
      }

    } else if (child->is_of_type(EggGroupNode::get_class_type())) {
      EggGroupNode *group_child = DCAST(EggGroupNode, child);
      replace_textures(group_child, replace);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: EggTextureCollection::uniquify_trefs
//       Access: Public
//  Description: Guarantees that each texture in the collection has a
//               unique TRef name.  This is essential before writing
//               an egg file.
////////////////////////////////////////////////////////////////////
void EggTextureCollection::
uniquify_trefs() {
  NameUniquifier nu(".tref", "tref");

  OrderedTextures::const_iterator oti;
  for (oti = _ordered_textures.begin(); 
       oti != _ordered_textures.end(); 
       ++oti) {
    EggTexture *tex = (*oti);

    tex->set_name(nu.add_name(tex->get_name()));
  }
}
 
////////////////////////////////////////////////////////////////////
//     Function: EggTextureCollection::sort_by_tref
//       Access: Public
//  Description: Sorts all the textures into alphabetical order by
//               TRef name.  Subsequent operations using begin()/end()
//               will traverse in this sorted order.
////////////////////////////////////////////////////////////////////
void EggTextureCollection::
sort_by_tref() {
  sort(_ordered_textures.begin(), _ordered_textures.end(),
       NamableOrderByName());
}

////////////////////////////////////////////////////////////////////
//     Function: EggTextureCollection::add_texture
//       Access: Public
//  Description: Explicitly adds a new texture to the collection.
//               Returns true if the texture was added, false if it
//               was already there or if there was some error.
////////////////////////////////////////////////////////////////////
bool EggTextureCollection::
add_texture(PT(EggTexture) texture) {
  nassertr(_textures.size() == _ordered_textures.size(), false);

  Textures::const_iterator ti;
  ti = _textures.find(texture);
  if (ti != _textures.end()) {
    // This texture is already a member of the collection.
    return false;
  }

  _textures.insert(Textures::value_type(texture, 0));
  _ordered_textures.push_back(texture);

  nassertr(_textures.size() == _ordered_textures.size(), false);
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: EggTextureCollection::remove_texture
//       Access: Public
//  Description: Explicitly removes a texture from the collection.
//               Returns true if the texture was removed, false if it
//               wasn't there or if there was some error.
////////////////////////////////////////////////////////////////////
bool EggTextureCollection::
remove_texture(PT(EggTexture) texture) {
  nassertr(_textures.size() == _ordered_textures.size(), false);

  Textures::iterator ti;
  ti = _textures.find(texture);
  if (ti == _textures.end()) {
    // This texture is not a member of the collection.
    return false;
  }

  _textures.erase(ti);

  OrderedTextures::iterator oti;
  oti = find(_ordered_textures.begin(), _ordered_textures.end(), texture);
  nassertr(oti != _ordered_textures.end(), false);

  _ordered_textures.erase(oti);

  nassertr(_textures.size() == _ordered_textures.size(), false);
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: EggTextureCollection::create_unique_texture
//       Access: Public
//  Description: Creates a new texture if there is not already one
//               equivalent (according to eq, see
//               EggTexture::is_equivalent_to()) to the indicated
//               texture, or returns the existing one if there is.
////////////////////////////////////////////////////////////////////
EggTexture *EggTextureCollection::
create_unique_texture(const EggTexture &copy, int eq) {
  // This requires a complete linear traversal, not terribly
  // efficient.
  OrderedTextures::const_iterator oti;
  for (oti = _ordered_textures.begin(); 
       oti != _ordered_textures.end(); 
       ++oti) {
    EggTexture *tex = (*oti);
    if (copy.is_equivalent_to(*tex, eq)) {
      return tex;
    }
  }

  PT(EggTexture) new_texture = new EggTexture(copy);
  add_texture(new_texture);
  return new_texture;
}

////////////////////////////////////////////////////////////////////
//     Function: EggTextureCollection::find_tref
//       Access: Public
//  Description: Returns the texture with the indicated TRef name, or
//               NULL if no texture matches.
////////////////////////////////////////////////////////////////////
EggTexture *EggTextureCollection::
find_tref(const string &tref_name) const {
  // This requires a complete linear traversal, not terribly
  // efficient.
  OrderedTextures::const_iterator oti;
  for (oti = _ordered_textures.begin(); 
       oti != _ordered_textures.end(); 
       ++oti) {
    EggTexture *tex = (*oti);
    if (tex->get_name() == tref_name) {
      return tex;
    }
  }

  return (EggTexture *)NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: EggTextureCollection::find_filename
//       Access: Public
//  Description: Returns the texture with the indicated filename, or
//               NULL if no texture matches.
////////////////////////////////////////////////////////////////////
EggTexture *EggTextureCollection::
find_filename(const Filename &filename) const {
  // This requires a complete linear traversal, not terribly
  // efficient.
  OrderedTextures::const_iterator oti;
  for (oti = _ordered_textures.begin(); 
       oti != _ordered_textures.end(); 
       ++oti) {
    EggTexture *tex = (*oti);
    if (tex->get_filename() == filename) {
      return tex;
    }
  }

  return (EggTexture *)NULL;
}
