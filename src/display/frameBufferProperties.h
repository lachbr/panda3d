// Filename: frameBufferProperties.h
// Created by:  drose (27Jan03)
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

#ifndef FRAMEBUFFERPROPERTIES_H
#define FRAMEBUFFERPROPERTIES_H

#include "pandabase.h"

////////////////////////////////////////////////////////////////////
//       Class : FrameBufferProperties
// Description : A container for the various kinds of properties we
//               might ask to have on a graphics frameBuffer before we
//               create a GSG.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA FrameBufferProperties {
PUBLISHED:
  FrameBufferProperties();
  INLINE FrameBufferProperties(const FrameBufferProperties &copy);
  void operator = (const FrameBufferProperties &copy);
  INLINE ~FrameBufferProperties();

  static FrameBufferProperties get_default();

  bool operator == (const FrameBufferProperties &other) const;
  INLINE bool operator != (const FrameBufferProperties &other) const;

  enum FrameBufferMode {
    FM_rgb            = 0x0000,
    FM_index          = 0x0001,
    FM_single_buffer  = 0x0000,
    FM_double_buffer  = 0x0002,
    FM_triple_buffer  = 0x0004,
    FM_buffer         = 0x0006,  // == (FM_single_buffer | FM_double_buffer | FM_triple_buffer)
    FM_accum          = 0x0008,
    FM_alpha          = 0x0010,
    FM_rgba           = 0x0010,  // == (FM_rgb | FM_alpha)
    FM_depth          = 0x0020,
    FM_stencil        = 0x0040,
    FM_multisample    = 0x0080,
    FM_stereo         = 0x0100,
    FM_software       = 0x0200,
    FM_hardware       = 0x0400,
  };
  
  void clear();
  INLINE void set_specified();
  INLINE int get_buffer_mask() const;
  INLINE bool is_any_specified() const;
  INLINE bool has_mode(int bit) const;
  INLINE bool is_single_buffered() const;
  INLINE bool is_stereo() const;
  bool subsumes(const FrameBufferProperties &prop) const;
  
  INLINE void set_frame_buffer_mode(int frameBuffer_mode);
  INLINE int get_frame_buffer_mode() const;
  INLINE bool has_frame_buffer_mode() const;
  INLINE void clear_frame_buffer_mode();

  INLINE void set_depth_bits(int depth_bits);
  INLINE int get_depth_bits() const;
  INLINE bool has_depth_bits() const;
  INLINE void clear_depth_bits();

  INLINE void set_color_bits(int color_bits);
  INLINE int get_color_bits() const;
  INLINE bool has_color_bits() const;
  INLINE void clear_color_bits();

  INLINE void set_alpha_bits(int alpha_bits);
  INLINE int get_alpha_bits() const;
  INLINE bool has_alpha_bits() const;
  INLINE void clear_alpha_bits();

  INLINE void set_stencil_bits(int stencil_bits);
  INLINE int get_stencil_bits() const;
  INLINE bool has_stencil_bits() const;
  INLINE void clear_stencil_bits();

  INLINE void set_multisamples(int multisamples);
  INLINE int get_multisamples() const;
  INLINE bool has_multisamples() const;
  INLINE void clear_multisamples();

  INLINE void set_aux_rgba(int naux);
  INLINE int get_aux_rgba() const;
  INLINE bool has_aux_rgba() const;
  INLINE void clear_aux_rgba();
  
  INLINE void set_aux_hrgba(int naux);
  INLINE int get_aux_hrgba() const;
  INLINE bool has_aux_hrgba() const;
  INLINE void clear_aux_hrgba();

  INLINE void set_aux_float(int naux);
  INLINE int get_aux_float() const;
  INLINE bool has_aux_float() const;
  INLINE void clear_aux_float();

  void add_properties(const FrameBufferProperties &other);
  void output(ostream &out) const;

public:
  INLINE void buffer_mask_add(int bits);
  INLINE void buffer_mask_remove(int bits);

private:
  void recalc_buffer_mask();

private:
  // This bitmask indicates which of the parameters in the properties
  // structure have been filled in by the user, and which remain
  // unspecified.
  enum Specified {
    S_frame_buffer_mode = 0x0001,
    S_depth_bits        = 0x0002,
    S_color_bits        = 0x0004,
    S_alpha_bits        = 0x0008,
    S_stencil_bits      = 0x0010,
    S_multisamples      = 0x0020,
    S_aux_rgba          = 0x0040,
    S_aux_hrgba         = 0x0080,
    S_aux_float         = 0x0100,
    S_ALL_SPECIFIED     = 0x01FF,
  };

  int _specified;
  int _flags;
  int _frame_buffer_mode;
  int _depth_bits;
  int _color_bits;
  int _alpha_bits;
  int _stencil_bits;
  int _multisamples;
  int _aux_rgba;
  int _aux_hrgba;
  int _aux_float;
  int _buffer_mask;
};

INLINE ostream &operator << (ostream &out, const FrameBufferProperties &properties);

#include "frameBufferProperties.I"

#endif
