// Filename: glxGraphicsWindow.cxx
// Created by:  mike (09Jan97)
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

#include "glxGraphicsWindow.h"
#include "config_glxdisplay.h"
#include "glxGraphicsPipe.h"

#include "graphicsPipe.h"
#include "keyboardButton.h"
#include "mouseButton.h"
#include "glGraphicsStateGuardian.h"
#include "clockObject.h"

#include <errno.h>
#include <sys/time.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

TypeHandle glxGraphicsWindow::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsWindow::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
glxGraphicsWindow::
glxGraphicsWindow(GraphicsPipe *pipe) :
  GraphicsWindow(pipe) 
{
  glxGraphicsPipe *glx_pipe;
  DCAST_INTO_V(glx_pipe, _pipe);
  _display = glx_pipe->get_display();
  _screen = glx_pipe->get_screen();
  _xwindow = (Window)0;
  _context = (GLXContext)0;
  _visual = (XVisualInfo *)NULL;

  GraphicsWindowInputDevice device =
    GraphicsWindowInputDevice::pointer_and_keyboard("keyboard/mouse");
  _input_devices.push_back(device);
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsWindow::Destructor
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
glxGraphicsWindow::
~glxGraphicsWindow() {
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsWindow::make_gsg
//       Access: Public, Virtual
//  Description: Creates a new GSG for the window and stores it in the
//               _gsg pointer.  This should only be called from within
//               the draw thread.
////////////////////////////////////////////////////////////////////
void glxGraphicsWindow::
make_gsg() {
  nassertv(_gsg == (GraphicsStateGuardian *)NULL);

  // First, we need to create the rendering context.
  _context = glXCreateContext(_display, _visual, None, GL_TRUE);
  if (!_context) {
    glxdisplay_cat.error()
      << "Could not create GLX context.\n";
    return;
  }

  // And make sure the new context is current.
  glXMakeCurrent(_display, _xwindow, _context);

  // Now we can make a GSG.
  _gsg = new GLGraphicsStateGuardian(this);
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsWindow::release_gsg
//       Access: Public, Virtual
//  Description: Releases the current GSG pointer, if it is currently
//               held, and resets the GSG to NULL.  This should only
//               be called from within the draw thread.
////////////////////////////////////////////////////////////////////
void glxGraphicsWindow::
release_gsg() {
  if (_gsg != (GraphicsStateGuardian *)NULL) {
    glXMakeCurrent(_display, _xwindow, _context);
    GraphicsWindow::release_gsg();
    glXDestroyContext(_display, _context);
    _context = (GLXContext)0;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsWindow::make_current
//       Access: Public, Virtual
//  Description: This function will be called within the draw thread
//               during begin_frame() to ensure the graphics context
//               is ready for drawing.
////////////////////////////////////////////////////////////////////
void glxGraphicsWindow::
make_current() {
  nassertv(_gsg != (GraphicsStateGuardian *)NULL);
  glXMakeCurrent(_display, _xwindow, _context);
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsWindow::begin_flip
//       Access: Public, Virtual
//  Description: This function will be called within the draw thread
//               after end_frame() has been called on all windows, to
//               initiate the exchange of the front and back buffers.
//
//               This should instruct the window to prepare for the
//               flip at the next video sync, but it should not wait.
//
//               We have the two separate functions, begin_flip() and
//               end_flip(), to make it easier to flip all of the
//               windows at the same time.
////////////////////////////////////////////////////////////////////
void glxGraphicsWindow::
begin_flip() {
  if (_gsg != (GraphicsStateGuardian *)NULL) {
    glXMakeCurrent(_display, _xwindow, _context);
    glXSwapBuffers(_display, _xwindow);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsWindow::process_events
//       Access: Public, Virtual
//  Description: Do whatever processing is necessary to ensure that
//               the window responds to user events.  Also, honor any
//               requests recently made via request_properties()
//
//               This function is called only within the window
//               thread.
////////////////////////////////////////////////////////////////////
void glxGraphicsWindow::
process_events() {
  GraphicsWindow::process_events();

  if (_xwindow == (Window)0) {
    return;
  }

  XEvent event;
  while (XCheckWindowEvent(_display, _xwindow, _event_mask, &event)) {
    WindowProperties properties;
    ButtonHandle button;

    switch (event.type) {
    case ReparentNotify:
      break;

    case ConfigureNotify:
      properties.set_size(event.xconfigure.width, event.xconfigure.height);
      system_changed_properties(properties);
      break;

    case ButtonPress:
      // This refers to the mouse buttons.
      button = MouseButton::button(event.xbutton.button - 1);
      _input_devices[0].set_pointer_in_window(event.xbutton.x, event.xbutton.y);
      _input_devices[0].button_down(button);
      break;
      
    case ButtonRelease:
      button = MouseButton::button(event.xbutton.button - 1);
      _input_devices[0].set_pointer_in_window(event.xbutton.x, event.xbutton.y);
      _input_devices[0].button_up(button);
      break;

    case MotionNotify:
      _input_devices[0].set_pointer_in_window(event.xmotion.x, event.xmotion.y);
      break;

    case KeyPress:
      {
        _input_devices[0].set_pointer_in_window(event.xkey.x, event.xkey.y);
        int index = ((event.xkey.state & ShiftMask) != 0) ? 1 : 0;

        // First, get the keystroke, as modified by the shift key.
        KeySym key = XLookupKeysym(&event.xkey, index);
        if (key > 0 && key < 128) {
          // If it's an ASCII key, press it.
          _input_devices[0].keystroke(key);
        }

        // Now get the raw unshifted button.
        ButtonHandle button = get_button(&event.xkey);
        if (button != ButtonHandle::none()) {
          _input_devices[0].button_down(button);
        }
      }
      break;

    case KeyRelease:
      button = get_button(&event.xkey);
      if (button != ButtonHandle::none()) {
        _input_devices[0].button_up(button);
      }
      break;

    case EnterNotify:
      _input_devices[0].set_pointer_in_window(event.xcrossing.x, event.xcrossing.y);
      break;

    case LeaveNotify:
      _input_devices[0].set_pointer_out_of_window();
      break;

    case FocusIn:
      properties.set_foreground(true);
      system_changed_properties(properties);
      break;

    case FocusOut:
      properties.set_foreground(false);
      system_changed_properties(properties);
      break;

    case UnmapNotify:
      properties.set_minimized(true);
      system_changed_properties(properties);
      break;

    case MapNotify:
      properties.set_minimized(false);
      system_changed_properties(properties);
      break;

    case DestroyNotify:
      cerr << "destroy\n";
      break;

    default:
      cerr << "unhandled X event type " << event.type << "\n";
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsWindow::set_properties_now
//       Access: Public, Virtual
//  Description: Applies the requested set of properties to the
//               window, if possible, for instance to request a change
//               in size or minimization status.
//
//               The window properties are applied immediately, rather
//               than waiting until the next frame.  This implies that
//               this method may *only* be called from within the
//               window thread.
//
//               The return value is true if the properties are set,
//               false if they are ignored.  This is mainly useful for
//               derived classes to implement extensions to this
//               function.
////////////////////////////////////////////////////////////////////
void glxGraphicsWindow::
set_properties_now(WindowProperties &properties) {
  GraphicsWindow::set_properties_now(properties);
  if (!properties.is_any_specified()) {
    // The base class has already handled this case.
    return;
  }

  // The window is already open; we are limited to what we can change
  // on the fly.  This appears to be just the window title.
  if (properties.has_title()) {
    WindowProperties wm_properties;
    wm_properties.set_title(properties.get_title());
    _properties.set_title(properties.get_title());
    set_wm_properties(wm_properties);
    properties.clear_title();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsWindow::close_window
//       Access: Protected, Virtual
//  Description: Closes the window right now.  Called from the window
//               thread.
////////////////////////////////////////////////////////////////////
void glxGraphicsWindow::
close_window() {
  if (_xwindow != (Window)0) {
    XDestroyWindow(_display, _xwindow);
    _xwindow = (Window)0;

    // This may be necessary if we just closed the last X window in an
    // application, so the server hears the close request.
    XFlush(_display);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsWindow::open_window
//       Access: Protected, Virtual
//  Description: Opens the window right now.  Called from the window
//               thread.  Returns true if the window is successfully
//               opened, or false if there was a problem.
////////////////////////////////////////////////////////////////////
bool glxGraphicsWindow::
open_window() {
  if (_properties.get_fullscreen()) {
    // We don't support fullscreen windows.
    return false;
  }

  if (!_properties.has_origin()) {
    _properties.set_origin(0, 0);
  }
  if (!_properties.has_size()) {
    _properties.set_size(100, 100);
  }

  glxGraphicsPipe *glx_pipe;
  DCAST_INTO_R(glx_pipe, _pipe, false);
  Window root_window = glx_pipe->get_root();

  if (!choose_visual()) {
    return false;
  }
  setup_colormap();

  _event_mask = 
    ButtonPressMask | ButtonReleaseMask |
    KeyPressMask | KeyReleaseMask |
    EnterWindowMask | LeaveWindowMask |
    PointerMotionMask |
    FocusChangeMask |
    StructureNotifyMask;

  // Initialize window attributes
  XSetWindowAttributes wa;
  wa.background_pixel = XBlackPixel(_display, _screen);
  wa.border_pixel = 0;
  wa.colormap = _colormap;
  wa.event_mask = _event_mask;
  wa.do_not_propagate_mask = 0;

  unsigned long attrib_mask = 
    CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

  _xwindow = XCreateWindow
    (_display, root_window,
     _properties.get_x_origin(), _properties.get_y_origin(),
     _properties.get_x_size(), _properties.get_y_size(),
     0,
     _visual->depth, InputOutput, _visual->visual, 
     attrib_mask, &wa);

  if (_xwindow == (Window)0) {
    glxdisplay_cat.error()
      << "failed to create X window.\n";
    return false;
  }
  set_wm_properties(_properties);

  XMapWindow(_display, _xwindow);

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsWindow::set_wm_properties
//       Access: Private
//  Description: Asks the window manager to set the appropriate
//               properties.  In X, these properties cannot be
//               specified directly by the application; they must be
//               requested via the window manager, which may or may
//               not choose to honor the request.
////////////////////////////////////////////////////////////////////
void glxGraphicsWindow::
set_wm_properties(const WindowProperties &properties) {
  // Name the window if there is a name
  XTextProperty window_name;
  XTextProperty *window_name_p = (XTextProperty *)NULL;
  if (properties.has_title()) {
    char *name = (char *)properties.get_title().c_str();
    if (XStringListToTextProperty(&name, 1, &window_name) != 0) {
      window_name_p = &window_name;
    }
  }

  // Setup size hints
  XSizeHints *size_hints_p = NULL;
  if (properties.has_origin() || properties.has_size()) {
    size_hints_p = XAllocSizeHints();
    if (size_hints_p != (XSizeHints *)NULL) {
      if (properties.has_origin()) {
        size_hints_p->x = properties.get_x_origin();
        size_hints_p->y = properties.get_y_origin();
        size_hints_p->flags |= USPosition;
      }
      if (properties.has_size()) {
        size_hints_p->width = properties.get_x_size();
        size_hints_p->height = properties.get_y_size();
        size_hints_p->flags |= USSize;
      }
    }
  }

  // Setup window manager hints
  XWMHints *wm_hints_p = NULL;
  wm_hints_p = XAllocWMHints();
  if (wm_hints_p != (XWMHints *)NULL) {
    if (properties.has_minimized() && properties.get_minimized()) {
      wm_hints_p->initial_state = IconicState;
    } else {
      wm_hints_p->initial_state = NormalState;
    }
    wm_hints_p->flags = StateHint;
  }

  // If we asked for a window without a border, there's no good way to
  // arrange that.  It completely depends on the user's window manager
  // of choice.  Instead, we'll totally punt and just set the window's
  // Class to "Undecorated", and let the user configure his/her window
  // manager not to put a border around windows of this class.
  XClassHint *class_hints_p = NULL;
  if (properties.has_undecorated() && properties.get_undecorated()) {
    class_hints_p = XAllocClassHint();
    class_hints_p->res_class = "Undecorated";
  }

  XSetWMProperties(_display, _xwindow, window_name_p, window_name_p,
                   NULL, 0, size_hints_p, wm_hints_p, class_hints_p);

  if (size_hints_p != (XSizeHints *)NULL) {
    XFree(size_hints_p);
  }
  if (wm_hints_p != (XWMHints *)NULL) {
    XFree(wm_hints_p);
  }
  if (class_hints_p != (XClassHint *)NULL) {
    XFree(class_hints_p);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsWindow::try_for_visual
//       Access: Private
//  Description: Attempt to get the requested visual, if it is
//               available.  It's just a wrapper around
//               glXChooseVisual().  It returns the visual information
//               if possible, or NULL if it is not.
////////////////////////////////////////////////////////////////////
XVisualInfo *glxGraphicsWindow::
try_for_visual(int framebuffer_mode,
               int want_depth_bits, int want_color_bits) const {
  static const int max_attrib_list = 32;
  int attrib_list[max_attrib_list];
  int n=0;

  glxdisplay_cat.debug()
    << "Trying for visual with: RGB(" << want_color_bits << ")";

  int want_color_component_bits;
  if (framebuffer_mode & WindowProperties::FM_alpha) {
    want_color_component_bits = max(want_color_bits / 4, 1);
  } else {
    want_color_component_bits = max(want_color_bits / 3, 1);
  }

  attrib_list[n++] = GLX_RGBA;
  attrib_list[n++] = GLX_RED_SIZE;
  attrib_list[n++] = want_color_component_bits;
  attrib_list[n++] = GLX_GREEN_SIZE;
  attrib_list[n++] = want_color_component_bits;
  attrib_list[n++] = GLX_BLUE_SIZE;
  attrib_list[n++] = want_color_component_bits;

  if (framebuffer_mode & WindowProperties::FM_alpha) {
    glxdisplay_cat.debug(false) << " ALPHA";
    attrib_list[n++] = GLX_ALPHA_SIZE;
    attrib_list[n++] = want_color_component_bits;
  }
  if (framebuffer_mode & WindowProperties::FM_double_buffer) {
    glxdisplay_cat.debug(false) << " DOUBLEBUFFER";
    attrib_list[n++] = GLX_DOUBLEBUFFER;
  }
  if (framebuffer_mode & WindowProperties::FM_stereo) {
    glxdisplay_cat.debug(false) << " STEREO";
    attrib_list[n++] = GLX_STEREO;
  }
  if (framebuffer_mode & WindowProperties::FM_depth) {
    glxdisplay_cat.debug(false) << " DEPTH(" << want_depth_bits << ")";
    attrib_list[n++] = GLX_DEPTH_SIZE;
    attrib_list[n++] = want_depth_bits;
  }
  if (framebuffer_mode & WindowProperties::FM_stencil) {
    glxdisplay_cat.debug(false) << " STENCIL";
    attrib_list[n++] = GLX_STENCIL_SIZE;
    attrib_list[n++] = 1;
  }
  if (framebuffer_mode & WindowProperties::FM_accum) {
    glxdisplay_cat.debug(false) << " ACCUM";
    attrib_list[n++] = GLX_ACCUM_RED_SIZE;
    attrib_list[n++] = want_color_component_bits;
    attrib_list[n++] = GLX_ACCUM_GREEN_SIZE;
    attrib_list[n++] = want_color_component_bits;
    attrib_list[n++] = GLX_ACCUM_BLUE_SIZE;
    attrib_list[n++] = want_color_component_bits;
    if (framebuffer_mode & WindowProperties::FM_alpha) {
      attrib_list[n++] = GLX_ACCUM_ALPHA_SIZE;
      attrib_list[n++] = want_color_component_bits;
    }
  }
#if defined(GLX_VERSION_1_1) && defined(GLX_SGIS_multisample)
  if (framebuffer_mode & WindowProperties::FM_multisample) {
    glxdisplay_cat.debug(false) << " MULTISAMPLE";
    attrib_list[n++] = GLX_SAMPLES_SGIS;
    // We decide 4 is minimum number of samples
    attrib_list[n++] = 4;
  }
#endif

  // Terminate the list
  nassertr(n < max_attrib_list, NULL);
  attrib_list[n] = (int)None;

  XVisualInfo *vinfo = glXChooseVisual(_display, _screen, attrib_list);

  if (glxdisplay_cat.is_debug()) {
    if (vinfo != NULL) {
      glxdisplay_cat.debug(false) << ", match found!\n";
    } else {
      glxdisplay_cat.debug(false) << ", no match.\n";
    }
  }

  return vinfo;
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsWindow::choose visual
//       Access: Private
//  Description: Selects an appropriate X visual for the window based
//               on the window properties.  Returns true if
//               successful, false otherwise.
//
//               Initializes _visual, and modifies _properties
//               according to the actual visual chosen.
////////////////////////////////////////////////////////////////////
bool glxGraphicsWindow::
choose_visual() {
  int framebuffer_mode = 0;
  int want_depth_bits = 0;
  int want_color_bits = 0;

  if (_properties.has_framebuffer_mode()) {
    framebuffer_mode = _properties.get_framebuffer_mode();
  }

  if (_properties.has_depth_bits()) {
    want_depth_bits = _properties.get_depth_bits();
  }

  if (_properties.has_color_bits()) {
    want_color_bits = _properties.get_color_bits();
  }

  /*
  if (framebuffer_mode & WindowProperties::FM_multisample) {
    if (!glx_supports("GLX_SGIS_multisample")) {
      glxdisplay_cat.info()
        << "multisample not supported by this glx implementation.\n";
      framebuffer_mode &= ~WindowProperties::FM_multisample;
    }
  }
  */

  _visual = try_for_visual(framebuffer_mode, want_depth_bits, want_color_bits);

  // This is the severity level at which we'll report the details of
  // the visual we actually do find.  Normally, it's debug-level
  // information: we don't care about that much detail.
  NotifySeverity show_visual_severity = NS_debug;

  if (_visual == NULL) {
    glxdisplay_cat.info()
      << "glxGraphicsWindow::choose_visual() - visual with requested\n"
      << "   capabilities not found; trying for lesser visual.\n";

    // If we're unable to get the visual we asked for, however, we
    // probably *do* care to know the details about what we actually
    // got, even if we don't have debug mode set.  So we'll report the
    // visual at a higher level.
    show_visual_severity = NS_info;

    bool special_size_request =
      (want_depth_bits != 1 || want_color_bits != 1);

    // We try to be smart about choosing a close match for the visual.
    // First, we'll eliminate some of the more esoteric options one at
    // a time, then two at a time, and finally we'll try just the bare
    // minimum.

    if (special_size_request) {
      // Actually, first we'll eliminate all of the minimum sizes, to
      // try to open a window with all of the requested options, but
      // maybe not as many bits in some options as we'd like.
      _visual = try_for_visual(framebuffer_mode, 1, 1);
    }

    if (_visual == NULL) {
      // Ok, not good enough.  Now try to eliminate options, but keep
      // as many bits as we asked for.

      // This array keeps the bitmasks of options that we pull out of
      // the requested framebuffer_mode, in order.

      static const int strip_properties[] = {
        // One esoteric option removed.
        WindowProperties::FM_multisample,
        WindowProperties::FM_stencil,
        WindowProperties::FM_accum,
        WindowProperties::FM_alpha,
        WindowProperties::FM_stereo,

        // Two esoteric options removed.
        WindowProperties::FM_stencil | WindowProperties::FM_multisample,
        WindowProperties::FM_accum | WindowProperties::FM_multisample,
        WindowProperties::FM_alpha | WindowProperties::FM_multisample,
        WindowProperties::FM_stereo | WindowProperties::FM_multisample,
        WindowProperties::FM_stencil | WindowProperties::FM_accum,
        WindowProperties::FM_alpha | WindowProperties::FM_stereo,
        WindowProperties::FM_stencil | WindowProperties::FM_accum | WindowProperties::FM_multisample,
        WindowProperties::FM_alpha | WindowProperties::FM_stereo | WindowProperties::FM_multisample,

        // All esoteric options removed.
        WindowProperties::FM_stencil | WindowProperties::FM_accum | WindowProperties::FM_alpha | WindowProperties::FM_stereo | WindowProperties::FM_multisample,

        // All esoteric options, plus some we'd really really prefer,
        // removed.
        WindowProperties::FM_stencil | WindowProperties::FM_accum | WindowProperties::FM_alpha | WindowProperties::FM_stereo | WindowProperties::FM_multisample | WindowProperties::FM_double_buffer,

        // A zero marks the end of the array.
        0
      };

      pset<int> tried_masks;
      tried_masks.insert(framebuffer_mode);

      int i;
      for (i = 0; _visual == NULL && strip_properties[i] != 0; i++) {
        int new_framebuffer_mode = framebuffer_mode & ~strip_properties[i];
        if (tried_masks.insert(new_framebuffer_mode).second) {
          _visual = try_for_visual(new_framebuffer_mode, want_depth_bits,
                                   want_color_bits);
        }
      }

      if (special_size_request) {
        tried_masks.clear();
        tried_masks.insert(framebuffer_mode);

        if (_visual == NULL) {
          // Try once more, this time eliminating all of the size
          // requests.
          for (i = 0; _visual == NULL && strip_properties[i] != 0; i++) {
            int new_framebuffer_mode = framebuffer_mode & ~strip_properties[i];
            if (tried_masks.insert(new_framebuffer_mode).second) {
              _visual = try_for_visual(new_framebuffer_mode, 1, 1);
            }
          }
        }
      }

      if (_visual == NULL) {
        // Here's our last-ditch desparation attempt: give us any GLX
        // visual at all!
        _visual = try_for_visual(0, 1, 1);
      }

      if (_visual == NULL) {
        glxdisplay_cat.error()
          << "Could not get any GLX visual.\n";
        return false;
      }
    }
  }

  glxdisplay_cat.info()
    << "Got visual 0x" << hex << (int)_visual->visualid << dec << ".\n";

  // Now update our frambuffer_mode and bit depth appropriately.
  int render_mode, double_buffer, stereo, red_size, green_size, blue_size,
    alpha_size, ared_size, agreen_size, ablue_size, aalpha_size,
    depth_size, stencil_size;
  
  glXGetConfig(_display, _visual, GLX_RGBA, &render_mode);
  glXGetConfig(_display, _visual, GLX_DOUBLEBUFFER, &double_buffer);
  glXGetConfig(_display, _visual, GLX_STEREO, &stereo);
  glXGetConfig(_display, _visual, GLX_RED_SIZE, &red_size);
  glXGetConfig(_display, _visual, GLX_GREEN_SIZE, &green_size);
  glXGetConfig(_display, _visual, GLX_BLUE_SIZE, &blue_size);
  glXGetConfig(_display, _visual, GLX_ALPHA_SIZE, &alpha_size);
  glXGetConfig(_display, _visual, GLX_ACCUM_RED_SIZE, &ared_size);
  glXGetConfig(_display, _visual, GLX_ACCUM_GREEN_SIZE, &agreen_size);
  glXGetConfig(_display, _visual, GLX_ACCUM_BLUE_SIZE, &ablue_size);
  glXGetConfig(_display, _visual, GLX_ACCUM_ALPHA_SIZE, &aalpha_size);
  glXGetConfig(_display, _visual, GLX_DEPTH_SIZE, &depth_size);
  glXGetConfig(_display, _visual, GLX_STENCIL_SIZE, &stencil_size);

  framebuffer_mode = 0;
  if (double_buffer) {
    framebuffer_mode |= WindowProperties::FM_double_buffer;
  }
  if (stereo) {
    framebuffer_mode |= WindowProperties::FM_stereo;
  }
  if (!render_mode) {
    framebuffer_mode |= WindowProperties::FM_index;
  }
  if (stencil_size != 0) {
    framebuffer_mode |= WindowProperties::FM_stencil;
  }
  if (depth_size != 0) {
    framebuffer_mode |= WindowProperties::FM_depth;
  }
  if (alpha_size != 0) {
    framebuffer_mode |= WindowProperties::FM_alpha;
  }
  if (ared_size + agreen_size + ablue_size != 0) {
    framebuffer_mode |= WindowProperties::FM_accum;
  }

  _properties.set_framebuffer_mode(framebuffer_mode);
  _properties.set_color_bits(red_size + green_size + blue_size + alpha_size);
  _properties.set_depth_bits(depth_size);

  if (glxdisplay_cat.is_on(show_visual_severity)) {
    glxdisplay_cat.out(show_visual_severity)
      << "GLX Visual Info (# bits of each):" << endl
      << " RGBA: " << red_size << " " << green_size << " " << blue_size
      << " " << alpha_size << endl
      << " Accum RGBA: " << ared_size << " " << agreen_size << " "
      << ablue_size << " " << aalpha_size << endl
      << " Depth: " << depth_size << endl
      << " Stencil: " << stencil_size << endl
      << " DoubleBuffer? " << double_buffer << endl
      << " Stereo? " << stereo << endl;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsWindow::setup_colormap
//       Access: Private
//  Description: Allocates a colormap appropriate to the visual and
//               stores in in the _colormap method.
////////////////////////////////////////////////////////////////////
void glxGraphicsWindow::
setup_colormap() {
  glxGraphicsPipe *glx_pipe;
  DCAST_INTO_V(glx_pipe, _pipe);
  Window root_window = glx_pipe->get_root();

  int visual_class = _visual->c_class;
  int rc, is_rgb;

  switch (visual_class) {
    case PseudoColor:
      rc = glXGetConfig(_display, _visual, GLX_RGBA, &is_rgb);
      if (rc == 0 && is_rgb) {
        glxdisplay_cat.warning()
          << "mesa pseudocolor not supported.\n";
        // this is a terrible terrible hack, but it seems to work
        _colormap = (Colormap)0;

      } else {
        _colormap = XCreateColormap(_display, root_window,
                                    _visual->visual, AllocAll);
      }
      break;
    case TrueColor:
    case DirectColor:
      _colormap = XCreateColormap(_display, root_window,
                                  _visual->visual, AllocNone);
      break;
    case StaticColor:
    case StaticGray:
    case GrayScale:
      _colormap = XCreateColormap(_display, root_window,
                                  _visual->visual, AllocNone);
      break;
    default:
      glxdisplay_cat.error()
        << "Could not allocate a colormap for visual class "
        << visual_class << ".\n";
      break;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: glxGraphicsWindow::get_button
//       Access: Private
//  Description: Returns the Panda ButtonHandle corresponding to the
//               keyboard button indicated by the given key event.
////////////////////////////////////////////////////////////////////
ButtonHandle glxGraphicsWindow::
get_button(XKeyEvent *key_event) {
  KeySym key = XLookupKeysym(key_event, 0);

  switch (key) {
  case XK_BackSpace:
    return KeyboardButton::backspace();
  case XK_Tab:
    return KeyboardButton::tab();
  case XK_Return:
    return KeyboardButton::enter();
  case XK_Escape:
    return KeyboardButton::escape();
  case XK_space:
    return KeyboardButton::space();
  case XK_exclam:
    return KeyboardButton::ascii_key('!');
  case XK_quotedbl:
    return KeyboardButton::ascii_key('"');
  case XK_numbersign:
    return KeyboardButton::ascii_key('#');
  case XK_dollar:
    return KeyboardButton::ascii_key('$');
  case XK_percent:
    return KeyboardButton::ascii_key('%');
  case XK_ampersand:
    return KeyboardButton::ascii_key('&');
  case XK_apostrophe: // == XK_quoteright
    return KeyboardButton::ascii_key('\'');
  case XK_parenleft:
    return KeyboardButton::ascii_key('(');
  case XK_parenright:
    return KeyboardButton::ascii_key(')');
  case XK_asterisk:
    return KeyboardButton::ascii_key('*');
  case XK_plus:
    return KeyboardButton::ascii_key('+');
  case XK_comma:
    return KeyboardButton::ascii_key(',');
  case XK_minus:
    return KeyboardButton::ascii_key('-');
  case XK_period:
    return KeyboardButton::ascii_key('.');
  case XK_slash:
    return KeyboardButton::ascii_key('/');
  case XK_0:
    return KeyboardButton::ascii_key('0');
  case XK_1:
    return KeyboardButton::ascii_key('1');
  case XK_2:
    return KeyboardButton::ascii_key('2');
  case XK_3:
    return KeyboardButton::ascii_key('3');
  case XK_4:
    return KeyboardButton::ascii_key('4');
  case XK_5:
    return KeyboardButton::ascii_key('5');
  case XK_6:
    return KeyboardButton::ascii_key('6');
  case XK_7:
    return KeyboardButton::ascii_key('7');
  case XK_8:
    return KeyboardButton::ascii_key('8');
  case XK_9:
    return KeyboardButton::ascii_key('9');
  case XK_colon:
    return KeyboardButton::ascii_key(':');
  case XK_semicolon:
    return KeyboardButton::ascii_key(';');
  case XK_less:
    return KeyboardButton::ascii_key('<');
  case XK_equal:
    return KeyboardButton::ascii_key('=');
  case XK_greater:
    return KeyboardButton::ascii_key('>');
  case XK_question:
    return KeyboardButton::ascii_key('?');
  case XK_at:
    return KeyboardButton::ascii_key('@');
  case XK_A:
    return KeyboardButton::ascii_key('A');
  case XK_B:
    return KeyboardButton::ascii_key('B');
  case XK_C:
    return KeyboardButton::ascii_key('C');
  case XK_D:
    return KeyboardButton::ascii_key('D');
  case XK_E:
    return KeyboardButton::ascii_key('E');
  case XK_F:
    return KeyboardButton::ascii_key('F');
  case XK_G:
    return KeyboardButton::ascii_key('G');
  case XK_H:
    return KeyboardButton::ascii_key('H');
  case XK_I:
    return KeyboardButton::ascii_key('I');
  case XK_J:
    return KeyboardButton::ascii_key('J');
  case XK_K:
    return KeyboardButton::ascii_key('K');
  case XK_L:
    return KeyboardButton::ascii_key('L');
  case XK_M:
    return KeyboardButton::ascii_key('M');
  case XK_N:
    return KeyboardButton::ascii_key('N');
  case XK_O:
    return KeyboardButton::ascii_key('O');
  case XK_P:
    return KeyboardButton::ascii_key('P');
  case XK_Q:
    return KeyboardButton::ascii_key('Q');
  case XK_R:
    return KeyboardButton::ascii_key('R');
  case XK_S:
    return KeyboardButton::ascii_key('S');
  case XK_T:
    return KeyboardButton::ascii_key('T');
  case XK_U:
    return KeyboardButton::ascii_key('U');
  case XK_V:
    return KeyboardButton::ascii_key('V');
  case XK_W:
    return KeyboardButton::ascii_key('W');
  case XK_X:
    return KeyboardButton::ascii_key('X');
  case XK_Y:
    return KeyboardButton::ascii_key('Y');
  case XK_Z:
    return KeyboardButton::ascii_key('Z');
  case XK_bracketleft:
    return KeyboardButton::ascii_key('[');
  case XK_backslash:
    return KeyboardButton::ascii_key('\\');
  case XK_bracketright:
    return KeyboardButton::ascii_key(']');
  case XK_asciicircum:
    return KeyboardButton::ascii_key('^');
  case XK_underscore:
    return KeyboardButton::ascii_key('_');
  case XK_grave: // == XK_quoteleft
    return KeyboardButton::ascii_key('`');
  case XK_a:
    return KeyboardButton::ascii_key('a');
  case XK_b:
    return KeyboardButton::ascii_key('b');
  case XK_c:
    return KeyboardButton::ascii_key('c');
  case XK_d:
    return KeyboardButton::ascii_key('d');
  case XK_e:
    return KeyboardButton::ascii_key('e');
  case XK_f:
    return KeyboardButton::ascii_key('f');
  case XK_g:
    return KeyboardButton::ascii_key('g');
  case XK_h:
    return KeyboardButton::ascii_key('h');
  case XK_i:
    return KeyboardButton::ascii_key('i');
  case XK_j:
    return KeyboardButton::ascii_key('j');
  case XK_k:
    return KeyboardButton::ascii_key('k');
  case XK_l:
    return KeyboardButton::ascii_key('l');
  case XK_m:
    return KeyboardButton::ascii_key('m');
  case XK_n:
    return KeyboardButton::ascii_key('n');
  case XK_o:
    return KeyboardButton::ascii_key('o');
  case XK_p:
    return KeyboardButton::ascii_key('p');
  case XK_q:
    return KeyboardButton::ascii_key('q');
  case XK_r:
    return KeyboardButton::ascii_key('r');
  case XK_s:
    return KeyboardButton::ascii_key('s');
  case XK_t:
    return KeyboardButton::ascii_key('t');
  case XK_u:
    return KeyboardButton::ascii_key('u');
  case XK_v:
    return KeyboardButton::ascii_key('v');
  case XK_w:
    return KeyboardButton::ascii_key('w');
  case XK_x:
    return KeyboardButton::ascii_key('x');
  case XK_y:
    return KeyboardButton::ascii_key('y');
  case XK_z:
    return KeyboardButton::ascii_key('z');
  case XK_braceleft:
    return KeyboardButton::ascii_key('{');
  case XK_bar:
    return KeyboardButton::ascii_key('|');
  case XK_braceright:
    return KeyboardButton::ascii_key('}');
  case XK_asciitilde:
    return KeyboardButton::ascii_key('~');
  case XK_F1:
    return KeyboardButton::f1();
  case XK_F2:
    return KeyboardButton::f2();
  case XK_F3:
    return KeyboardButton::f3();
  case XK_F4:
    return KeyboardButton::f4();
  case XK_F5:
    return KeyboardButton::f5();
  case XK_F6:
    return KeyboardButton::f6();
  case XK_F7:
    return KeyboardButton::f7();
  case XK_F8:
    return KeyboardButton::f8();
  case XK_F9:
    return KeyboardButton::f9();
  case XK_F10:
    return KeyboardButton::f10();
  case XK_F11:
    return KeyboardButton::f11();
  case XK_F12:
    return KeyboardButton::f12();
  case XK_KP_Left:
  case XK_Left:
    return KeyboardButton::left();
  case XK_KP_Up:
  case XK_Up:
    return KeyboardButton::up();
  case XK_KP_Right:
  case XK_Right:
    return KeyboardButton::right();
  case XK_KP_Down:
  case XK_Down:
    return KeyboardButton::down();
  case XK_KP_Prior:
  case XK_Prior:
    return KeyboardButton::page_up();
  case XK_KP_Next:
  case XK_Next:
    return KeyboardButton::page_down();
  case XK_KP_Home:
  case XK_Home:
    return KeyboardButton::home();
  case XK_KP_End:
  case XK_End:
    return KeyboardButton::end();
  case XK_KP_Insert:
  case XK_Insert:
    return KeyboardButton::insert();
  case XK_KP_Delete:
  case XK_Delete:
    return KeyboardButton::del();
  case XK_Shift_L:
  case XK_Shift_R:
    return KeyboardButton::shift();
  case XK_Control_L:
  case XK_Control_R:
    return KeyboardButton::control();
  case XK_Alt_L:
  case XK_Alt_R:
    return KeyboardButton::alt();
  case XK_Meta_L:
  case XK_Meta_R:
    return KeyboardButton::meta();
  case XK_Caps_Lock:
    return KeyboardButton::caps_lock();
  case XK_Shift_Lock:
    return KeyboardButton::shift_lock();
  }

  return ButtonHandle::none();
}
