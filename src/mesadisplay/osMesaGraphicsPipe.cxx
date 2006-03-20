// Filename: osMesaGraphicsPipe.cxx
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

#include "osMesaGraphicsPipe.h"
#include "osMesaGraphicsBuffer.h"
#include "osMesaGraphicsStateGuardian.h"
#include "config_mesadisplay.h"
#include "frameBufferProperties.h"
#include "mutexHolder.h"

TypeHandle OsMesaGraphicsPipe::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: OsMesaGraphicsPipe::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
OsMesaGraphicsPipe::
OsMesaGraphicsPipe() {
  _supported_types = OT_buffer | OT_texture_buffer;
  _is_valid = true;
}

////////////////////////////////////////////////////////////////////
//     Function: OsMesaGraphicsPipe::Destructor
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
OsMesaGraphicsPipe::
~OsMesaGraphicsPipe() {
}

////////////////////////////////////////////////////////////////////
//     Function: OsMesaGraphicsPipe::get_interface_name
//       Access: Published, Virtual
//  Description: Returns the name of the rendering interface
//               associated with this GraphicsPipe.  This is used to
//               present to the user to allow him/her to choose
//               between several possible GraphicsPipes available on a
//               particular platform, so the name should be meaningful
//               and unique for a given platform.
////////////////////////////////////////////////////////////////////
string OsMesaGraphicsPipe::
get_interface_name() const {
  return "Offscreen Mesa";
}

////////////////////////////////////////////////////////////////////
//     Function: OsMesaGraphicsPipe::pipe_constructor
//       Access: Public, Static
//  Description: This function is passed to the GraphicsPipeSelection
//               object to allow the user to make a default
//               OsMesaGraphicsPipe.
////////////////////////////////////////////////////////////////////
PT(GraphicsPipe) OsMesaGraphicsPipe::
pipe_constructor() {
  return new OsMesaGraphicsPipe;
}

////////////////////////////////////////////////////////////////////
//     Function: OsMesaGraphicsPipe::make_gsg
//       Access: Protected, Virtual
//  Description: Creates a new GSG to use the pipe (but no windows
//               have been created yet for the GSG).  This method will
//               be called in the draw thread for the GSG.
////////////////////////////////////////////////////////////////////
PT(GraphicsStateGuardian) OsMesaGraphicsPipe::
make_gsg(const FrameBufferProperties &properties, 
         GraphicsStateGuardian *share_with) {

  OSMesaGraphicsStateGuardian *share_gsg = NULL;

  if (share_with != (GraphicsStateGuardian *)NULL) {
    if (!share_with->is_exact_type(OSMesaGraphicsStateGuardian::get_class_type())) {
      mesadisplay_cat.error()
        << "Cannot share context between OSMesaGraphicsStateGuardian and "
        << share_with->get_type() << "\n";
      return NULL;
    }

    DCAST_INTO_R(share_gsg, share_with, NULL);
  }

  // We ignore the requested properties; OSMesa contexts are all the
  // same.

  FrameBufferProperties mesa_props;

  int frame_buffer_mode = 
    FrameBufferProperties::FM_rgba | 
    FrameBufferProperties::FM_single_buffer | 
    FrameBufferProperties::FM_accum | 
    FrameBufferProperties::FM_depth | 
    FrameBufferProperties::FM_stencil | 
    FrameBufferProperties::FM_software;

  mesa_props.set_frame_buffer_mode(frame_buffer_mode);
  mesa_props.set_color_bits(24);
  mesa_props.set_alpha_bits(8);
  mesa_props.set_stencil_bits(8);
  mesa_props.set_depth_bits(8);

  return new OSMesaGraphicsStateGuardian(mesa_props, share_gsg);
}

////////////////////////////////////////////////////////////////////
//     Function: OsMesaGraphicsPipe::make_output
//       Access: Protected, Virtual
//  Description: Creates a new window on the pipe, if possible.
////////////////////////////////////////////////////////////////////
PT(GraphicsOutput) OsMesaGraphicsPipe::
make_output(const string &name,
            int x_size, int y_size, int flags,
            GraphicsStateGuardian *gsg,
            GraphicsOutput *host,
            int retry,
            bool precertify) {
  
  if (!_is_valid) {
    return NULL;
  }
  
  // First thing to try: an OsMesaGraphicsBuffer

  if (retry == 0) {
    if ((!support_render_texture)||
        ((flags&BF_require_parasite)!=0)||
        ((flags&BF_require_window)!=0)||
        ((flags&BF_need_aux_rgba_MASK)!=0)||
        ((flags&BF_need_aux_hrgba_MASK)!=0)||
        ((flags&BF_need_aux_float_MASK)!=0)||
        ((flags&BF_size_track_host)!=0)||
        ((flags&BF_can_bind_every)!=0)) {
      return NULL;
    }
    return new OsMesaGraphicsBuffer(this, name, x_size, y_size, flags, gsg, host);
  }
  
  // Nothing else left to try.
  return NULL;
}
