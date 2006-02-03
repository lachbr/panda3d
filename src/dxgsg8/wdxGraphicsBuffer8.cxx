// Filename: wdxGraphicsBuffer8.cxx
// Created by:  drose (08Feb04)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001 - 2005, Disney Enterprises, Inc.  All rights reserved
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

#include "wdxGraphicsPipe8.h"
#include "wdxGraphicsBuffer8.h"
#include "pStatTimer.h"


// ISSUES:
  // render to texure format
    // can be specified via the DXGraphicsStateGuardian8 member
    // _render_to_texture_d3d_format  default = D3DFMT_X8R8G8B8

  // should check texture creation with CheckDepthStencilMatch
  // support copy from texture to ram?
    // check D3DCAPS2_DYNAMICTEXTURES

#define FL << "\n" << __FILE__ << " " << __LINE__ << "\n"

TypeHandle wdxGraphicsBuffer8::_type_handle;


////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsBuffer8::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
wdxGraphicsBuffer8::
wdxGraphicsBuffer8(GraphicsPipe *pipe, GraphicsStateGuardian *gsg,
                  const string &name,
                  int x_size, int y_size) :
  GraphicsBuffer(pipe, gsg, name, x_size, y_size)
{
  // initialize all class members
  _cube_map_index = -1;
  _back_buffer = NULL;
  _z_stencil_buffer = NULL;
  _direct_3d_surface = NULL;
  _dx_texture_context8 = NULL;
  _new_z_stencil_surface = NULL;

// is this correct ???
  // Since the pbuffer never gets flipped, we get screenshots from the
  // same buffer we draw into.
  _screenshot_buffer_type = _draw_buffer_type;
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsBuffer8::Destructor
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
wdxGraphicsBuffer8::
~wdxGraphicsBuffer8() {
  this -> close_buffer ( );
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsBuffer8::begin_frame
//       Access: Public, Virtual
//  Description: This function will be called within the draw thread
//               before beginning rendering for a given frame.  It
//               should do whatever setup is required, and return true
//               if the frame should be rendered, or false if it
//               should be skipped.
////////////////////////////////////////////////////////////////////
bool wdxGraphicsBuffer8::
begin_frame() {
  begin_frame_spam();
  if (_gsg == (GraphicsStateGuardian *)NULL) {
    return false;
  }
  auto_resize();
  if (needs_context()) {
    if (!make_context()) {
      return false;
    }
  }
  make_current();
  begin_render_texture();
  clear_cube_map_selection();
  return _gsg->begin_frame();
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsBuffer8::end_frame
//       Access: Public, Virtual
//  Description: This function will be called within the draw thread
//               after rendering is completed for a given frame.  It
//               should do whatever finalization is required.
////////////////////////////////////////////////////////////////////
void wdxGraphicsBuffer8::
end_frame() {
  end_frame_spam();
  nassertv(_gsg != (GraphicsStateGuardian *)NULL);
  _gsg->end_frame();
  end_render_texture();
  copy_to_textures();
  trigger_flip();
  if (_one_shot) {
    prepare_for_deletion();
  }
  clear_cube_map_selection();
}

////////////////////////////////////////////////////////////////////
//     Function: wglGraphicsStateGuardian::begin_render_texture
//       Access: Public, Virtual
//  Description: If the GraphicsOutput supports direct render-to-texture,
//               and if any setup needs to be done during begin_frame,
//               then the setup code should go here.  Any textures that
//               can not be rendered to directly should be reflagged
//               as RTM_copy_texture.
////////////////////////////////////////////////////////////////////
void wdxGraphicsBuffer8::
begin_render_texture() {

  if (_gsg != (GraphicsStateGuardian *)NULL) {

    DXGraphicsStateGuardian8 *dxgsg;
    DCAST_INTO_V(dxgsg, _gsg);

    HRESULT hr;
    bool state;

    state = false;

    if (dxgsg -> _d3d_device) {
      Texture *tex = get_texture(0);
      tex->set_render_to_texture(true);
      TextureContext *tc = tex->prepare_now(_gsg->get_prepared_objects(), _gsg);

      _dx_texture_context8 = DCAST (DXTextureContext8, tc);

      // save render context
      hr = dxgsg -> _d3d_device -> GetRenderTarget (&_back_buffer);
      if (SUCCEEDED (hr)) {
        hr = dxgsg -> _d3d_device -> GetDepthStencilSurface (&_z_stencil_buffer);
        if (SUCCEEDED (hr)) {
          state = true;
        }
        else {
          dxgsg8_cat.error ( ) << "GetDepthStencilSurface " << D3DERRORSTRING(hr) FL;
        }
      }
      else {
        dxgsg8_cat.error ( ) << "GetRenderTarget " << D3DERRORSTRING(hr) FL;
      }

      // set render context
      if (state && _dx_texture_context8)
      {
        DXTextureContext8 *dx_texture_context8;

        // use saved dx_texture_context8
        dx_texture_context8 = _dx_texture_context8;

        UINT mipmap_level;

        mipmap_level = 0;
        _direct_3d_surface = NULL;

        // render to texture 2D
        IDirect3DTexture8 *direct_3d_texture;

        direct_3d_texture = dx_texture_context8 -> _d3d_2d_texture;
        if (direct_3d_texture) {
          hr = direct_3d_texture -> GetSurfaceLevel (mipmap_level, &_direct_3d_surface);
          if (SUCCEEDED (hr)) {
            IDirect3DSurface8 *z_stencil_buffer;

            if (this -> _new_z_stencil_surface) {
              z_stencil_buffer = this -> _new_z_stencil_surface;
            }
            else {
              z_stencil_buffer = _z_stencil_buffer;
            }

            hr = dxgsg -> _d3d_device -> SetRenderTarget (_direct_3d_surface, z_stencil_buffer);
            if (SUCCEEDED (hr)) {

            } else {
              dxgsg8_cat.error ( ) << "SetRenderTarget " << D3DERRORSTRING(hr) FL;
            }
          } else {
            dxgsg8_cat.error ( ) << "GetSurfaceLevel " << D3DERRORSTRING(hr) FL;
          }
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsOutput::end_render_texture
//       Access: Public, Virtual
//  Description: If the GraphicsOutput supports direct render-to-texture,
//               and if any setup needs to be done during end_frame,
//               then the setup code should go here.  Any textures that
//               could not be rendered to directly should be reflagged
//               as RTM_copy_texture.
////////////////////////////////////////////////////////////////////
void wdxGraphicsBuffer8::
end_render_texture() {

  if (_gsg != (GraphicsStateGuardian *)NULL) {

    DXGraphicsStateGuardian8 *dxgsg;
    DCAST_INTO_V(dxgsg, _gsg);

    if (dxgsg -> _d3d_device) {
      // restore render context
      HRESULT hr;

      if (_back_buffer) {
        hr = dxgsg -> _d3d_device -> SetRenderTarget (_back_buffer,
          _z_stencil_buffer);
        if (SUCCEEDED (hr)) {
          _back_buffer -> Release ( );
        } else {
          dxgsg8_cat.error ( ) << "SetRenderTarget " << D3DERRORSTRING(hr) FL;
        }
        _back_buffer = NULL;
      }
      if (_z_stencil_buffer) {
        _z_stencil_buffer -> Release ( );
        _z_stencil_buffer = NULL;
      }
      if (_direct_3d_surface) {
        _direct_3d_surface -> Release ( );
        _direct_3d_surface = NULL;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsBuffer8::select_cube_map
//       Access: Public, Virtual
//  Description: Called internally when the window is in
//               render-to-a-texture mode and we are in the process of
//               rendering the six faces of a cube map.  This should
//               do whatever needs to be done to switch the buffer to
//               the indicated face.
////////////////////////////////////////////////////////////////////
void wdxGraphicsBuffer8::
select_cube_map(int cube_map_index) {

  if (_gsg != (GraphicsStateGuardian *)NULL) {

    DXGraphicsStateGuardian8 *dxgsg;
    DCAST_INTO_V(dxgsg, _gsg);

    _cube_map_index = cube_map_index;

    HRESULT hr;
    UINT mipmap_level;
    int render_target_index;
    DXTextureContext8 *dx_texture_context8;

    mipmap_level = 0;
    render_target_index = 0;
    dx_texture_context8 = _dx_texture_context8;

    if (_direct_3d_surface) {
        _direct_3d_surface -> Release ( );
        _direct_3d_surface = NULL;
    }

    if (dxgsg -> _d3d_device) {

      // render to cubemap face
      IDirect3DCubeTexture8 *direct_3d_cube_texture;

      direct_3d_cube_texture = dx_texture_context8 -> _d3d_cube_texture;
      if (direct_3d_cube_texture) {
        if (_cube_map_index >= 0 && _cube_map_index < 6) {
          hr = direct_3d_cube_texture -> GetCubeMapSurface (
            (D3DCUBEMAP_FACES) _cube_map_index, mipmap_level, &_direct_3d_surface);
          if (SUCCEEDED (hr)) {
            IDirect3DSurface8 *z_stencil_buffer;

            if (this -> _new_z_stencil_surface) {
              z_stencil_buffer = this -> _new_z_stencil_surface;
            }
            else {
              z_stencil_buffer = _z_stencil_buffer;
            }

            hr = dxgsg -> _d3d_device -> SetRenderTarget (_direct_3d_surface, z_stencil_buffer);
            if (SUCCEEDED (hr)) {

            } else {
              dxgsg8_cat.error ( ) << "SetRenderTarget " << D3DERRORSTRING(hr) FL;
            }
          } else {
            dxgsg8_cat.error ( ) << "GetCubeMapSurface " << D3DERRORSTRING(hr) FL;
          }
        } else {
          dxgsg8_cat.error ( ) << "Invalid Cube Map Face Index " << _cube_map_index FL;
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsBuffer8::make_current
//       Access: Public, Virtual
//  Description: This function will be called within the draw thread
//               during begin_frame() to ensure the graphics context
//               is ready for drawing.
////////////////////////////////////////////////////////////////////
void wdxGraphicsBuffer8::
make_current() {
  PStatTimer timer(_make_current_pcollector);

  DXGraphicsStateGuardian8 *dxgsg;
  DCAST_INTO_V(dxgsg, _gsg);

  // do nothing here
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsBuffer8::release_gsg
//       Access: Public, Virtual
//  Description: Releases the current GSG pointer, if it is currently
//               held, and resets the GSG to NULL.  The window will be
//               permanently unable to render; this is normally called
//               only just before destroying the window.  This should
//               only be called from within the draw thread.
////////////////////////////////////////////////////////////////////
void wdxGraphicsBuffer8::
release_gsg() {
  GraphicsBuffer::release_gsg();
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsBuffer8::process_events
//       Access: Public, Virtual
//  Description: Do whatever processing is necessary to ensure that
//               the window responds to user events.  Also, honor any
//               requests recently made via request_properties()
//
//               This function is called only within the window
//               thread.
////////////////////////////////////////////////////////////////////
void wdxGraphicsBuffer8::
process_events() {
  GraphicsBuffer::process_events();

  MSG msg;

  // Handle all the messages on the queue in a row.  Some of these
  // might be for another window, but they will get dispatched
  // appropriately.
  while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
    process_1_event();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsBuffer8::close_buffer
//       Access: Protected, Virtual
//  Description: Closes the buffer right now.  Called from the window
//               thread.
////////////////////////////////////////////////////////////////////
void wdxGraphicsBuffer8::
close_buffer() {

  if (_gsg != (GraphicsStateGuardian *)NULL) {
    DXGraphicsStateGuardian8 *dxgsg;
    DCAST_INTO_V(dxgsg, _gsg);

    DXTextureContext8 *dx_texture_context8;

    dx_texture_context8 = _dx_texture_context8;
    if (dx_texture_context8) {
      // release render texture
      if (dx_texture_context8 -> _d3d_texture) {
        dx_texture_context8 -> _d3d_texture -> Release ( );
      }
      dx_texture_context8 -> _d3d_texture = NULL;
      dx_texture_context8 -> _d3d_2d_texture = NULL;
      dx_texture_context8 -> _d3d_volume_texture = NULL;
      dx_texture_context8 -> _d3d_cube_texture = NULL;
    }

    // release new depth stencil buffer if one was created
    if (this -> _new_z_stencil_surface) {
      this -> _new_z_stencil_surface -> Release ( );
      this -> _new_z_stencil_surface = NULL;
    }
  }

  _cube_map_index = -1;

  _is_valid = false;
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsBuffer8::open_buffer
//       Access: Protected, Virtual
//  Description: Opens the window right now.  Called from the window
//               thread.  Returns true if the window is successfully
//               opened, or false if there was a problem.
////////////////////////////////////////////////////////////////////
bool wdxGraphicsBuffer8::
open_buffer() {
  bool state;
  DXGraphicsStateGuardian8 *dxgsg;
  DCAST_INTO_R(dxgsg, _gsg, false);

  // create texture
  int tex_index;

  state = false;
  _is_valid = false;

  // assume tex_index is 0
  tex_index = 0;

  Texture *tex = get_texture(tex_index);

  // render to texture must be set
  tex->set_render_to_texture(true);

  TextureContext *tc = tex->prepare_now(_gsg->get_prepared_objects(), _gsg);
  if (tc != NULL) {
    HRESULT hr;
    IDirect3DSurface8 *_depth_stencil_surface;

    _dx_texture_context8 = DCAST (DXTextureContext8, tc);

    // create a depth stencil buffer if needed
    hr = dxgsg -> _d3d_device -> GetDepthStencilSurface (&_depth_stencil_surface);
    if (SUCCEEDED  (hr))
    {
      D3DSURFACE_DESC surface_description;

      // get and copy the current depth stencil's parameters for the new buffer
      hr = _depth_stencil_surface -> GetDesc (&surface_description);
      if (SUCCEEDED  (hr)) {
        UINT width;
        UINT height;
        D3DFORMAT format;
        D3DMULTISAMPLE_TYPE multisample_type;
//        DWORD multisample_quality;
        BOOL discard;

        width = tex -> get_x_size ( );
        height = tex -> get_y_size ( );

//        dxgsg8_cat.error ( ) << "-------------RTT SIZE " << "t width " << width << " t height " << height << "\n";

        if (surface_description.Width < width || surface_description.Height < height) {
          format = surface_description.Format;
          multisample_type = surface_description.MultiSampleType;
//          multisample_quality = surface_description.MultiSampleQuality;
          discard = false;

          hr = dxgsg -> _d3d_device -> CreateDepthStencilSurface (
            width, height, format, multisample_type, /* multisample_quality,
            discard, */ &this -> _new_z_stencil_surface);
          if (SUCCEEDED  (hr)) {

//            dxgsg8_cat.error ( ) << "-------------CreatedDepthStencilSurface for RTT \n";

            state = true;

          } else {
            dxgsg8_cat.error ( ) << "CreateDepthStencilSurface " << D3DERRORSTRING(hr) FL;
          }
        }
        else {
          // no need to create a separate depth stencil buffer since
          // the current depth stencil buffer size is big enough
          state = true;
        }
      }
      else {
        dxgsg8_cat.error ( ) << "GetDesc " << D3DERRORSTRING(hr) FL;
      }

      _depth_stencil_surface -> Release ( );
    }
    else {
      dxgsg8_cat.error ( ) << "GetDepthStencilSurface " << D3DERRORSTRING(hr) FL;
    }

    if (state) {
      _is_valid = true;
    }
  }

  return state;
}

////////////////////////////////////////////////////////////////////
//     Function: wdxGraphicsBuffer8::process_1_event
//       Access: Private, Static
//  Description: Handles one event from the message queue.
////////////////////////////////////////////////////////////////////
void wdxGraphicsBuffer8::
process_1_event() {
  MSG msg;

  if (!GetMessage(&msg, NULL, 0, 0)) {
    // WM_QUIT received.  We need a cleaner way to deal with this.
    //    DestroyAllWindows(false);
    exit(msg.wParam);  // this will invoke AtExitFn
  }

  // Translate virtual key messages
  TranslateMessage(&msg);
  // Call window_proc
  DispatchMessage(&msg);
}
