// Filename: eggRenderMode.h
// Created by:  drose (20Jan99)
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

#ifndef EGGRENDERMODE_H
#define EGGRENDERMODE_H

#include "pandabase.h"
#include "typedObject.h"


////////////////////////////////////////////////////////////////////
//       Class : EggRenderMode
// Description : This class stores miscellaneous rendering properties
//               that is associated with geometry, and which may be
//               set on the geometry primitive level, on the group
//               above it, or indirectly via a texture.  It's intended
//               to be a base class for egg objects that can have
//               these properties set.
//
//               This class cannot inherit from EggObject, because it
//               causes problems at the EggPolygon level with multiple
//               appearances of the EggObject base class.  And making
//               EggObject a virtual base class is just no fun.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAEGG EggRenderMode {
public:
  INLINE EggRenderMode();
  INLINE EggRenderMode(const EggRenderMode &copy);
  INLINE EggRenderMode &operator = (const EggRenderMode &copy);

  void write(ostream &out, int indent_level) const;

  enum AlphaMode {  // Specifies implementation of transparency.
    AM_unspecified, 
    AM_off,     // No transparency.
    AM_on,      // Use whatever the default model is.
    AM_blend,   // Normal alpha blending, e.g. TransparencyAttrib::M_alpha.
    AM_blend_no_occlude,  // Alpha blending w/o depth write.
    AM_ms,      // TransparencyAttrib::M_multisample
    AM_ms_mask, // TransparencyAttrib::M_multisample_mask
    AM_binary,  // TransparencyAttrib::M_binary
    AM_dual     // TransparencyAttrib::M_dual
  };

  enum DepthWriteMode {
    DWM_unspecified, DWM_off, DWM_on
  };

  enum DepthTestMode {
    DTM_unspecified, DTM_off, DTM_on
  };

  INLINE void set_alpha_mode(AlphaMode mode);
  INLINE AlphaMode get_alpha_mode() const;

  INLINE void set_depth_write_mode(DepthWriteMode mode);
  INLINE DepthWriteMode get_depth_write_mode() const;

  INLINE void set_depth_test_mode(DepthTestMode mode);
  INLINE DepthTestMode get_depth_test_mode() const;

  INLINE void set_draw_order(int order);
  INLINE int get_draw_order() const;
  INLINE bool has_draw_order() const;
  INLINE void clear_draw_order();

  INLINE void set_bin(const string &bin);
  INLINE string get_bin() const;
  INLINE bool has_bin() const;
  INLINE void clear_bin();

  // Comparison operators are handy.
  bool operator == (const EggRenderMode &other) const;
  INLINE bool operator != (const EggRenderMode &other) const;
  bool operator < (const EggRenderMode &other) const;

  static AlphaMode string_alpha_mode(const string &string);
  static DepthWriteMode string_depth_write_mode(const string &string);
  static DepthTestMode string_depth_test_mode(const string &string);

private:
  AlphaMode _alpha_mode;
  DepthWriteMode _depth_write_mode;
  DepthTestMode _depth_test_mode;
  int _draw_order;
  bool _has_draw_order;
  string _bin;


public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    register_type(_type_handle, "EggRenderMode");
  }

private:
  static TypeHandle _type_handle;
};

ostream &operator << (ostream &out, EggRenderMode::AlphaMode mode);
ostream &operator << (ostream &out, EggRenderMode::DepthWriteMode mode);
ostream &operator << (ostream &out, EggRenderMode::DepthTestMode mode);

#include "eggRenderMode.I"

#endif

