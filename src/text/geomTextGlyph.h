// Filename: geomTextGlyph.h
// Created by:  drose (11Feb02)
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

#ifndef GEOMTEXTGLYPH_H
#define GEOMTEXTGLYPH_H

#include "pandabase.h"

#ifdef HAVE_FREETYPE

#include "geomTristrip.h"
#include "dynamicTextGlyph.h"

////////////////////////////////////////////////////////////////////
//       Class : GeomTextGlyph
// Description : This is a specialization on GeomTristrip for
//               containing a triangle strip intended to represent a
//               DynamicTextGlyph.  Its sole purpose is to maintain
//               the geom count on the glyph, so we can determine the
//               actual usage count on a dynamic glyph (and thus know
//               when it is safe to recycle the glyph).
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA GeomTextGlyph : public GeomTristrip {
public:
  INLINE GeomTextGlyph(DynamicTextGlyph *glyph);
  INLINE GeomTextGlyph(const GeomTextGlyph &copy);
  void operator = (const GeomTextGlyph &copy);
  virtual ~GeomTextGlyph();

  virtual Geom *make_copy() const;

private:
  PT(DynamicTextGlyph) _glyph;

public:
  static void register_with_read_factory();
  static TypedWritable *make_GeomTextGlyph(const FactoryParams &params);

PUBLISHED:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
public:
  static void init_type() {
    GeomTristrip::init_type();
    register_type(_type_handle, "GeomTextGlyph",
                  GeomTristrip::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

#include "geomTextGlyph.I"

#endif  // HAVE_FREETYPE

#endif // GEOMTEXTGLYPH_H
