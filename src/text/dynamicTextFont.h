// Filename: dynamicTextFont.h
// Created by:  drose (08Feb02)
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

#ifndef DYNAMICTEXTFONT_H
#define DYNAMICTEXTFONT_H

#include "pandabase.h"

#ifdef HAVE_FREETYPE

#include "config_text.h"
#include "textFont.h"
#include "dynamicTextGlyph.h"
#include "dynamicTextPage.h"
#include "filename.h"
#include "pvector.h"
#include "pmap.h"

#include <ft2build.h>
#include FT_FREETYPE_H

////////////////////////////////////////////////////////////////////
//       Class : DynamicTextFont
// Description : A DynamicTextFont is a special TextFont object that
//               rasterizes its glyphs from a standard font file
//               (e.g. a TTF file) on the fly.  It requires the
//               FreeType 2.0 library (or any higher,
//               backward-compatible version).
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA DynamicTextFont : public TextFont {
PUBLISHED:
  DynamicTextFont(const Filename &font_filename, int face_index = 0);

  INLINE bool set_point_size(float point_size);
  INLINE float get_point_size() const;

  INLINE bool set_pixels_per_unit(float pixels_per_unit);
  INLINE float get_pixels_per_unit() const;

  INLINE void set_small_caps(bool small_caps);
  INLINE bool get_small_caps() const;
  INLINE void set_small_caps_scale(float small_caps_scale);
  INLINE float get_small_caps_scale() const;

  INLINE void set_texture_margin(int texture_margin);
  INLINE int get_texture_margin() const;
  INLINE void set_poly_margin(float poly_margin);
  INLINE float get_poly_margin() const;

  INLINE void set_page_size(int x_size, int y_size);
  INLINE int get_page_x_size() const;
  INLINE int get_page_y_size() const;

  INLINE void set_minfilter(Texture::FilterType filter);
  INLINE Texture::FilterType get_minfilter() const;
  INLINE void set_magfilter(Texture::FilterType filter);
  INLINE Texture::FilterType get_magfilter() const;
  INLINE void set_anisotropic_degree(int anisotropic_degree);
  INLINE int get_anisotropic_degree() const;

  INLINE static void set_update_cleared_glyphs(bool update_cleared_glyphs);
  INLINE static bool get_update_cleared_glyphs();

  int get_num_pages() const;
  DynamicTextPage *get_page(int n) const;

  int garbage_collect();
  void update_texture_memory();
  void clear();

  virtual void write(ostream &out, int indent_level) const;

public:
  virtual bool get_glyph(int character, const TextGlyph *&glyph,
                         float &glyph_scale);

private:
  void update_filters();
  bool reset_scale();
  DynamicTextGlyph *make_glyph(int glyph_index);
  DynamicTextGlyph *slot_glyph(int x_size, int y_size);

  static void initialize_ft_library();

  float _point_size;
  float _pixels_per_unit;
  bool _small_caps;
  float _small_caps_scale;
  int _texture_margin;
  float _poly_margin;
  int _page_x_size, _page_y_size;
  static bool _update_cleared_glyphs;

  Texture::FilterType _minfilter;
  Texture::FilterType _magfilter;
  int _anisotropic_degree;

  typedef pvector< PT(DynamicTextPage) > Pages;
  Pages _pages;
  int _preferred_page;

  // This doesn't need to be a reference-counting pointer, because the
  // reference to each glyph is kept by the DynamicTextPage object.
  typedef pmap<int, DynamicTextGlyph *> Cache;
  Cache _cache;

  // This is a list of the glyphs that do not have any printable
  // properties (e.g. space), but still have an advance measure.  We
  // store them here to keep their reference counts; they also appear
  // in the above table.
  typedef pvector< PT(DynamicTextGlyph) > EmptyGlyphs;
  EmptyGlyphs _empty_glyphs;

  FT_Face _face;

  static FT_Library _ft_library;
  static bool _ft_initialized;
  static bool _ft_ok;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    TextFont::init_type();
    register_type(_type_handle, "DynamicTextFont",
                  TextFont::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;

  friend class TextNode;
};

#include "dynamicTextFont.I"

#endif  // HAVE_FREETYPE

#endif
