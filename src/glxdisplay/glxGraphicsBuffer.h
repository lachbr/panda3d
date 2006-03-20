// Filename: glxGraphicsBuffer.h
// Created by:  drose (09Feb04)
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

#ifndef GLXGRAPHICSBUFFER_H
#define GLXGRAPHICSBUFFER_H

#include "pandabase.h"

#include "glxGraphicsPipe.h"
#include "graphicsBuffer.h"

#ifdef HAVE_GLXFBCONFIG
// This whole class doesn't make sense unless we have the GLXFBConfig
// and associated GLXPbuffer interfaces available.

////////////////////////////////////////////////////////////////////
//       Class : glxGraphicsBuffer
// Description : An offscreen buffer in the GLX environment.  This
//               creates a GLXPbuffer.
////////////////////////////////////////////////////////////////////
class glxGraphicsBuffer : public GraphicsBuffer {
public:
  glxGraphicsBuffer(GraphicsPipe *pipe, 
                    const string &name,
                    int x_size, int y_size, int flags,
                    GraphicsStateGuardian *gsg,
                    GraphicsOutput *host);
  virtual ~glxGraphicsBuffer();

  virtual bool begin_frame(FrameMode mode);
  virtual void end_frame(FrameMode mode);
  virtual void release_gsg();

protected:
  virtual void close_buffer();
  virtual bool open_buffer();

private:
  Display *_display;
  GLXPbuffer _pbuffer;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    GraphicsBuffer::init_type();
    register_type(_type_handle, "glxGraphicsBuffer",
                  GraphicsBuffer::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

#include "glxGraphicsBuffer.I"

#endif  // HAVE_GLXFBCONFIG

#endif
