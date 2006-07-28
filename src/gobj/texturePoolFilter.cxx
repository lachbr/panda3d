// Filename: texturePoolFilter.cxx
// Created by:  drose (27Jul06)
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

#include "texturePoolFilter.h"

TypeHandle TexturePoolFilter::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: TexturePoolFilter::Destructor
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
TexturePoolFilter::
~TexturePoolFilter() {
}

////////////////////////////////////////////////////////////////////
//     Function: TexturePoolFilter::pre_load
//       Access: Public, Virtual
//  Description: This method is called before each texture is loaded
//               from disk, via the TexturePool, for the first time.
//               If this method returns NULL, then a new Texture will
//               be allocated and loaded from disk normally by the
//               TexturePool; otherwise, if it returns non-NULL, then
//               that returned pointer will be used as the Texture for
//               this filename.
////////////////////////////////////////////////////////////////////
PT(Texture) TexturePoolFilter::
pre_load(const Filename &, const Filename &, int, int, bool) {
  return NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: TexturePoolFilter::post_load
//       Access: Public, Virtual
//  Description: This method is called after each texture has been
//               loaded from disk, via the TexturePool, for the first
//               time.  By the time this method is called, the Texture
//               has already been fully read from disk.  This method
//               should return the Texture pointer that the
//               TexturePool should actually return (usually it is the
//               same as the pointer supplied).
////////////////////////////////////////////////////////////////////
PT(Texture) TexturePoolFilter::
post_load(Texture *tex) {
  return tex;
}

////////////////////////////////////////////////////////////////////
//     Function: TexturePoolFilter::output
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void TexturePoolFilter::
output(ostream &out) const {
  out << get_type();
}
