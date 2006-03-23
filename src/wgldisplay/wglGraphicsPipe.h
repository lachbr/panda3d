// Filename: wglGraphicsPipe.h
// Created by:  drose (20Dec02)
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

#ifndef WGLGRAPHICSPIPE_H
#define WGLGRAPHICSPIPE_H

#include "pandabase.h"
#include "winGraphicsPipe.h"

class wglGraphicsStateGuardian;

////////////////////////////////////////////////////////////////////
//       Class : wglGraphicsPipe
// Description : This graphics pipe represents the interface for
//               creating OpenGL graphics windows on the various
//               Windows OSes.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAGL wglGraphicsPipe : public WinGraphicsPipe {
public:
  wglGraphicsPipe();
  virtual ~wglGraphicsPipe();

  virtual string get_interface_name() const;
  static PT(GraphicsPipe) pipe_constructor();

protected:
  virtual PT(GraphicsOutput) make_output(const string &name,
                                         const FrameBufferProperties &properties,
                                         int x_size, int y_size, int flags,
                                         GraphicsStateGuardian *gsg,
                                         GraphicsOutput *host,
                                         int retry,
                                         bool &precertify);
  virtual PT(GraphicsStateGuardian) make_gsg(const FrameBufferProperties &properties, 
                                             GraphicsStateGuardian *share_with);

private:
  
  static int choose_pfnum(const FrameBufferProperties &properties, HDC hdc);
  static int try_for_pfnum(HDC hdc, bool hardware, bool software, 
                           int frame_buffer_mode,
                           int want_depth_bits = 1, int want_color_bits = 1,
                           int want_alpha_bits = 1, int want_stencil_bits = 1);

  static void get_properties(FrameBufferProperties &properties, HDC hdc,
                             int pfnum);

  static int choose_pfnum_advanced(const FrameBufferProperties &properties, 
                                   const wglGraphicsStateGuardian *wglgsg,
                                   HDC window_dc, int orig_pfnum);
  static int try_for_pfnum_advanced(int orig_pfnum, 
                                    const wglGraphicsStateGuardian *wglgsg,
                                    HDC window_dc, int frame_buffer_mode,
                                    int want_depth_bits = 1, int want_color_bits = 1,
                                    int want_alpha_bits = 1, int want_stencil_bits = 1,
                                    int want_multisamples = 1);
  static bool get_properties_advanced(FrameBufferProperties &properties, 
                                      const wglGraphicsStateGuardian *wglgsg, 
                                      HDC window_dc, int pfnum);
  static string format_pfd_flags(DWORD pfd_flags);

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    WinGraphicsPipe::init_type();
    register_type(_type_handle, "wglGraphicsPipe",
                  WinGraphicsPipe::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;

  friend class wglGraphicsBuffer;
  friend class wglGraphicsWindow;
};

#include "wglGraphicsPipe.I"

#endif
