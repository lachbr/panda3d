// Filename: frameBufferProperties.cxx
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

#include "frameBufferProperties.h"
#include "string_utils.h"
#include "renderBuffer.h"
#include "config_display.h"

////////////////////////////////////////////////////////////////////
//     Function: FrameBufferProperties::Constructor
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
FrameBufferProperties::
FrameBufferProperties() {
  clear();
}

////////////////////////////////////////////////////////////////////
//     Function: FrameBufferProperties::Copy Assignment Operator
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
void FrameBufferProperties::
operator = (const FrameBufferProperties &copy) {
  _specified = copy._specified;
  _flags = copy._flags;
  _frame_buffer_mode = copy._frame_buffer_mode;
  _depth_bits = copy._depth_bits;
  _color_bits = copy._color_bits;
  _alpha_bits = copy._alpha_bits;
  _stencil_bits = copy._stencil_bits;
  _multisamples = copy._multisamples;
  _aux_rgba = copy._aux_rgba;
  _aux_hrgba = copy._aux_hrgba;
  _aux_float = copy._aux_float;
  _buffer_mask = copy._buffer_mask;
}

////////////////////////////////////////////////////////////////////
//     Function: FrameBufferProperties::subsumes
//       Access: Public
//  Description: Returns true if this set of properties makes
//               strictly greater or equal demands of the framebuffer
//               than the other set of framebuffer properties.
////////////////////////////////////////////////////////////////////
bool FrameBufferProperties::
subsumes(const FrameBufferProperties &other) const {
  // NOT IMPLEMENTED YET
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: FrameBufferProperties::get_default
//       Access: Published, Static
//  Description: Returns a FrameBufferProperties structure with all of
//               the default values filled in according to the user's
//               config file.
////////////////////////////////////////////////////////////////////
FrameBufferProperties FrameBufferProperties::
get_default() {
  FrameBufferProperties props;

  int mode = 0;
  int num_words = framebuffer_mode.get_num_words();
  for (int i = 0; i < num_words; i++) {
    string word = framebuffer_mode.get_word(i);
    if (cmp_nocase_uh(word, "rgb") == 0) {
      mode |= FM_rgb;

    } else if (cmp_nocase_uh(word, "index") == 0) {
      mode |= FM_index;

    } else if (cmp_nocase_uh(word, "single") == 0 ||
               cmp_nocase_uh(word, "single-buffer") == 0) {
      mode = (mode & ~FM_buffer) | FM_single_buffer;

    } else if (cmp_nocase_uh(word, "double") == 0 ||
               cmp_nocase_uh(word, "double-buffer") == 0) {
      mode = (mode & ~FM_buffer) | FM_double_buffer;

    } else if (cmp_nocase_uh(word, "triple") == 0 ||
               cmp_nocase_uh(word, "triple-buffer") == 0) {
      mode = (mode & ~FM_buffer) | FM_triple_buffer;

    } else if (cmp_nocase_uh(word, "accum") == 0) {
      mode |= FM_accum;

    } else if (cmp_nocase_uh(word, "alpha") == 0) {
      mode |= FM_alpha;

    } else if (cmp_nocase_uh(word, "rgba") == 0) {
      mode |= FM_rgba;

    } else if (cmp_nocase_uh(word, "depth") == 0) {
      mode |= FM_depth;

    } else if (cmp_nocase_uh(word, "stencil") == 0) {
      mode |= FM_stencil;

    } else if (cmp_nocase_uh(word, "multisample") == 0) {
      mode |= FM_multisample;

    } else if (cmp_nocase_uh(word, "stereo") == 0) {
      mode |= FM_stereo;

    } else if (cmp_nocase_uh(word, "software") == 0) {
      mode |= FM_software;

    } else if (cmp_nocase_uh(word, "hardware") == 0) {
      mode |= FM_hardware;

    } else {
      display_cat.warning()
        << "Unknown framebuffer keyword: " << word << "\n";
    }
  }

  if (framebuffer_hardware) {
    mode |= FM_hardware;
  }
  if (framebuffer_software) {
    mode |= FM_software;
  }
  if (framebuffer_multisample) {
    mode |= FM_multisample;
  }
  if (framebuffer_depth) {
    mode |= FM_depth;
  }
  if (framebuffer_alpha) {
    mode |= FM_alpha;
  }
  if (framebuffer_stereo) {
    mode |= FM_stereo;
  }

  props.set_frame_buffer_mode(mode);
  props.set_depth_bits(depth_bits);
  props.set_color_bits(color_bits);
  props.set_alpha_bits(alpha_bits);
  props.set_stencil_bits(stencil_bits);
  props.set_multisamples(multisamples);

  props.recalc_buffer_mask();

  return props;
}

////////////////////////////////////////////////////////////////////
//     Function: FrameBufferProperties::operator == 
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
bool FrameBufferProperties::
operator == (const FrameBufferProperties &other) const {
  return (_specified == other._specified &&
          _flags == other._flags &&
          _frame_buffer_mode == other._frame_buffer_mode &&
          _depth_bits == other._depth_bits &&
          _color_bits == other._color_bits &&
          _alpha_bits == other._alpha_bits &&
          _stencil_bits == other._stencil_bits &&
          _multisamples == other._multisamples &&
          _aux_rgba == other._aux_rgba &&
          _aux_hrgba == other._aux_hrgba &&
          _aux_float == other._aux_float &&
          _buffer_mask == other._buffer_mask);
}

////////////////////////////////////////////////////////////////////
//     Function: FrameBufferProperties::clear
//       Access: Published
//  Description: Unsets all properties that have been specified so
//               far, and resets the FrameBufferProperties structure to its
//               initial empty state.
////////////////////////////////////////////////////////////////////
void FrameBufferProperties::
clear() {
  _specified = 0;
  _flags = 0;
  _frame_buffer_mode = 0;
  _depth_bits = 1;
  _color_bits = 1;
  _alpha_bits = 1;
  _stencil_bits = 1;
  _multisamples = 1;
  _aux_rgba = 0;
  _aux_hrgba = 0;
  _aux_float = 0;
  recalc_buffer_mask();
}

////////////////////////////////////////////////////////////////////
//     Function: FrameBufferProperties::add_properties
//       Access: Published
//  Description: Sets any properties that are explicitly specified in
//               other on this object.  Leaves other properties
//               unchanged.
////////////////////////////////////////////////////////////////////
void FrameBufferProperties::
add_properties(const FrameBufferProperties &other) {
  if (other.has_frame_buffer_mode()) {
    set_frame_buffer_mode(other.get_frame_buffer_mode());
  }
  if (other.has_depth_bits()) {
    set_depth_bits(other.get_depth_bits());
  }
  if (other.has_color_bits()) {
    set_color_bits(other.get_color_bits());
  }
  if (other.has_alpha_bits()) {
    set_alpha_bits(other.get_alpha_bits());
  }
  if (other.has_stencil_bits()) {
    set_stencil_bits(other.get_stencil_bits());
  }
  if (other.has_multisamples()) {
    set_multisamples(other.get_multisamples());
  }
  if (other.has_aux_rgba()) {
    set_aux_rgba(other.get_aux_rgba());
  }
  if (other.has_aux_hrgba()) {
    set_aux_hrgba(other.get_aux_hrgba());
  }
  if (other.has_aux_float()) {
    set_aux_float(other.get_aux_float());
  }
}

////////////////////////////////////////////////////////////////////
//     Function: FrameBufferProperties::output
//       Access: Published
//  Description: Sets any properties that are explicitly specified in
//               other on this object.  Leaves other properties
//               unchanged.
////////////////////////////////////////////////////////////////////
void FrameBufferProperties::
output(ostream &out) const {
  if (has_frame_buffer_mode()) {
    out << "frameBuffer_mode=";
    int frameBuffer_mode = get_frame_buffer_mode();
    if ((frameBuffer_mode & FM_index) != 0) {
      out << "FM_index";
    } else {
      out << "FM_rgb";
    }

    if ((frameBuffer_mode & FM_triple_buffer) != 0) {
      out << "|FM_triple_buffer";
    } else if ((frameBuffer_mode & FM_double_buffer) != 0) {
      out << "|FM_double_buffer";
    } else {
      out << "|FM_single_buffer";
    }

    if ((frameBuffer_mode & FM_accum) != 0) {
      out << "|FM_accum";
    }
    if ((frameBuffer_mode & FM_alpha) != 0) {
      out << "|FM_alpha";
    }
    if ((frameBuffer_mode & FM_depth) != 0) {
      out << "|FM_depth";
    }
    if ((frameBuffer_mode & FM_stencil) != 0) {
      out << "|FM_stencil";
    }
    if ((frameBuffer_mode & FM_multisample) != 0) {
      out << "|FM_multisample";
    }
    if ((frameBuffer_mode & FM_stereo) != 0) {
      out << "|FM_stereo";
    }
    if ((frameBuffer_mode & FM_software) != 0) {
      out << "|FM_software";
    }
    if ((frameBuffer_mode & FM_hardware) != 0) {
      out << "|FM_hardware";
    }
    out << " ";
  }
  if (has_depth_bits()) {
    out << "depth_bits=" << get_depth_bits() << " ";
  }
  if (has_color_bits()) {
    out << "color_bits=" << get_color_bits() << " ";
  }
  if (has_alpha_bits()) {
    out << "alpha_bits=" << get_alpha_bits() << " ";
  }
  if (has_stencil_bits()) {
    out << "stencil_bits=" << get_stencil_bits() << " ";
  }
  if (has_multisamples()) {
    out << "multisamples=" << get_multisamples() << " ";
  }
  if (has_aux_rgba()) {
    out << "aux_rgba=" << get_aux_rgba() << " ";
  }
  if (has_aux_hrgba()) {
    out << "aux_hrgba=" << get_aux_hrgba() << " ";
  }
  if (has_aux_float()) {
    out << "aux_float=" << get_aux_float() << " ";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: FrameBufferProperties::recalc_buffer_mask
//       Access: Private
//  Description: Calculates the buffer mask based on the mode
//               and aux bitplanes.
////////////////////////////////////////////////////////////////////
void FrameBufferProperties::
recalc_buffer_mask() {
  if ((_frame_buffer_mode & FM_buffer) == FM_single_buffer) {
    _buffer_mask = RenderBuffer::T_front;
  } else {
    _buffer_mask = RenderBuffer::T_front | RenderBuffer::T_back;
  }
  if (_frame_buffer_mode & FM_depth) {
    _buffer_mask |= RenderBuffer::T_depth;
  }
  if (_frame_buffer_mode & FM_stencil) {
    _buffer_mask |= RenderBuffer::T_stencil;
  }
  for (int aux_rgba=0; aux_rgba < _aux_rgba; ++aux_rgba) {
    _buffer_mask |= (RenderBuffer::T_aux_rgba_0 << aux_rgba);
  }
  for (int aux_hrgba=0; aux_hrgba < _aux_hrgba; ++aux_hrgba) {
    _buffer_mask |= (RenderBuffer::T_aux_hrgba_0 << aux_hrgba);
  }
  for (int aux_float=0; aux_float < _aux_float; ++aux_float) {
    _buffer_mask |= (RenderBuffer::T_aux_float_0 << aux_float);
  }
}
