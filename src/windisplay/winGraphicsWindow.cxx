/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file winGraphicsWindow.cxx
 * @author drose
 * @date 2002-12-20
 */

#include "winGraphicsWindow.h"
#include "config_windisplay.h"
#include "winGraphicsPipe.h"

#include "graphicsPipe.h"
#include "keyboardButton.h"
#include "mouseButton.h"
#include "clockObject.h"
#include "config_putil.h"
#include "throw_event.h"
#include "nativeWindowHandle.h"

#include <tchar.h>

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

#ifndef WM_TOUCH
#define WM_TOUCH 0x0240
#endif

#if WINVER < 0x0601
// Not used on Windows XP, but we still need to define it.
#define TOUCH_COORD_TO_PIXEL(l) ((l) / 100)

DECLARE_HANDLE(HTOUCHINPUT);
#endif

TypeHandle WinGraphicsWindow::_type_handle;
TypeHandle WinGraphicsWindow::WinWindowHandle::_type_handle;

WinGraphicsWindow::WindowHandles WinGraphicsWindow::_window_handles;
WinGraphicsWindow *WinGraphicsWindow::_creating_window = NULL;

WinGraphicsWindow *WinGraphicsWindow::_cursor_window = NULL;
bool WinGraphicsWindow::_cursor_hidden = false;

RECT WinGraphicsWindow::_mouse_unconfined_cliprect;

// These are used to save the previous state of the fancy Win2000 effects that
// interfere with rendering when the mouse wanders into a window's client
// area.
bool WinGraphicsWindow::_got_saved_params = false;
int WinGraphicsWindow::_saved_mouse_trails;
BOOL WinGraphicsWindow::_saved_cursor_shadow;
BOOL WinGraphicsWindow::_saved_mouse_vanish;

WinGraphicsWindow::IconFilenames WinGraphicsWindow::_icon_filenames;
WinGraphicsWindow::IconFilenames WinGraphicsWindow::_cursor_filenames;

WinGraphicsWindow::WindowClasses WinGraphicsWindow::_window_classes;
int WinGraphicsWindow::_window_class_index = 0;

static const char * const errorbox_title = "Panda3D Error";

// These static variables contain pointers to the touch input functions, which
// are dynamically extracted from USER32.DLL
typedef BOOL (WINAPI *PFN_REGISTERTOUCHWINDOW)(IN HWND hWnd, IN ULONG ulFlags);
typedef BOOL (WINAPI *PFN_GETTOUCHINPUTINFO)(IN HTOUCHINPUT hTouchInput, IN UINT cInputs, OUT PTOUCHINPUT pInputs, IN int cbSize);
typedef BOOL (WINAPI *PFN_CLOSETOUCHINPUTHANDLE)(IN HTOUCHINPUT hTouchInput);

static PFN_REGISTERTOUCHWINDOW pRegisterTouchWindow = 0;
static PFN_GETTOUCHINPUTINFO pGetTouchInputInfo = 0;
static PFN_CLOSETOUCHINPUTHANDLE pCloseTouchInputHandle = 0;

/**
 *
 */
WinGraphicsWindow::
WinGraphicsWindow(GraphicsEngine *engine, GraphicsPipe *pipe,
                  const string &name,
                  const FrameBufferProperties &fb_prop,
                  const WindowProperties &win_prop,
                  int flags,
                  GraphicsStateGuardian *gsg,
                  GraphicsOutput *host) :
  GraphicsWindow(engine, pipe, name, fb_prop, win_prop, flags, gsg, host)
{
  initialize_input_devices();
  _hWnd = (HWND)0;
  _ime_open = false;
  _ime_active = false;
  _tracking_mouse_leaving = false;
  _cursor = 0;
  _lost_keypresses = false;
  _lshift_down = false;
  _rshift_down = false;
  _lcontrol_down = false;
  _rcontrol_down = false;
  _lalt_down = false;
  _ralt_down = false;
  _hparent = NULL;
  _num_touches = 0;
}

/**
 *
 */
WinGraphicsWindow::
~WinGraphicsWindow() {
  if (_window_handle != (WindowHandle *)NULL) {
    DCAST(WinWindowHandle, _window_handle)->clear_window();
  }
}

/**
 * Forces the pointer to the indicated position within the window, if
 * possible.
 *
 * Returns true if successful, false on failure.  This may fail if the mouse
 * is not currently within the window, or if the API doesn't support this
 * operation.
 */
bool WinGraphicsWindow::
move_pointer(int device, int x, int y) {
  // First, indicate that the IME is no longer active, so that it won't send
  // the string through WM_IME_COMPOSITION.  But we still leave _ime_open
  // true, so that it also won't send the string through WM_CHAR.
  _ime_active = false;

  // Note: this is not thread-safe; it should be called only from App.
  // Probably not an issue.
  if (device == 0) {
    // Move the system mouse pointer.
    if (!_properties.get_foreground() )
      // !_input_devices[0].get_pointer().get_in_window())
      {
        // If the window doesn't have input focus, or the mouse isn't
        // currently within the window, forget it.
        return false;
      }

    RECT view_rect;
    get_client_rect_screen(_hWnd, &view_rect);

    SetCursorPos(view_rect.left + x, view_rect.top + y);
    _input_devices[0].set_pointer_in_window(x, y);
    return true;
  } else {
    // Move a raw mouse.
    if ((device < 1)||(device >= (int)_input_devices.size())) {
      return false;
    }
    _input_devices[device].set_pointer_in_window(x, y);
    return true;
  }
}

/**
 * Forces the ime window to close, if any
 *
 */
void WinGraphicsWindow::
close_ime() {
  // Check if the ime window is open
  if (!_ime_open)
    return;

  HIMC hIMC = ImmGetContext(_hWnd);
  if (hIMC != 0) {
    if (!ImmSetOpenStatus(hIMC, false)) {
      windisplay_cat.debug() << "ImmSetOpenStatus failed\n";
    }
    ImmReleaseContext(_hWnd, hIMC);
  }
  _ime_open = false;

  windisplay_cat.debug() << "success: closed ime window\n";
  return;
}

/**
 * This function will be called within the draw thread after end_frame() has
 * been called on all windows, to initiate the exchange of the front and back
 * buffers.
 *
 * This should instruct the window to prepare for the flip at the next video
 * sync, but it should not wait.
 *
 * We have the two separate functions, begin_flip() and end_flip(), to make it
 * easier to flip all of the windows at the same time.
 */
void WinGraphicsWindow::
begin_flip() {
}

/**
 * Do whatever processing is necessary to ensure that the window responds to
 * user events.  Also, honor any requests recently made via
 * request_properties()
 *
 * This function is called only within the window thread.
 */
void WinGraphicsWindow::
process_events() {
  GraphicsWindow::process_events();

  // We can't treat the message loop specially just because the window is
  // minimized, because we might be reading messages queued up for some other
  // window, which is not minimized.
  /*
  if (!_window_active) {
      // Get 1 msg at a time until no more are left and we block and sleep, or
      // message changes _return_control_to_app or !_window_active status

      while(!_window_active && (!_return_control_to_app)) {
          process_1_event();
      }
      _return_control_to_app = false;

  } else
  */

  MSG msg;

  // Handle all the messages on the queue in a row.  Some of these might be
  // for another window, but they will get dispatched appropriately.
  while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
    process_1_event();
  }
}

/**
 * Applies the requested set of properties to the window, if possible, for
 * instance to request a change in size or minimization status.
 *
 * The window properties are applied immediately, rather than waiting until
 * the next frame.  This implies that this method may *only* be called from
 * within the window thread.
 *
 * The properties that have been applied are cleared from the structure by
 * this function; so on return, whatever remains in the properties structure
 * are those that were unchanged for some reason (probably because the
 * underlying interface does not support changing that property on an open
 * window).
 */
void WinGraphicsWindow::
set_properties_now(WindowProperties &properties) {
  GraphicsWindow::set_properties_now(properties);
  if (!properties.is_any_specified()) {
    // The base class has already handled this case.
    return;
  }

  if (properties.has_title()) {
    string title = properties.get_title();
    _properties.set_title(title);
    TextEncoder encoder;
    wstring title_w = encoder.decode_text(title);
    SetWindowTextW(_hWnd, title_w.c_str());
    properties.clear_title();
  }

  if (properties.has_icon_filename()) {
    HICON icon = get_icon(properties.get_icon_filename());
    if (icon != 0) {
      ::SendMessage(_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)icon);
      ::SendMessage(_hWnd, WM_SETICON, ICON_BIG, (LPARAM)icon);

      _properties.set_icon_filename(properties.get_icon_filename());
      properties.clear_icon_filename();
    }
  }

  if (properties.has_cursor_hidden()) {
    bool hide_cursor = properties.get_cursor_hidden();
    _properties.set_cursor_hidden(hide_cursor);
    if (_cursor_window == this) {
      hide_or_show_cursor(hide_cursor);
    }

    properties.clear_cursor_hidden();
  }

  if (properties.has_cursor_filename()) {
    Filename filename = properties.get_cursor_filename();
    _properties.set_cursor_filename(filename);

    _cursor = get_cursor(filename);
    if (_cursor == 0) {
      _cursor = LoadCursor(NULL, IDC_ARROW);
    }

    if (_cursor_window == this) {
      SetCursor(_cursor);
    }

    properties.clear_cursor_filename();
  }

  if (properties.has_z_order()) {
    WindowProperties::ZOrder last_z_order = _properties.get_z_order();
    _properties.set_z_order(properties.get_z_order());
    adjust_z_order(last_z_order, properties.get_z_order());

    properties.clear_z_order();
  }

  if (properties.has_foreground() && properties.get_foreground()) {
    if (!SetActiveWindow(_hWnd)) {
      windisplay_cat.warning()
        << "SetForegroundWindow() failed!\n";
    } else {
      _properties.set_foreground(true);
    }

    properties.clear_foreground();
  }

  if (properties.has_minimized()) {
    if (_properties.get_minimized() != properties.get_minimized()) {
      if (properties.get_minimized()) {
        ShowWindow(_hWnd, SW_MINIMIZE);
      } else {
        ShowWindow(_hWnd, SW_RESTORE);
      }
      _properties.set_minimized(properties.get_minimized());
      _properties.set_foreground(!properties.get_minimized());
    }
    properties.clear_minimized();
  }

  if (properties.has_fullscreen()) {
    if (properties.get_fullscreen() && !is_fullscreen()) {
      if (do_fullscreen_switch()){
        _properties.set_fullscreen(true);
        properties.clear_fullscreen();
      } else {
        windisplay_cat.warning()
          << "Switching to fullscreen mode failed!\n";
      }
    } else if (!properties.get_fullscreen() && is_fullscreen()){
      if (do_windowed_switch()){
        _properties.set_fullscreen(false);
        properties.clear_fullscreen();
      } else {
        windisplay_cat.warning()
          << "Switching to windowed mode failed!\n";
      }
    }
  }

  if (properties.has_mouse_mode()) {
    if (properties.get_mouse_mode() != _properties.get_mouse_mode()) {
      switch (properties.get_mouse_mode()) {
      case WindowProperties::M_absolute:
      case WindowProperties::M_relative:    // not implemented, treat as absolute

        if (_properties.get_mouse_mode() == WindowProperties::M_confined) {
          ClipCursor(NULL);
          windisplay_cat.info() << "Unconfining cursor from window\n";
        }
        _properties.set_mouse_mode(WindowProperties::M_absolute);
        break;

      case WindowProperties::M_confined:
        {
          RECT clip;

          if (!GetWindowRect(_hWnd, &clip)) {
            windisplay_cat.warning()
                << "GetWindowRect() failed in set_properties_now.  Cannot confine cursor.\n";
          } else {
            windisplay_cat.info()
                    << "ClipCursor() to " << clip.left << "," << clip.top << " to "
                    << clip.right << "," << clip.bottom << endl;

            GetClipCursor(&_mouse_unconfined_cliprect);
            if (!ClipCursor(&clip)) {
              windisplay_cat.warning()
                      << "ClipCursor() failed in set_properties_now.  Ignoring.\n";
            } else {
              _properties.set_mouse_mode(WindowProperties::M_confined);
              windisplay_cat.info() << "Confining cursor to window\n";
            }
          }
        }
        break;
      }
    }
    properties.clear_mouse_mode();
  }

}

/**
 * To be called at the end of the frame, after the window has successfully
 * been drawn and is ready to be flipped (if appropriate).
 */
void WinGraphicsWindow::
trigger_flip() {
  GraphicsWindow::trigger_flip();

  if (!get_unexposed_draw()) {
    // Now that we've drawn or whatever, invalidate the rectangle so we won't
    // redraw again until we get the WM_PAINT message.

    InvalidateRect(_hWnd, NULL, FALSE);
    _got_expose_event = false;

    if (windisplay_cat.is_spam()) {
      windisplay_cat.spam()
        << "InvalidateRect: " << this << "\n";
    }
  }
}

/**
 * Closes the window right now.  Called from the window thread.
 */
void WinGraphicsWindow::
close_window() {
  set_cursor_out_of_window();
  DestroyWindow(_hWnd);

  if (is_fullscreen()) {
    // revert to default display mode.
    do_fullscreen_disable();
  }

  // Remove the window handle from our global map.
  _window_handles.erase(_hWnd);
  _hWnd = (HWND)0;

  GraphicsWindow::close_window();
}

/**
 * Opens the window right now.  Called from the window thread.  Returns true
 * if the window is successfully opened, or false if there was a problem.
 */
bool WinGraphicsWindow::
open_window() {
  if (_properties.has_cursor_filename()) {
    _cursor = get_cursor(_properties.get_cursor_filename());
  }
  if (_cursor == 0) {
    _cursor = LoadCursor(NULL, IDC_ARROW);
  }
  bool want_foreground = (!_properties.has_foreground() || _properties.get_foreground());
  bool want_minimized = (_properties.has_minimized() && _properties.get_minimized()) && !want_foreground;

  HWND old_foreground_window = GetForegroundWindow();

  // Store the current window pointer in _creating_window, so we can call
  // CreateWindow() and know which window it is sending events to even before
  // it gives us a handle.  Warning: this is not thread safe!
  _creating_window = this;
  bool opened = open_graphic_window(is_fullscreen());
  _creating_window = (WinGraphicsWindow *)NULL;

  if (!opened) {
    return false;
  }

  // Now that we have a window handle, store it in our global map, so future
  // messages for this window can be routed properly.
  _window_handles.insert(WindowHandles::value_type(_hWnd, this));

  // move window to top of zorder.
  SetWindowPos(_hWnd, HWND_TOP, 0,0,0,0,
               SWP_NOMOVE | SWP_NOSENDCHANGING | SWP_NOSIZE);

  // need to do twice to override any minimized flags in StartProcessInfo
  if (want_minimized) {
    ShowWindow(_hWnd, SW_MINIMIZE);
    ShowWindow(_hWnd, SW_MINIMIZE);
  } else {
    ShowWindow(_hWnd, SW_SHOWNORMAL);
    ShowWindow(_hWnd, SW_SHOWNORMAL);
  }

  HWND new_foreground_window = _hWnd;
  if (!want_foreground) {
    // If we specifically requested the window not to be on top, restore the
    // previous foreground window (if we can).
    new_foreground_window = old_foreground_window;
  }

  if (!SetActiveWindow(new_foreground_window)) {
    windisplay_cat.warning()
      << "SetActiveWindow() failed!\n";
  }

  // Let's aggressively call SetForegroundWindow() in addition to
  // SetActiveWindow().  It seems to work in some cases to make the window
  // come to the top, where SetActiveWindow doesn't work.
  if (!SetForegroundWindow(new_foreground_window)) {
    windisplay_cat.warning()
      << "SetForegroundWindow() failed!\n";
  }

  // Determine the initial open status of the IME.
  _ime_open = false;
  _ime_active = false;
  HIMC hIMC = ImmGetContext(_hWnd);
  if (hIMC != 0) {
    _ime_open = (ImmGetOpenStatus(hIMC) != 0);
    ImmReleaseContext(_hWnd, hIMC);
  }

  // Registers to receive the WM_INPUT messages
  if (_input_devices.size() > 1) {
    RAWINPUTDEVICE Rid;
    Rid.usUsagePage = 0x01;
    Rid.usUsage = 0x02;
    Rid.dwFlags = 0;// RIDEV_NOLEGACY;   // adds HID mouse and also ignores legacy mouse messages
    Rid.hwndTarget = _hWnd;
    RegisterRawInputDevices(&Rid, 1, sizeof (Rid));
  }

  // Create a WindowHandle for ourselves
  _window_handle = NativeWindowHandle::make_win(_hWnd);

  // Actually, we want a WinWindowHandle.
  _window_handle = new WinWindowHandle(this, *_window_handle);

  // And tell our parent window that we're now its child.
  if (_parent_window_handle != (WindowHandle *)NULL) {
    _parent_window_handle->attach_child(_window_handle);
  }

  // set us as the focus window for keyboard input
  set_focus();

  // Try initializing the touch function pointers.
  static bool initialized = false;
  if (!initialized) {
    initialized = true;
    HMODULE user32 = GetModuleHandleA("user32.dll");
    if (user32) {
      // Introduced in Windows 7.
      pRegisterTouchWindow = (PFN_REGISTERTOUCHWINDOW)GetProcAddress(user32, "RegisterTouchWindow");
      pGetTouchInputInfo = (PFN_GETTOUCHINPUTINFO)GetProcAddress(user32, "GetTouchInputInfo");
      pCloseTouchInputHandle = (PFN_CLOSETOUCHINPUTHANDLE)GetProcAddress(user32, "CloseTouchInputHandle");
    }
  }

  // Register for Win7 touch events.
  if (pRegisterTouchWindow != NULL) {
    pRegisterTouchWindow(_hWnd, 0);
  }

  return true;
}

/**
 * Creates the array of input devices.  The first one is always the system
 * mouse and keyboard.  Each subsequent one is a raw mouse device.  Also
 * initializes a parallel array, _input_device_handle, with the win32 handle
 * of each raw input device.
 */

void WinGraphicsWindow::
initialize_input_devices() {
  UINT nInputDevices;
  PRAWINPUTDEVICELIST pRawInputDeviceList;

  nassertv(_input_devices.size() == 0);

  // Clear the handle array, and set up the system keyboardmouse
  memset(_input_device_handle, 0, sizeof(_input_device_handle));
  GraphicsWindowInputDevice device =
    GraphicsWindowInputDevice::pointer_and_keyboard(this, "keyboard_mouse");
  add_input_device(device);

  // Get the number of devices.
  if (GetRawInputDeviceList(NULL, &nInputDevices, sizeof(RAWINPUTDEVICELIST)) != 0) {
    return;
  }

  // Allocate the array to hold the DeviceList
  pRawInputDeviceList = (PRAWINPUTDEVICELIST)alloca(sizeof(RAWINPUTDEVICELIST) * nInputDevices);
  if (pRawInputDeviceList==0) {
    return;
  }

  // Fill the Array
  if (GetRawInputDeviceList(pRawInputDeviceList, &nInputDevices, sizeof(RAWINPUTDEVICELIST)) == -1) {
    return;
  }

  // Loop through all raw devices and find the raw mice
  for (int i = 0; i < (int)nInputDevices; i++) {
    if (pRawInputDeviceList[i].dwType == RIM_TYPEMOUSE) {
      // Fetch information about specified mouse device.
      UINT nSize;
      if (GetRawInputDeviceInfoA(pRawInputDeviceList[i].hDevice, RIDI_DEVICENAME, (LPVOID)0, &nSize) != 0) {
        return;
      }
      char *psName = (char*)alloca(sizeof(TCHAR) * nSize);
      if (psName == 0) return;
      if (GetRawInputDeviceInfoA(pRawInputDeviceList[i].hDevice, RIDI_DEVICENAME, (LPVOID)psName, &nSize) < 0) {
        return;
      }

      // If it's not an RDP mouse, add it to the list of raw mice.
      if (strncmp(psName,"\\??\\Root#RDP_MOU#0000#",22)!=0) {
        if (_input_devices.size() < 32) {
          if (strncmp(psName,"\\??\\",4)==0) psName += 4;
          char *pound1 = strchr(psName,'#');
          char *pound2 = pound1 ? strchr(pound1+1,'#') : 0;
          char *pound3 = pound2 ? strchr(pound2+1,'#') : 0;
          if (pound3) *pound3 = 0;
          for (char *p = psName; *p; p++) {
            if (((*p<'a')||(*p>'z')) && ((*p<'A')||(*p>'Z')) && ((*p<'0')||(*p>'9'))) {
              *p = '_';
            }
          }
          if (pound2) *pound2 = '.';
          _input_device_handle[_input_devices.size()] = pRawInputDeviceList[i].hDevice;
          GraphicsWindowInputDevice device = GraphicsWindowInputDevice::pointer_only(this, psName);
          device.set_pointer_in_window(0,0);
          add_input_device(device);
        }
      }
    }
  }
}

/**
 * This is a hook for derived classes to do something special, if necessary,
 * when a fullscreen window has been minimized.  The given WindowProperties
 * struct will be applied to this window's properties after this function
 * returns.
 */
void WinGraphicsWindow::
fullscreen_minimized(WindowProperties &) {
}

/**
 * This is a hook for derived classes to do something special, if necessary,
 * when a fullscreen window has been restored after being minimized.  The
 * given WindowProperties struct will be applied to this window's properties
 * after this function returns.
 */
void WinGraphicsWindow::
fullscreen_restored(WindowProperties &) {
}

/**
 * Called from the window thread in response to a request from within the code
 * (via request_properties()) to change the size and/or position of the
 * window.  Returns true if the window is successfully changed, or false if
 * there was a problem.
 */
bool WinGraphicsWindow::
do_reshape_request(int x_origin, int y_origin, bool has_origin,
                   int x_size, int y_size) {
  if (windisplay_cat.is_debug()) {
    windisplay_cat.debug()
      << "Got reshape request (" << x_origin << ", " << y_origin
      << ", " << has_origin << ", " << x_size << ", " << y_size << ")\n";
  }
  if (!is_fullscreen()) {
    if (has_origin) {
      // A coordinate of -2 means to center the window in its client area.
      if (x_origin == -2) {
        x_origin = 0.5 * (_pipe->get_display_width() - x_size);
      }
      if (y_origin == -2) {
        y_origin = 0.5 * (_pipe->get_display_height() - y_size);
      }
      _properties.set_origin(x_origin, y_origin);

      if (x_origin == -1 && y_origin == -1) {
        x_origin = 0;
        y_origin = 0;
        has_origin = false;
      }
    }

    // Compute the appropriate size and placement for the window, including
    // decorations.
    RECT view_rect;
    SetRect(&view_rect, x_origin, y_origin,
            x_origin + x_size, y_origin + y_size);
    WINDOWINFO wi;
    GetWindowInfo(_hWnd, &wi);
    AdjustWindowRectEx(&view_rect, wi.dwStyle, FALSE, wi.dwExStyle);

    UINT flags = SWP_NOZORDER | SWP_NOSENDCHANGING;

    if (has_origin) {
      x_origin = view_rect.left;
      y_origin = view_rect.top;
    } else {
      x_origin = CW_USEDEFAULT;
      y_origin = CW_USEDEFAULT;
      flags |= SWP_NOMOVE;
    }

    SetWindowPos(_hWnd, NULL, x_origin, y_origin,
                 view_rect.right - view_rect.left,
                 view_rect.bottom - view_rect.top,
                 flags);

    handle_reshape();
    return true;
  }

  // Resizing a fullscreen window is a little trickier.
  return do_fullscreen_resize(x_size, y_size);
}

/**
 * Called in the window thread when the window size or location is changed,
 * this updates the properties structure accordingly.
 */
void WinGraphicsWindow::
handle_reshape() {
  RECT view_rect;
  if (!GetClientRect(_hWnd, &view_rect)) {
    // Sometimes we get a "reshape" before the window is fully created, in
    // which case GetClientRect() ought to fail.  Ignore this.
    if (windisplay_cat.is_debug()) {
      windisplay_cat.debug()
        << "GetClientRect() failed in handle_reshape.  Ignoring.\n";
    }
    return;
  }

  // But in practice, GetClientRect() doesn't really fail, but just returns
  // all zeroes.  Ignore this too.
  if (view_rect.left == 0 && view_rect.right == 0 &&
      view_rect.bottom == 0 && view_rect.top == 0) {
    if (windisplay_cat.is_debug()) {
      windisplay_cat.debug()
        << "GetClientRect() returned all zeroes in handle_reshape.  Ignoring.\n";
    }
    return;
  }

  bool result = (FALSE != ClientToScreen(_hWnd, (POINT*)&view_rect.left));   // translates top,left pnt
  if (result) {
    result = (FALSE != ClientToScreen(_hWnd, (POINT*)&view_rect.right));  // translates right,bottom pnt
  }

  if (!result) {
    if (windisplay_cat.is_debug()) {
      windisplay_cat.debug()
        << "ClientToScreen() failed in handle_reshape.  Ignoring.\n";
    }
    return;
  }

  WindowProperties properties;
  properties.set_size((view_rect.right - view_rect.left),
                      (view_rect.bottom - view_rect.top));

  // _props origin should reflect upper left of view rectangle
  properties.set_origin(view_rect.left, view_rect.top);

  if (windisplay_cat.is_debug()) {
    windisplay_cat.debug()
      << "reshape to origin: (" << properties.get_x_origin() << ","
      << properties.get_y_origin() << "), size: (" << properties.get_x_size()
      << "," << properties.get_y_size() << ")\n";
  }

  adjust_z_order();
  system_changed_properties(properties);
}

/**
 * Called in the window thread to resize a fullscreen window.
 */
bool WinGraphicsWindow::
do_fullscreen_resize(int x_size, int y_size) {
  HWND hDesktopWindow = GetDesktopWindow();
  HDC scrnDC = GetDC(hDesktopWindow);
  DWORD dwFullScreenBitDepth = GetDeviceCaps(scrnDC, BITSPIXEL);
  ReleaseDC(hDesktopWindow, scrnDC);

  // resize will always leave screen bitdepth unchanged

  // allowing resizing of lowvidmem cards to > 640x480.  why?  I'll assume
  // check was already done by caller, so he knows what he wants

  DEVMODE dm;
  if (!find_acceptable_display_mode(x_size, y_size,
                                    dwFullScreenBitDepth, dm)) {
    windisplay_cat.error()
      << "window resize(" << x_size << ", " << y_size
      << ") failed, no compatible fullscreen display mode found!\n";
    return false;
  }

  // this causes WM_SIZE msg to be produced
  SetWindowPos(_hWnd, NULL, 0,0, x_size, y_size,
               SWP_NOZORDER | SWP_NOMOVE | SWP_NOSENDCHANGING);
  int chg_result = ChangeDisplaySettings(&dm, CDS_FULLSCREEN);

  if (chg_result != DISP_CHANGE_SUCCESSFUL) {
    windisplay_cat.error()
      << "resize ChangeDisplaySettings failed (error code: "
      << chg_result << ") for specified res: "
      << dm.dmPelsWidth << " x " << dm.dmPelsHeight
      << " x " << dm.dmBitsPerPel << ", "
      << dm.dmDisplayFrequency << " Hz\n";
    return false;
  }

  _fullscreen_display_mode = dm;

  windisplay_cat.info()
    << "Resized fullscreen window to " << x_size << ", " << y_size
    << " bitdepth " << dwFullScreenBitDepth << ", "
    << dm.dmDisplayFrequency << "Hz\n";

  _properties.set_size(x_size, y_size);
  set_size_and_recalc(x_size, y_size);

  return true;
}

/**
 * Called in the set_properties_now function to switch to fullscreen.
 */
bool WinGraphicsWindow::
do_fullscreen_switch() {
  if (!do_fullscreen_enable()) {
    // Couldn't get fullscreen.
    return false;
  }

  DWORD window_style = make_style(true);
  SetWindowLong(_hWnd, GWL_STYLE, window_style);

  WINDOW_METRICS metrics;
  bool has_origin;
  if (!calculate_metrics(true, window_style, metrics, has_origin)){
    return false;
  }

  SetWindowPos(_hWnd, HWND_NOTOPMOST, 0, 0, metrics.width, metrics.height,
    SWP_FRAMECHANGED | SWP_SHOWWINDOW);
  return true;
}

/**
 * Called in the set_properties_now function to switch to windowed mode.
 */
bool WinGraphicsWindow::
do_windowed_switch() {
  do_fullscreen_disable();
  DWORD window_style = make_style(false);
  SetWindowLong(_hWnd, GWL_STYLE, window_style);

  WINDOW_METRICS metrics;
  bool has_origin;

  if (!calculate_metrics(false, window_style, metrics, has_origin)){
    return false;
  }

  // We send SWP_FRAMECHANGED so that the new styles are taken into account.
  // Also, we place the Windows at 0,0 to play safe until we decide how to get
  // Panda to remember the windowed origin.

  SetWindowPos(_hWnd, HWND_NOTOPMOST, 0, 0,
               metrics.width, metrics.height,
               SWP_FRAMECHANGED | SWP_SHOWWINDOW);

  return true;
}

/**
 * Called before creating a fullscreen window to give the driver a chance to
 * adjust the particular resolution request, if necessary.
 */
void WinGraphicsWindow::
reconsider_fullscreen_size(DWORD &, DWORD &, DWORD &) {
}

/**
 * Some windows graphics contexts (e.g.  DirectX) require special support to
 * enable the displaying of an overlay window (particularly the IME window)
 * over the fullscreen graphics window.  This is a hook for the window to
 * enable or disable that mode when necessary.
 */
void WinGraphicsWindow::
support_overlay_window(bool) {
}

/**
 * Constructs a dwStyle for the specified mode, be it windowed or fullscreen.
 */
DWORD WinGraphicsWindow::
make_style(bool fullscreen) {
  // from MSDN: An OpenGL window has its own pixel format.  Because of this,
  // only device contexts retrieved for the client area of an OpenGL window
  // are allowed to draw into the window.  As a result, an OpenGL window
  // should be created with the WS_CLIPCHILDREN and WS_CLIPSIBLINGS styles.
  // Additionally, the window class attribute should not include the
  // CS_PARENTDC style.

  DWORD window_style = WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

  if (fullscreen){
    window_style |= WS_SYSMENU;
  } else if (!_properties.get_undecorated()) {
    window_style |= (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);

    if (!_properties.get_fixed_size()) {
      window_style |= (WS_SIZEBOX | WS_MAXIMIZEBOX);
    } else {
      window_style |= WS_BORDER;
    }
  }
  return window_style;
}

/**
 * Calculates the metrics for the specified mode, be it windowed or
 * fullscreen.
 */
bool WinGraphicsWindow::
calculate_metrics(bool fullscreen, DWORD window_style, WINDOW_METRICS &metrics,
                  bool &has_origin) {
  metrics.x = 0;
  metrics.y = 0;
  has_origin = _properties.has_origin();
  if (!fullscreen && has_origin) {
    metrics.x = _properties.get_x_origin();
    metrics.y = _properties.get_y_origin();

    // A coordinate of -2 means to center the window in its client area.
    if (metrics.x == -2) {
      metrics.x = 0.5 * (_pipe->get_display_width() - _properties.get_x_size());
    }
    if (metrics.y == -2) {
      metrics.y = 0.5 * (_pipe->get_display_height() - _properties.get_y_size());
    }
    _properties.set_origin(metrics.x, metrics.y);

    if (metrics.x == -1 && metrics.y == -1) {
      metrics.x = 0;
      metrics.y = 0;
      has_origin = false;
    }
  }

  metrics.width = _properties.get_x_size();
  metrics.height = _properties.get_y_size();

  if (!fullscreen){
    RECT win_rect;
    SetRect(&win_rect, metrics.x, metrics.y,
            metrics.x + metrics.width, metrics.y + metrics.height);

    // Compute window size based on desired client area size
    if (!AdjustWindowRect(&win_rect, window_style, FALSE)) {
      windisplay_cat.error()
        << "AdjustWindowRect failed!" << endl;
      return false;
    }

    if (has_origin) {
      metrics.x = win_rect.left;
      metrics.y = win_rect.top;
    } else {
      metrics.x = CW_USEDEFAULT;
      metrics.y = CW_USEDEFAULT;
    }
    metrics.width = win_rect.right - win_rect.left;
    metrics.height = win_rect.bottom - win_rect.top;
  }

  return true;
}

/**
 * Creates a regular or fullscreen window.
 */
bool WinGraphicsWindow::
open_graphic_window(bool fullscreen) {
  DWORD window_style = make_style(fullscreen);

  wstring title;
  if (_properties.has_title()) {
    TextEncoder encoder;
    title = encoder.decode_text(_properties.get_title());
  }

  if (!_properties.has_size()) {
    // Just fill in a conservative default size if one isn't specified.
    _properties.set_size(640, 480);
  }

  WINDOW_METRICS metrics;
  bool has_origin;
  if (!calculate_metrics(fullscreen, window_style, metrics, has_origin)){
    return false;
  }

  const WindowClass &wclass = register_window_class(_properties);
  HINSTANCE hinstance = GetModuleHandle(NULL);

  _hparent = NULL;

  if (!fullscreen){
    WindowHandle *window_handle = _properties.get_parent_window();
    if (window_handle != NULL) {
      windisplay_cat.info()
        << "Got parent_window " << *window_handle << "\n";
      WindowHandle::OSHandle *os_handle = window_handle->get_os_handle();
      if (os_handle != NULL) {
        windisplay_cat.info()
          << "os_handle type " << os_handle->get_type() << "\n";

        if (os_handle->is_of_type(NativeWindowHandle::WinHandle::get_class_type())) {
          NativeWindowHandle::WinHandle *win_handle = DCAST(NativeWindowHandle::WinHandle, os_handle);
          _hparent = win_handle->get_handle();
          } else if (os_handle->is_of_type(NativeWindowHandle::IntHandle::get_class_type())) {
          NativeWindowHandle::IntHandle *int_handle = DCAST(NativeWindowHandle::IntHandle, os_handle);
          _hparent = (HWND)int_handle->get_handle();
        }
      }
    }
    _parent_window_handle = window_handle;
  } else {
    _parent_window_handle = NULL;
  }

  if (!_hparent) { // This can be a regular window or a fullscreen window
    _hWnd = CreateWindowW(wclass._name.c_str(), title.c_str(), window_style,
                          metrics.x, metrics.y,
                          metrics.width,
                          metrics.height,
                          NULL, NULL, hinstance, 0);
  } else { // This is a regular window with a parent
    int x_origin = 0;
    int y_origin = 0;

    if (!fullscreen && has_origin) {
      x_origin = _properties.get_x_origin();
      y_origin = _properties.get_y_origin();
    }

    _hWnd = CreateWindowW(wclass._name.c_str(), title.c_str(),
                          WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS ,
                          x_origin, y_origin,
                          _properties.get_x_size(), _properties.get_y_size(),
                          _hparent, NULL, hinstance, 0);

    if (_hWnd) {
      // join our keyboard state with the parents

      // Actually, let's not.  Is there really any reason to do this?  It
      // causes problems with the browser plugin--it deadlocks when the parent
      // process is waiting on the child process.
      // AttachThreadInput(GetWindowThreadProcessId(_hparent,NULL),
      // GetCurrentThreadId(),TRUE);

      WindowProperties properties;
      properties.set_foreground(true);
      system_changed_properties(properties);
    }
  }

  if (!_hWnd) {
    windisplay_cat.error()
      << "CreateWindow() failed!" << endl;
    show_error_message();
    return false;
  }

  // I'd prefer to CreateWindow after DisplayChange in case it messes up GL
  // somehow, but I need the window's black background to cover up the desktop
  // during the mode change.

  if (fullscreen){
    if (!do_fullscreen_enable()){
      return false;
    }
  }

  return true;
}

/**
 * This is a low-level function that just puts Windows in fullscreen mode.
 * Not to confuse with do_fullscreen_switch().
 */
bool WinGraphicsWindow::
do_fullscreen_enable() {

  HWND hDesktopWindow = GetDesktopWindow();
  HDC scrnDC = GetDC(hDesktopWindow);
  DWORD cur_bitdepth = GetDeviceCaps(scrnDC, BITSPIXEL);
  // DWORD drvr_ver = GetDeviceCaps(scrnDC, DRIVERVERSION); DWORD
  // cur_scrnwidth = GetDeviceCaps(scrnDC, HORZRES); DWORD cur_scrnheight =
  // GetDeviceCaps(scrnDC, VERTRES);
  ReleaseDC(hDesktopWindow, scrnDC);

  DWORD dwWidth = _properties.get_x_size();
  DWORD dwHeight = _properties.get_y_size();
  DWORD dwFullScreenBitDepth = cur_bitdepth;

  DEVMODE dm;
  reconsider_fullscreen_size(dwWidth, dwHeight, dwFullScreenBitDepth);
  if (!find_acceptable_display_mode(dwWidth, dwHeight, dwFullScreenBitDepth, dm)) {
    windisplay_cat.error()
      << "Videocard has no supported display resolutions at specified res ("
      << dwWidth << " x " << dwHeight << " x " << dwFullScreenBitDepth <<")\n";
    return false;
  }

  dm.dmPelsWidth = dwWidth;
  dm.dmPelsHeight = dwHeight;
  dm.dmBitsPerPel = dwFullScreenBitDepth;
  int chg_result = ChangeDisplaySettings(&dm, CDS_FULLSCREEN);

  if (chg_result != DISP_CHANGE_SUCCESSFUL) {
    windisplay_cat.error()
      << "ChangeDisplaySettings failed (error code: "
      << chg_result << ") for specified res: "
      << dm.dmPelsWidth << " x " << dm.dmPelsHeight
      << " x " << dm.dmBitsPerPel << ", "
      << dm.dmDisplayFrequency << " Hz\n";
    return false;
  }

  _fullscreen_display_mode = dm;

  _properties.set_origin(0, 0);
  _properties.set_size(dwWidth, dwHeight);

  return true;

}

/**
 * This is a low-level function that just gets Windows out of fullscreen mode.
 * Not to confuse with do_windowed_switch().
 */
bool WinGraphicsWindow::
do_fullscreen_disable() {
  int chg_result = ChangeDisplaySettings(NULL, 0x0);
  if (chg_result != DISP_CHANGE_SUCCESSFUL) {
    windisplay_cat.warning()
      << "ChangeDisplaySettings failed to restore Windowed mode\n";
    return false;
  }
  return true;
}

/**
 * Adjusts the Z-order of a window after it has been moved.
 */
void WinGraphicsWindow::
adjust_z_order() {
  WindowProperties::ZOrder z_order = _properties.get_z_order();
  adjust_z_order(z_order, z_order);
}

/**
 * Adjusts the Z-order of a window after it has been moved.
 */
void WinGraphicsWindow::
adjust_z_order(WindowProperties::ZOrder last_z_order,
               WindowProperties::ZOrder this_z_order) {
  HWND order;
  bool do_change = false;

  switch (this_z_order) {
  case WindowProperties::Z_bottom:
    order = HWND_BOTTOM;
    do_change = true;
    break;

  case WindowProperties::Z_normal:
    if ((last_z_order != WindowProperties::Z_normal) &&
        // If we aren't changing the window order, don't move it to the top.
        (last_z_order != WindowProperties::Z_bottom ||
         _properties.get_foreground())
        // If the window was previously on the bottom, but it doesn't have
        // focus now, don't move it to the top; it will get moved the next
        // time we get focus.
        ) {
      order = HWND_NOTOPMOST;
      do_change = true;
    }
    break;

  case WindowProperties::Z_top:
    order = HWND_TOPMOST;
    do_change = true;
    break;
  }
  if (do_change) {
    BOOL result = SetWindowPos(_hWnd, order, 0,0,0,0,
                               SWP_NOMOVE | SWP_NOSENDCHANGING | SWP_NOSIZE);
    if (!result) {
      windisplay_cat.warning()
        << "SetWindowPos failed.\n";
    }
  }
}

/**
 * Intended to be called whenever mouse motion is detected within the window,
 * this indicates that the mouse is within the window and tells Windows that
 * we want to be told when the mouse leaves the window.
 */
void WinGraphicsWindow::
track_mouse_leaving(HWND hwnd) {
  WinGraphicsPipe *winpipe;
  DCAST_INTO_V(winpipe, _pipe);

  TRACKMOUSEEVENT tme = {
    sizeof(TRACKMOUSEEVENT),
    TME_LEAVE,
    hwnd,
    0
  };

  // tell win32 to post WM_MOUSELEAVE msgs
  BOOL bSucceeded = TrackMouseEvent(&tme);

  if (!bSucceeded && windisplay_cat.is_debug()) {
    windisplay_cat.debug()
      << "TrackMouseEvent failed!, LastError=" << GetLastError() << endl;
  }

  _tracking_mouse_leaving = true;
}

/**
 * Attempts to set this window as the "focus" window, so that keyboard events
 * come here.
 */
void WinGraphicsWindow::
set_focus() {
  if (SetFocus(_hWnd) == NULL && GetLastError() != 0) {
    // If the SetFocus() request failed, maybe we're running in the plugin
    // environment on Vista, with UAC enabled.  In this case, we're not
    // allowed to assign focus to the Panda window for some stupid reason.  So
    // instead, we have to ask the parent window (in the browser process) to
    // proxy our keyboard events for us.
    if (_parent_window_handle != NULL && _window_handle != NULL) {
      _parent_window_handle->request_keyboard_focus(_window_handle);
    } else {
      // Otherwise, something is wrong.
      windisplay_cat.error()
        << "SetFocus failed: " << GetLastError() << "\n";
    }
  }
}

/**
 * This is called to receive a keyboard event generated by proxy by another
 * window in a parent process.  This hacky system is used in the web plugin
 * system to allow the Panda window to receive keyboard events on Vista, which
 * doesn't allow the Panda window to set keyboard focus to itself.
 */
void WinGraphicsWindow::
receive_windows_message(unsigned int msg, int wparam, int lparam) {
  // Well, we'll just deliver this directly to window_proc(), supplying our
  // own window handle.  For the most part, we don't care about the window
  // handle anyway, but this might become an issue for the IME.  TODO:
  // investigate IME issues.

  window_proc(_hWnd, msg, wparam, lparam);
}

/**
 * This is the nonstatic window_proc function.  It is called to handle window
 * events for this particular window.
 */
LONG WinGraphicsWindow::
window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  if (windisplay_cat.is_spam()) {
    windisplay_cat.spam()
      << ClockObject::get_global_clock()->get_real_time()
      << " window_proc(" << (void *)this << ", " << hwnd << ", "
      << msg << ", " << wparam << ", " << lparam << ")\n";
  }
  WindowProperties properties;

  switch (msg) {
  case WM_MOUSEMOVE:
    if (!_tracking_mouse_leaving) {
      // need to re-call TrackMouseEvent every time mouse re-enters window
      track_mouse_leaving(hwnd);
    }
    set_cursor_in_window();
    if(handle_mouse_motion(translate_mouse(LOWORD(lparam)), translate_mouse(HIWORD(lparam))))
      return 0;
    break;

  case WM_INPUT:
    handle_raw_input((HRAWINPUT)lparam);
    break;

  case WM_MOUSELEAVE:
    _tracking_mouse_leaving = false;
    handle_mouse_exit();
    set_cursor_out_of_window();
    break;

  case WM_CREATE:
    {
      track_mouse_leaving(hwnd);
      ClearToBlack(hwnd, _properties);

      POINT cpos;
      GetCursorPos(&cpos);
      ScreenToClient(hwnd, &cpos);
      RECT clientRect;
      GetClientRect(hwnd, &clientRect);
      if (PtInRect(&clientRect,cpos)) {
        set_cursor_in_window();  // should window focus be true as well?
      } else {
        set_cursor_out_of_window();
      }
    }
    break;

    /*
  case WM_SHOWWINDOW:
    // You'd think WM_SHOWWINDOW would be just the thing for embedded windows,
    // but it turns out it's not sent to the child windows when the parent is
    // minimized.  I guess it's only sent for an explicit call to ShowWindow,
    // phooey.
    {
      if (windisplay_cat.is_debug()) {
        windisplay_cat.debug()
          << "WM_SHOWWINDOW: " << hwnd << ", " << wparam << "\n";
      }
      if (wparam) {
        // Window is being shown.
        properties.set_minimized(false);
      } else {
        // Window is being hidden.
        properties.set_minimized(true);
      }
      system_changed_properties(properties);
    }
    break;
    */

  case WM_CLOSE:
    // This is a message from the system indicating that the user has
    // requested to close the window (e.g.  alt-f4).
    {
      string close_request_event = get_close_request_event();
      if (!close_request_event.empty()) {
        // In this case, the app has indicated a desire to intercept the
        // request and process it directly.
        throw_event(close_request_event);
        return 0;

      } else {
        // In this case, the default case, the app does not intend to service
        // the request, so we do by closing the window.
        close_window();
        properties.set_open(false);
        system_changed_properties(properties);

        // TODO: make sure we release the GSG properly.
      }
    }
    break;

  case WM_CHILDACTIVATE:
    if (windisplay_cat.is_debug()) {
      windisplay_cat.debug()
        << "WM_CHILDACTIVATE: " << hwnd << "\n";
    }
    break;

  case WM_ACTIVATE:
    if (windisplay_cat.is_debug()) {
      windisplay_cat.debug()
        << "WM_ACTIVATE: " << hwnd << ", " << wparam << ", " << lparam << "\n";
    }
    properties.set_minimized((wparam & 0xffff0000) != 0);
    if ((wparam & 0xffff) != WA_INACTIVE)
      {
        properties.set_foreground(true);
        if (is_fullscreen())
          {
            // When a fullscreen window goes active, it automatically gets un-
            // minimized.
            int chg_result =
              ChangeDisplaySettings(&_fullscreen_display_mode, CDS_FULLSCREEN);
            if (chg_result != DISP_CHANGE_SUCCESSFUL) {
              const DEVMODE &dm = _fullscreen_display_mode;
              windisplay_cat.error()
                << "restore ChangeDisplaySettings failed (error code: "
                << chg_result << ") for specified res: "
                << dm.dmPelsWidth << " x " << dm.dmPelsHeight
                << " x " << dm.dmBitsPerPel << ", "
                << dm.dmDisplayFrequency << " Hz\n";
            }

            GdiFlush();
            SetWindowPos(_hWnd, HWND_TOP, 0,0,0,0, SWP_NOMOVE | SWP_NOSENDCHANGING | SWP_NOSIZE | SWP_NOOWNERZORDER);
            fullscreen_restored(properties);
          }
      }
    else
      {
        properties.set_foreground(false);
        if (is_fullscreen())
          {
            // When a fullscreen window goes inactive, it automatically gets
            // minimized.
            properties.set_minimized(true);

            // It seems order is important here.  We must minimize the window
            // before restoring the display settings, or risk losing the
            // graphics context.
            ShowWindow(_hWnd, SW_MINIMIZE);
            GdiFlush();
            do_fullscreen_disable();
            fullscreen_minimized(properties);
          }
      }

    adjust_z_order();
    system_changed_properties(properties);
    break;

  case WM_SIZE:
    // Actually, since we don't return in WM_WINDOWPOSCHANGED, WM_SIZE won't
    // end up being called at all.  This is more efficient according to MSDN.
    if (windisplay_cat.is_debug()) {
      windisplay_cat.debug()
        << "WM_SIZE: " << hwnd << ", " << wparam << "\n";
    }
    break;

  case WM_EXITSIZEMOVE:
    // handle_reshape();
    break;

  case WM_WINDOWPOSCHANGED:
    if (windisplay_cat.is_debug()) {
      windisplay_cat.debug()
        << "WM_WINDOWPOSCHANGED: " << hwnd << ", " << wparam << "\n";
    }
    if (_hWnd != NULL) {
      handle_reshape();
    }
    adjust_z_order();
    return 0;

  case WM_PAINT:
    // In response to WM_PAINT, we check to see if there are any update
    // regions at all; if there are, we declare the window exposed.  This is
    // used to implement !_unexposed_draw.
    if (GetUpdateRect(_hWnd, NULL, false)) {
      if (windisplay_cat.is_spam()) {
        windisplay_cat.spam()
          << "Got update regions: " << this << "\n";
      }
      _got_expose_event = true;
    }
    break;

  case WM_LBUTTONDOWN:
    if (_lost_keypresses) {
      resend_lost_keypresses();
    }
    SetCapture(hwnd);
    _input_devices[0].set_pointer_in_window(translate_mouse(LOWORD(lparam)), translate_mouse(HIWORD(lparam)));
    _input_devices[0].button_down(MouseButton::button(0), get_message_time());

    // A button-click in the window means to grab the keyboard focus.
    set_focus();
    return 0;

  case WM_MBUTTONDOWN:
    if (_lost_keypresses) {
      resend_lost_keypresses();
    }
    SetCapture(hwnd);
    _input_devices[0].set_pointer_in_window(translate_mouse(LOWORD(lparam)), translate_mouse(HIWORD(lparam)));
    _input_devices[0].button_down(MouseButton::button(1), get_message_time());
    // A button-click in the window means to grab the keyboard focus.
    set_focus();
    return 0;

  case WM_RBUTTONDOWN:
    if (_lost_keypresses) {
      resend_lost_keypresses();
    }
    SetCapture(hwnd);
    _input_devices[0].set_pointer_in_window(translate_mouse(LOWORD(lparam)), translate_mouse(HIWORD(lparam)));
    _input_devices[0].button_down(MouseButton::button(2), get_message_time());
    // A button-click in the window means to grab the keyboard focus.
    set_focus();
    return 0;

  case WM_XBUTTONDOWN:
    {
      if (_lost_keypresses) {
        resend_lost_keypresses();
      }
      SetCapture(hwnd);
      int whichButton = GET_XBUTTON_WPARAM(wparam);
      _input_devices[0].set_pointer_in_window(translate_mouse(LOWORD(lparam)), translate_mouse(HIWORD(lparam)));
      if (whichButton == XBUTTON1) {
        _input_devices[0].button_down(MouseButton::button(3), get_message_time());
      } else if (whichButton == XBUTTON2) {
        _input_devices[0].button_down(MouseButton::button(4), get_message_time());
      }
    }
    return 0;

  case WM_LBUTTONUP:
    if (_lost_keypresses) {
      resend_lost_keypresses();
    }
    ReleaseCapture();
    _input_devices[0].button_up(MouseButton::button(0), get_message_time());
    return 0;

  case WM_MBUTTONUP:
    if (_lost_keypresses) {
      resend_lost_keypresses();
    }
    ReleaseCapture();
    _input_devices[0].button_up(MouseButton::button(1), get_message_time());
    return 0;

  case WM_RBUTTONUP:
    if (_lost_keypresses) {
      resend_lost_keypresses();
    }
    ReleaseCapture();
    _input_devices[0].button_up(MouseButton::button(2), get_message_time());
    return 0;

  case WM_XBUTTONUP:
    {
      if (_lost_keypresses) {
        resend_lost_keypresses();
      }
      ReleaseCapture();
      int whichButton = GET_XBUTTON_WPARAM(wparam);
      if (whichButton == XBUTTON1) {
        _input_devices[0].button_up(MouseButton::button(3), get_message_time());
      } else if (whichButton == XBUTTON2) {
        _input_devices[0].button_up(MouseButton::button(4), get_message_time());
      }
    }
    return 0;

  case WM_MOUSEWHEEL:
    {
      int delta = GET_WHEEL_DELTA_WPARAM(wparam);

      POINT point;
      GetCursorPos(&point);
      ScreenToClient(hwnd, &point);
      double time = get_message_time();

      if (delta >= 0) {
        while (delta > 0) {
          handle_keypress(MouseButton::wheel_up(), point.x, point.y, time);
          handle_keyrelease(MouseButton::wheel_up(), time);
          delta -= WHEEL_DELTA;
        }
      } else {
        while (delta < 0) {
          handle_keypress(MouseButton::wheel_down(), point.x, point.y, time);
          handle_keyrelease(MouseButton::wheel_down(), time);
          delta += WHEEL_DELTA;
        }
      }
      return 0;
    }
    break;


  case WM_IME_SETCONTEXT:
    if (!ime_hide)
      break;

    windisplay_cat.debug() << "hwnd = " << hwnd << " and GetFocus = " << GetFocus() << endl;
    _ime_hWnd = ImmGetDefaultIMEWnd(hwnd);
    if (::SendMessage(_ime_hWnd, WM_IME_CONTROL, IMC_CLOSESTATUSWINDOW, 0))
      // if (::SendMessage(hwnd, WM_IME_CONTROL, IMC_CLOSESTATUSWINDOW, 0))
      windisplay_cat.debug() << "SendMessage failed for " << _ime_hWnd << endl;
    else
      windisplay_cat.debug() << "SendMessage Succeeded for " << _ime_hWnd << endl;

    windisplay_cat.debug() << "wparam is " << wparam << ", lparam is " << lparam << endl;
    lparam &= ~ISC_SHOWUIALL;
    if (ImmIsUIMessage(_ime_hWnd, msg, wparam, lparam))
      windisplay_cat.debug() << "wparam is " << wparam << ", lparam is " << lparam << endl;

    break;


  case WM_IME_NOTIFY:
    if (wparam == IMN_SETOPENSTATUS) {
      HIMC hIMC = ImmGetContext(hwnd);
      nassertr(hIMC != 0, 0);
      _ime_open = (ImmGetOpenStatus(hIMC) != 0);
      if (!_ime_open) {
        _ime_active = false;  // Sanity enforcement.
      }
      if (ime_hide) {
        // if (0) {
        COMPOSITIONFORM comf;
        CANDIDATEFORM canf;
        ImmGetCompositionWindow(hIMC, &comf);
        ImmGetCandidateWindow(hIMC, 0, &canf);
        windisplay_cat.debug() <<
          "comf style " << comf.dwStyle <<
          " comf point: x" << comf.ptCurrentPos.x << ",y " << comf.ptCurrentPos.y <<
          " comf rect: l " << comf.rcArea.left << ",t " << comf.rcArea.top << ",r " <<
          comf.rcArea.right << ",b " << comf.rcArea.bottom << endl;
        windisplay_cat.debug() <<
          "canf style " << canf.dwStyle <<
          " canf point: x" << canf.ptCurrentPos.x << ",y " << canf.ptCurrentPos.y <<
          " canf rect: l " << canf.rcArea.left << ",t " << canf.rcArea.top << ",r " <<
          canf.rcArea.right << ",b " << canf.rcArea.bottom << endl;
        comf.dwStyle = CFS_POINT;
        comf.ptCurrentPos.x = 2000;
        comf.ptCurrentPos.y = 2000;

        canf.dwStyle = CFS_EXCLUDE;
        canf.dwIndex = 0;
        canf.ptCurrentPos.x = 0;
        canf.ptCurrentPos.y = 0;
        canf.rcArea.left = 0;
        canf.rcArea.top = 0;
        canf.rcArea.right = 640;
        canf.rcArea.bottom = 480;

#if 0
        comf.rcArea.left = 200;
        comf.rcArea.top = 200;
        comf.rcArea.right = 0;
        comf.rcArea.bottom = 0;
#endif

        if (ImmSetCompositionWindow(hIMC, &comf))
          windisplay_cat.debug() << "set composition form: success\n";
        for (int i=0; i<3; ++i) {
          if (ImmSetCandidateWindow(hIMC, &canf))
            windisplay_cat.debug() << "set candidate form: success\n";
          canf.dwIndex++;
        }
      }

      ImmReleaseContext(hwnd, hIMC);
    }
    break;

  case WM_IME_STARTCOMPOSITION:
    support_overlay_window(true);
    _ime_active = true;
    break;

  case WM_IME_ENDCOMPOSITION:
    support_overlay_window(false);
    _ime_active = false;

    if (ime_aware) {
      wstring ws;
      _input_devices[0].candidate(ws, 0, 0, 0);
    }

    break;

  case WM_IME_COMPOSITION:
    if (ime_aware) {

      // If the ime window is not marked as active at this point, we must be
      // in the process of closing it down (in close_ime), and we don't want
      // to send the current composition string in that case.  But we do need
      // to return 0 to tell windows not to try to send the composition string
      // through WM_CHAR messages.
      if (!_ime_active) {
        return 0;
      }

      HIMC hIMC = ImmGetContext(hwnd);
      nassertr(hIMC != 0, 0);

      DWORD result_size = 0;
      static const int ime_buffer_size = 256;
      static const int ime_buffer_size_bytes = ime_buffer_size / sizeof(wchar_t);
      wchar_t ime_buffer[ime_buffer_size];
      size_t cursor_pos, delta_start;

      if (lparam & GCS_RESULTSTR) {
        result_size = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR,
                                               ime_buffer, ime_buffer_size_bytes);
        size_t num_chars = result_size / sizeof(wchar_t);
        for (size_t i = 0; i < num_chars; ++i) {
          _input_devices[0].keystroke(ime_buffer[i]);
        }
      }

      if (lparam & GCS_COMPSTR) {
        result_size = ImmGetCompositionStringW(hIMC, GCS_CURSORPOS, NULL, 0);
        cursor_pos = result_size & 0xffff;

        result_size = ImmGetCompositionStringW(hIMC, GCS_DELTASTART, NULL, 0);
        delta_start = result_size & 0xffff;
        result_size = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, ime_buffer, ime_buffer_size);
        size_t num_chars = result_size / sizeof(wchar_t);

        _input_devices[0].candidate(wstring(ime_buffer, num_chars),
                                    min(cursor_pos, delta_start),
                                    max(cursor_pos, delta_start),
                                    cursor_pos);
      }
      ImmReleaseContext(hwnd, hIMC);
    }
    break;

  case WM_CHAR:
    // Ignore WM_CHAR messages if we have the IME open, since everything will
    // come in through WM_IME_COMPOSITION.  (It's supposed to come in through
    // WM_CHAR, too, but there seems to be a bug in Win2000 in that it only
    // sends question mark characters through here.)

    // Actually, probably that "bug" was due to the fact that we were
    // previously using the ANSI versions of RegisterClass etc., in which case
    // the actual value passed to WM_CHAR seems to be poorly defined.  Now we
    // are using RegisterClassW etc., which means WM_CHAR is absolutely
    // supposed to be utf-16.
    if (!_ime_open) {
      _input_devices[0].keystroke(wparam);
    }
    break;

  case WM_SYSKEYDOWN:
    if (_lost_keypresses) {
      resend_lost_keypresses();
    }
    if (windisplay_cat.is_debug()) {
      windisplay_cat.debug()
        << "syskeydown: " << wparam << " (" << lookup_key(wparam) << ")\n";
    }
    {
      // Alt and F10 are sent as WM_SYSKEYDOWN instead of WM_KEYDOWN want to
      // use defwindproc on Alt syskey so std windows cmd Alt-F4 works, etc
      POINT point;
      GetCursorPos(&point);
      ScreenToClient(hwnd, &point);
      handle_keypress(lookup_key(wparam), point.x, point.y,
                      get_message_time());

      if ((lparam & 0x40000000) == 0) {
        handle_raw_keypress(lookup_raw_key(lparam), get_message_time());
      }

/*
 * wparam does not contain leftright information for SHIFT, CONTROL, or ALT,
 * so we have to track their status and do the right thing.  We'll send the
 * leftright specific key event along with the general key event.  Key
 * repeating is not being handled consistently for LALT and RALT, but from
 * comments below, it's only being handled the way it is for backspace, so
 * we'll leave it as is.
 */
      if (wparam == VK_MENU) {
        if ((GetKeyState(VK_LMENU) & 0x8000) != 0 && ! _lalt_down) {
          handle_keypress(KeyboardButton::lalt(), point.x, point.y,
                          get_message_time());
          _lalt_down = true;
        }
        if ((GetKeyState(VK_RMENU) & 0x8000) != 0 && ! _ralt_down) {
          handle_keypress(KeyboardButton::ralt(), point.x, point.y,
                          get_message_time());
          _ralt_down = true;
        }
      }
      if (wparam == VK_F10) {
        // bypass default windproc F10 behavior (it activates the main menu,
        // but we have none)
        return 0;
      }
    }
    break;

  case WM_SYSCOMMAND:
    if (wparam == SC_KEYMENU) {
      // if Alt is released (alone wo other keys), defwindproc will send this
      // command, which will 'activate' the title bar menu (we have none) and
      // give focus to it.  we don't want this to happen, so kill this msg.

      // Note that the WM_SYSKEYUP message for Alt has already been sent (if
      // it is going to be), so ignoring this special message does no harm.
      return 0;
    }
    break;

  case WM_KEYDOWN:
    if (_lost_keypresses) {
      resend_lost_keypresses();
    }
    if (windisplay_cat.is_debug()) {
      windisplay_cat.debug()
        << "keydown: " << wparam << " (" << lookup_key(wparam) << ")\n";
    }

    // If this bit is not zero, this is just a keyrepeat echo; we ignore these
    // for handle_keypress (we respect keyrepeat only for handle_keystroke).
    if ((lparam & 0x40000000) == 0) {
      POINT point;
      GetCursorPos(&point);
      ScreenToClient(hwnd, &point);
      handle_keypress(lookup_key(wparam), point.x, point.y,
                      get_message_time());
      handle_raw_keypress(lookup_raw_key(lparam), get_message_time());

      // wparam does not contain leftright information for SHIFT, CONTROL, or
      // ALT, so we have to track their status and do the right thing.  We'll
      // send the leftright specific key event along with the general key
      // event.
      if (wparam == VK_SHIFT) {
        if ((GetKeyState(VK_LSHIFT) & 0x8000) != 0 && ! _lshift_down) {
          handle_keypress(KeyboardButton::lshift(), point.x, point.y,
                          get_message_time());
          _lshift_down = true;
        }
        if ((GetKeyState(VK_RSHIFT) & 0x8000) != 0 && ! _rshift_down) {
          handle_keypress(KeyboardButton::rshift(), point.x, point.y,
                          get_message_time());
          _rshift_down = true;
        }
      } else if(wparam == VK_CONTROL) {
        if ((GetKeyState(VK_LCONTROL) & 0x8000) != 0 && ! _lcontrol_down) {
          handle_keypress(KeyboardButton::lcontrol(), point.x, point.y,
                          get_message_time());
          _lcontrol_down = true;
        }
        if ((GetKeyState(VK_RCONTROL) & 0x8000) != 0 && ! _rcontrol_down) {
          handle_keypress(KeyboardButton::rcontrol(), point.x, point.y,
                          get_message_time());
          _rcontrol_down = true;
        }
      }

      // Handle Cntrl-V paste from clipboard.  Is there a better way to detect
      // this hotkey?
      if ((wparam=='V') && (GetKeyState(VK_CONTROL) < 0) &&
          !_input_devices.empty()) {
        HGLOBAL hglb;
        char *lptstr;

        if (IsClipboardFormatAvailable(CF_TEXT) && OpenClipboard(NULL)) {
          // Maybe we should support CF_UNICODETEXT if it is available too?
          hglb = GetClipboardData(CF_TEXT);
          if (hglb!=NULL) {
            lptstr = (char *) GlobalLock(hglb);
            if (lptstr != NULL)  {
              char *pChar;
              for (pChar=lptstr; *pChar; pChar++) {
                _input_devices[0].keystroke((uchar)*pChar);
              }
              GlobalUnlock(hglb);
            }
          }
          CloseClipboard();
        }
      }
    } else {
      // Actually, for now we'll respect the repeat anyway, just so we support
      // backspace properly.  Rethink later.
      POINT point;
      GetCursorPos(&point);
      ScreenToClient(hwnd, &point);
      handle_keypress(lookup_key(wparam), point.x, point.y,
                      get_message_time());

/*
 * wparam does not contain leftright information for SHIFT, CONTROL, or ALT,
 * so we have to track their status and do the right thing.  We'll send the
 * leftright specific key event along with the general key event.  If the user
 * presses LSHIFT and then RSHIFT, the RSHIFT event will come in with the
 * keyrepeat flag on (i.e.  it will end up in this block).  The logic below
 * should detect this correctly and only send the RSHIFT event.  Note that the
 * CONTROL event will be sent twice, once for each keypress.  Since keyrepeats
 * are currently being sent simply as additional keypress events, that should
 * be okay for now.
 */
      if (wparam == VK_SHIFT) {
        if (((GetKeyState(VK_LSHIFT) & 0x8000) != 0) && ! _lshift_down ) {
          handle_keypress(KeyboardButton::lshift(), point.x, point.y,
                          get_message_time());
          _lshift_down = true;
        } else if (((GetKeyState(VK_RSHIFT) & 0x8000) != 0) && ! _rshift_down ) {
          handle_keypress(KeyboardButton::rshift(), point.x, point.y,
                          get_message_time());
          _rshift_down = true;
        } else {
          if ((GetKeyState(VK_LSHIFT) & 0x8000) != 0) {
            handle_keypress(KeyboardButton::lshift(), point.x, point.y,
                            get_message_time());
          }
          if ((GetKeyState(VK_RSHIFT) & 0x8000) != 0) {
            handle_keypress(KeyboardButton::rshift(), point.x, point.y,
                            get_message_time());
          }
        }
      } else if(wparam == VK_CONTROL) {
        if (((GetKeyState(VK_LCONTROL) & 0x8000) != 0) && ! _lcontrol_down ) {
          handle_keypress(KeyboardButton::lcontrol(), point.x, point.y,
                          get_message_time());
          _lcontrol_down = true;
        } else if (((GetKeyState(VK_RCONTROL) & 0x8000) != 0) && ! _rcontrol_down ) {
          handle_keypress(KeyboardButton::rcontrol(), point.x, point.y,
                          get_message_time());
          _rcontrol_down = true;
        } else {
          if ((GetKeyState(VK_LCONTROL) & 0x8000) != 0) {
            handle_keypress(KeyboardButton::lcontrol(), point.x, point.y,
                            get_message_time());
          }
          if ((GetKeyState(VK_RCONTROL) & 0x8000) != 0) {
            handle_keypress(KeyboardButton::rcontrol(), point.x, point.y,
                            get_message_time());
          }
        }
      }
    }
    break;

  case WM_SYSKEYUP:
  case WM_KEYUP:
    if (_lost_keypresses) {
      resend_lost_keypresses();
    }
    if (windisplay_cat.is_debug()) {
      windisplay_cat.debug()
        << "keyup: " << wparam << " (" << lookup_key(wparam) << ")\n";
    }
    handle_keyrelease(lookup_key(wparam), get_message_time());
    handle_raw_keyrelease(lookup_raw_key(lparam), get_message_time());

    // wparam does not contain leftright information for SHIFT, CONTROL, or
    // ALT, so we have to track their status and do the right thing.  We'll
    // send the leftright specific key event along with the general key event.
    if (wparam == VK_SHIFT) {
      if ((GetKeyState(VK_LSHIFT) & 0x8000) == 0 && _lshift_down) {
        handle_keyrelease(KeyboardButton::lshift(), get_message_time());
        _lshift_down = false;
      }
      if ((GetKeyState(VK_RSHIFT) & 0x8000) == 0 && _rshift_down) {
        handle_keyrelease(KeyboardButton::rshift(), get_message_time());
        _rshift_down = false;
      }
    } else if(wparam == VK_CONTROL) {
      if ((GetKeyState(VK_LCONTROL) & 0x8000) == 0 && _lcontrol_down) {
        handle_keyrelease(KeyboardButton::lcontrol(), get_message_time());
        _lcontrol_down = false;
      }
      if ((GetKeyState(VK_RCONTROL) & 0x8000) == 0 && _rcontrol_down) {
        handle_keyrelease(KeyboardButton::rcontrol(), get_message_time());
        _rcontrol_down = false;
      }
    } else if(wparam == VK_MENU) {
      if ((GetKeyState(VK_LMENU) & 0x8000) == 0 && _lalt_down) {
        handle_keyrelease(KeyboardButton::lalt(), get_message_time());
        _lalt_down = false;
      }
      if ((GetKeyState(VK_RMENU) & 0x8000) == 0 && _ralt_down) {
        handle_keyrelease(KeyboardButton::ralt(), get_message_time());
        _ralt_down = false;
      }
    }
    break;

  case WM_KILLFOCUS:
    if (windisplay_cat.is_debug()) {
      windisplay_cat.debug()
        << "killfocus\n";
    }

    _input_devices[0].focus_lost(get_message_time());
    properties.set_foreground(false);
    system_changed_properties(properties);
    break;

  case WM_SETFOCUS:
    // You would think that this would be a good time to call
    // resend_lost_keypresses(), but it turns out that we get WM_SETFOCUS
    // slightly before Windows starts resending key updown events to us.

/*
 * In particular, if the user restored focus using alt-tab, then at this point
 * the keyboard state will indicate that both the alt and tab keys are held
 * down.  However, there is a small window of opportunity for the user to
 * release these keys before Windows starts telling us about keyup events.
 * Thus, if we record the fact that alt and tab are being held down now, we
 * may miss the keyup events for them, and they can get "stuck" down.
 */

    // So we have to defer calling resend_lost_keypresses() until we know
    // Windows is ready to send us key updown events.  I don't know when we
    // can guarantee that, except when we actually do start to receive key
    // updown events, so that call is made there.

    if (windisplay_cat.is_debug()) {
      windisplay_cat.debug()
        << "setfocus\n";
    }

    if (_lost_keypresses) {
      resend_lost_keypresses();
    }

    properties.set_foreground(true);
    system_changed_properties(properties);
    break;

  case PM_ACTIVE:
    if (windisplay_cat.is_debug()) {
      windisplay_cat.debug()
        << "PM_ACTIVE\n";
    }
    properties.set_foreground(true);
    system_changed_properties(properties);
    break;

  case PM_INACTIVE:
    if (windisplay_cat.is_debug()) {
      windisplay_cat.debug()
        << "PM_INACTIVE\n";
    }
    properties.set_foreground(false);
    system_changed_properties(properties);
    break;

  case WM_DPICHANGED:
    // The window moved to a monitor of different DPI, or someone changed the
    // DPI setting in the configuration panel.
    if (windisplay_cat.is_debug()) {
      windisplay_cat.debug() << "DPI changed to " << LOWORD(wparam);

      if (LOWORD(wparam) != HIWORD(wparam)) {
        windisplay_cat.debug(false) << "x" << HIWORD(wparam) << "\n";
      } else {
        windisplay_cat.debug(false) << "\n";
      }
    }
    // Resize the window if requested to match the new DPI. Obviously, don't
    // do this if a fixed size was requested.
    if (!_properties.get_fixed_size() && dpi_window_resize) {
      RECT &rect = *(LPRECT)lparam;
      SetWindowPos(_hWnd, HWND_TOP, rect.left, rect.top,
                   rect.right - rect.left, rect.bottom - rect.top,
                   SWP_NOZORDER | SWP_NOACTIVATE);
    }
    break;

  case WM_TOUCH:
    _num_touches = LOWORD(wparam);
    if (_num_touches > MAX_TOUCHES) {
      _num_touches = MAX_TOUCHES;
    }
    if (pGetTouchInputInfo != 0) {
      pGetTouchInputInfo((HTOUCHINPUT)lparam, _num_touches, _touches, sizeof(TOUCHINPUT));
      pCloseTouchInputHandle((HTOUCHINPUT)lparam);
    }
    break;
  }

  // do custom messages processing if any has been set
  for ( WinProcClasses::iterator it=_window_proc_classes.begin() ; it != _window_proc_classes.end(); it++ ){
      (*it)->wnd_proc(this, hwnd, msg, wparam, lparam);
  }

  return DefWindowProcW(hwnd, msg, wparam, lparam);
}


/**
 * This is attached to the window class for all WinGraphicsWindow windows; it
 * is called to handle window events.
 */
LONG WINAPI WinGraphicsWindow::
static_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  // Look up the window in our global map.
  WindowHandles::const_iterator wi;
  wi = _window_handles.find(hwnd);
  if (wi != _window_handles.end()) {
    // We found the window.
    return (*wi).second->window_proc(hwnd, msg, wparam, lparam);
  }

  // The window wasn't in the map; we must be creating it right now.
  if (_creating_window != (WinGraphicsWindow *)NULL) {
    return _creating_window->window_proc(hwnd, msg, wparam, lparam);
  }

  // Oops, we weren't creating a window!  Don't know how to handle the
  // message, so just pass it on to Windows to deal with it.
  return DefWindowProcW(hwnd, msg, wparam, lparam);
}

/**
 * Handles one event from the message queue.
 */
void WinGraphicsWindow::
process_1_event() {
  MSG msg;

  if (!GetMessage(&msg, NULL, 0, 0)) {
    // WM_QUIT received.  We need a cleaner way to deal with this.
    // DestroyAllWindows(false);
    exit(msg.wParam);  // this will invoke AtExitFn
  }

  // Translate virtual key messages
  TranslateMessage(&msg);
  // Call window_proc
  DispatchMessage(&msg);
}

/**
 * Called when the keyboard focus has been restored to the window after it has
 * been lost for a time, this rechecks the keyboard state and generates key
 * up/down messages for keys that have changed state in the meantime.
 */
void WinGraphicsWindow::
resend_lost_keypresses() {
  nassertv(_lost_keypresses);
  // This is now a no-op.  Not sure we really want to generate new "down" or
  // "resume" events for keys that were held while the window focus is
  // restored.

  _lost_keypresses = false;
}

/**
 * Changes _cursor_window from its current value to the indicated value.  This
 * also changes the cursor properties appropriately.
 */
void WinGraphicsWindow::
update_cursor_window(WinGraphicsWindow *to_window) {
  bool hide_cursor = false;
  if (to_window == (WinGraphicsWindow *)NULL) {
    // We are leaving a graphics window; we should restore the Win2000
    // effects.
    if (_got_saved_params) {
      SystemParametersInfo(SPI_SETMOUSETRAILS, 0,
                           (PVOID)_saved_mouse_trails, 0);
      SystemParametersInfo(SPI_SETCURSORSHADOW, 0,
                           (PVOID)_saved_cursor_shadow, 0);
      SystemParametersInfo(SPI_SETMOUSEVANISH, 0,
                           (PVOID)_saved_mouse_vanish, 0);
      _got_saved_params = false;
    }

  } else {
    const WindowProperties &to_props = to_window->get_properties();
    hide_cursor = to_props.get_cursor_hidden();

    // We are entering a graphics window; we should save and disable the
    // Win2000 effects.  These don't work at all well over a 3-D window.

    // These parameters are only defined for Win2000XP, but they should just
    // cause a silent error on earlier OS's, which is OK.
    if (!_got_saved_params) {
      SystemParametersInfo(SPI_GETMOUSETRAILS, 0,
                           &_saved_mouse_trails, 0);
      SystemParametersInfo(SPI_GETCURSORSHADOW, 0,
                           &_saved_cursor_shadow, 0);
      SystemParametersInfo(SPI_GETMOUSEVANISH, 0,
                           &_saved_mouse_vanish, 0);
      _got_saved_params = true;

      SystemParametersInfo(SPI_SETMOUSETRAILS, 0, (PVOID)0, 0);
      SystemParametersInfo(SPI_SETCURSORSHADOW, 0, (PVOID)false, 0);
      SystemParametersInfo(SPI_SETMOUSEVANISH, 0, (PVOID)false, 0);
    }

    SetCursor(to_window->_cursor);
  }

  hide_or_show_cursor(hide_cursor);

  _cursor_window = to_window;
}

/**
 * Hides or shows the mouse cursor according to the indicated parameter.  This
 * is normally called when the mouse wanders into or out of a window with the
 * cursor_hidden properties.
 */
void WinGraphicsWindow::
hide_or_show_cursor(bool hide_cursor) {
  if (hide_cursor) {
    if (!_cursor_hidden) {
      ShowCursor(false);
      _cursor_hidden = true;
    }
  } else {
    if (_cursor_hidden) {
      ShowCursor(true);
      _cursor_hidden = false;
    }
  }
}

// don't pick any video modes < MIN_REFRESH_RATE Hz
#define MIN_REFRESH_RATE 60
// EnumDisplaySettings may indicate 0 or 1 for refresh rate, which means use
// driver default rate (assume its >min_refresh_rate)
#define ACCEPTABLE_REFRESH_RATE(RATE) ((RATE >= MIN_REFRESH_RATE) || (RATE==0) || (RATE==1))

/**
 * Looks for a fullscreen mode that meets the specified size and bitdepth
 * requirements.  Returns true if a suitable mode is found, false otherwise.
 */
bool WinGraphicsWindow::
find_acceptable_display_mode(DWORD dwWidth, DWORD dwHeight, DWORD bpp,
                             DEVMODE &dm) {

  // Get the current mode.  We'll try to match the refresh rate.
  DEVMODE cur_dm;
  ZeroMemory(&cur_dm, sizeof(cur_dm));
  cur_dm.dmSize = sizeof(cur_dm);
  EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &cur_dm);

  int modenum = 0;
  int saved_modenum = -1;

  while (1) {
    ZeroMemory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);

    if (!EnumDisplaySettings(NULL, modenum, &dm)) {
      break;
    }

    if ((dm.dmPelsWidth == dwWidth) && (dm.dmPelsHeight == dwHeight) &&
        (dm.dmBitsPerPel == bpp)) {
      // If this also matches in refresh rate, we're done here.  Otherwise,
      // save this as a second choice for later.
      if (dm.dmDisplayFrequency == cur_dm.dmDisplayFrequency) {
        return true;
      } else if (saved_modenum == -1) {
        saved_modenum = modenum;
      }
    }
    modenum++;
  }

  // Failed to find an exact match, but we do have a match that didn't match
  // the refresh rate.
  if (saved_modenum != -1) {
    ZeroMemory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);

    if (EnumDisplaySettings(NULL, saved_modenum, &dm)) {
      return true;
    }
  }

  return false;
}

/**
 * Pops up a dialog box with the indicated Windows error message ID (or the
 * last error message generated) for meaningful display to the user.
 */
void WinGraphicsWindow::
show_error_message(DWORD message_id) {
  LPTSTR message_buffer;

  if (message_id == 0) {
    message_id = GetLastError();
  }

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL, message_id,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), //The user default language
                (LPTSTR)&message_buffer,  // the weird ptrptr->ptr cast is intentional, see FORMAT_MESSAGE_ALLOCATE_BUFFER
                1024, NULL);
  MessageBox(GetDesktopWindow(), message_buffer, _T(errorbox_title), MB_OK);
  windisplay_cat.fatal() << "System error msg: " << message_buffer << endl;
  LocalFree(message_buffer);
}

/**
 *
 */
void WinGraphicsWindow::
handle_keypress(ButtonHandle key, int x, int y, double time) {
  _input_devices[0].set_pointer_in_window(x, y);
  if (key != ButtonHandle::none()) {
    _input_devices[0].button_down(key, time);
  }
}

/**
 * Indicates we detected a key was already down when the focus is restored to
 * the window.  Mainly useful for tracking the state of modifier keys.
 */
void WinGraphicsWindow::
handle_keyresume(ButtonHandle key, double time) {
  if (key != ButtonHandle::none()) {
    _input_devices[0].button_resume_down(key, time);
  }
}

/**
 *
 */
void WinGraphicsWindow::
handle_keyrelease(ButtonHandle key, double time) {
  if (key != ButtonHandle::none()) {
    _input_devices[0].button_up(key, time);
  }
}

/**
 *
 */
void WinGraphicsWindow::
handle_raw_keypress(ButtonHandle key, double time) {
  if (key != ButtonHandle::none()) {
    _input_devices[0].raw_button_down(key, time);
  }
}

/**
 *
 */
void WinGraphicsWindow::
handle_raw_keyrelease(ButtonHandle key, double time) {
  if (key != ButtonHandle::none()) {
    _input_devices[0].raw_button_up(key, time);
  }
}

/**
 * Translates the keycode reported by Windows to an appropriate Panda
 * ButtonHandle.
 */
ButtonHandle WinGraphicsWindow::
lookup_key(WPARAM wparam) const {
  // First, check for a few buttons that we filter out when the IME window is
  // open.
  if (!_ime_active) {
    switch(wparam) {
    case VK_BACK: return KeyboardButton::backspace();
    case VK_DELETE: return KeyboardButton::del();
    case VK_ESCAPE: return KeyboardButton::escape();
    case VK_SPACE: return KeyboardButton::space();
    case VK_UP: return KeyboardButton::up();
    case VK_DOWN: return KeyboardButton::down();
    case VK_LEFT: return KeyboardButton::left();
    case VK_RIGHT: return KeyboardButton::right();
    }
  }

  // Now check for the rest of the buttons, including the ones that we allow
  // through even when the IME window is open.
  switch(wparam) {
  case VK_TAB: return KeyboardButton::tab();
  case VK_PRIOR: return KeyboardButton::page_up();
  case VK_NEXT: return KeyboardButton::page_down();
  case VK_HOME: return KeyboardButton::home();
  case VK_END: return KeyboardButton::end();
  case VK_F1: return KeyboardButton::f1();
  case VK_F2: return KeyboardButton::f2();
  case VK_F3: return KeyboardButton::f3();
  case VK_F4: return KeyboardButton::f4();
  case VK_F5: return KeyboardButton::f5();
  case VK_F6: return KeyboardButton::f6();
  case VK_F7: return KeyboardButton::f7();
  case VK_F8: return KeyboardButton::f8();
  case VK_F9: return KeyboardButton::f9();
  case VK_F10: return KeyboardButton::f10();
  case VK_F11: return KeyboardButton::f11();
  case VK_F12: return KeyboardButton::f12();
  case VK_INSERT: return KeyboardButton::insert();
  case VK_CAPITAL: return KeyboardButton::caps_lock();
  case VK_NUMLOCK: return KeyboardButton::num_lock();
  case VK_SCROLL: return KeyboardButton::scroll_lock();
  case VK_PAUSE: return KeyboardButton::pause();
  case VK_SNAPSHOT: return KeyboardButton::print_screen();

  case VK_SHIFT: return KeyboardButton::shift();
  case VK_LSHIFT: return KeyboardButton::lshift();
  case VK_RSHIFT: return KeyboardButton::rshift();

  case VK_CONTROL: return KeyboardButton::control();
  case VK_LCONTROL: return KeyboardButton::lcontrol();
  case VK_RCONTROL: return KeyboardButton::rcontrol();

  case VK_MENU: return KeyboardButton::alt();
  case VK_LMENU: return KeyboardButton::lalt();
  case VK_RMENU: return KeyboardButton::ralt();

  default:
    int key = MapVirtualKey(wparam, 2);
    if (isascii(key) && key != 0) {
      // We used to try to remap lowercase to uppercase keys here based on the
      // state of the shift andor caps lock keys.  But that's a mistake, and
      // doesn't allow for international or user-defined keyboards; let
      // Windows do that mapping.

      // Nowadays, we make a distinction between a "button" and a "keystroke".
      // A button corresponds to a physical button on the keyboard and has a
      // down and up event associated.  A keystroke may or may not correspond
      // to a physical button, but will be some Unicode character and will not
      // have a corresponding up event.
      return KeyboardButton::ascii_key(tolower(key));
    }
    break;
  }
  return ButtonHandle::none();
}

/**
 * Translates the scancode reported by Windows to an appropriate Panda
 * ButtonHandle.
 */
ButtonHandle WinGraphicsWindow::
lookup_raw_key(LPARAM lparam) const {
  unsigned char vsc = (lparam & 0xff0000) >> 16;

  if (lparam & 0x1000000) {
    // Extended keys
    switch (vsc) {
    case 28: return KeyboardButton::enter();
    case 29: return KeyboardButton::rcontrol();
    case 53: return KeyboardButton::ascii_key('/');
    case 55: return KeyboardButton::print_screen();
    case 56: return KeyboardButton::ralt();
    case 69: return KeyboardButton::num_lock();
    case 71: return KeyboardButton::home();
    case 72: return KeyboardButton::up();
    case 73: return KeyboardButton::page_up();
    case 75: return KeyboardButton::left();
    case 77: return KeyboardButton::right();
    case 79: return KeyboardButton::end();
    case 80: return KeyboardButton::down();
    case 81: return KeyboardButton::page_down();
    case 82: return KeyboardButton::insert();
    case 83: return KeyboardButton::del();
    case 91: return KeyboardButton::lmeta();
    case 92: return KeyboardButton::rmeta();
    case 93: return KeyboardButton::menu();
    }
  }

  if (vsc <= 83) {
    static ButtonHandle raw_map[] = {
      ButtonHandle::none(),
      KeyboardButton::escape(),
      KeyboardButton::ascii_key('1'),
      KeyboardButton::ascii_key('2'),
      KeyboardButton::ascii_key('3'),
      KeyboardButton::ascii_key('4'),
      KeyboardButton::ascii_key('5'),
      KeyboardButton::ascii_key('6'),
      KeyboardButton::ascii_key('7'),
      KeyboardButton::ascii_key('8'),
      KeyboardButton::ascii_key('9'),
      KeyboardButton::ascii_key('0'),
      KeyboardButton::ascii_key('-'),
      KeyboardButton::ascii_key('='),
      KeyboardButton::backspace(),
      KeyboardButton::tab(),
      KeyboardButton::ascii_key('q'),
      KeyboardButton::ascii_key('w'),
      KeyboardButton::ascii_key('e'),
      KeyboardButton::ascii_key('r'),
      KeyboardButton::ascii_key('t'),
      KeyboardButton::ascii_key('y'),
      KeyboardButton::ascii_key('u'),
      KeyboardButton::ascii_key('i'),
      KeyboardButton::ascii_key('o'),
      KeyboardButton::ascii_key('p'),
      KeyboardButton::ascii_key('['),
      KeyboardButton::ascii_key(']'),
      KeyboardButton::enter(),
      KeyboardButton::lcontrol(),
      KeyboardButton::ascii_key('a'),
      KeyboardButton::ascii_key('s'),
      KeyboardButton::ascii_key('d'),
      KeyboardButton::ascii_key('f'),
      KeyboardButton::ascii_key('g'),
      KeyboardButton::ascii_key('h'),
      KeyboardButton::ascii_key('j'),
      KeyboardButton::ascii_key('k'),
      KeyboardButton::ascii_key('l'),
      KeyboardButton::ascii_key(';'),
      KeyboardButton::ascii_key('\''),
      KeyboardButton::ascii_key('`'),
      KeyboardButton::lshift(),
      KeyboardButton::ascii_key('\\'),
      KeyboardButton::ascii_key('z'),
      KeyboardButton::ascii_key('x'),
      KeyboardButton::ascii_key('c'),
      KeyboardButton::ascii_key('v'),
      KeyboardButton::ascii_key('b'),
      KeyboardButton::ascii_key('n'),
      KeyboardButton::ascii_key('m'),
      KeyboardButton::ascii_key(','),
      KeyboardButton::ascii_key('.'),
      KeyboardButton::ascii_key('/'),
      KeyboardButton::rshift(),
      KeyboardButton::ascii_key('*'),
      KeyboardButton::lalt(),
      KeyboardButton::space(),
      KeyboardButton::caps_lock(),
      KeyboardButton::f1(),
      KeyboardButton::f2(),
      KeyboardButton::f3(),
      KeyboardButton::f4(),
      KeyboardButton::f5(),
      KeyboardButton::f6(),
      KeyboardButton::f7(),
      KeyboardButton::f8(),
      KeyboardButton::f9(),
      KeyboardButton::f10(),
      KeyboardButton::pause(),
      KeyboardButton::scroll_lock(),
      KeyboardButton::ascii_key('7'),
      KeyboardButton::ascii_key('8'),
      KeyboardButton::ascii_key('9'),
      KeyboardButton::ascii_key('-'),
      KeyboardButton::ascii_key('4'),
      KeyboardButton::ascii_key('5'),
      KeyboardButton::ascii_key('6'),
      KeyboardButton::ascii_key('+'),
      KeyboardButton::ascii_key('1'),
      KeyboardButton::ascii_key('2'),
      KeyboardButton::ascii_key('3'),
      KeyboardButton::ascii_key('0'),
      KeyboardButton::ascii_key('.')
    };
    return raw_map[vsc];
  }

  // A few additional keys don't fit well in the above table.
  switch (vsc) {
  case 87: return KeyboardButton::f11();
  case 88: return KeyboardButton::f12();
  default: return ButtonHandle::none();
  }
}

/**
 * Returns a ButtonMap containing the association between raw buttons and
 * virtual buttons.
 *
 * Note that on Windows, the pause button and numpad keys are not mapped
 * reliably.
 */
ButtonMap *WinGraphicsWindow::
get_keyboard_map() const {
  ButtonMap *map = new ButtonMap;

  wchar_t text[256];
  UINT vsc = 0;
  unsigned short ex_vsc[] = {0x57, 0x58,
    0x011c, 0x011d, 0x0135, 0x0137, 0x0138, 0x0145, 0x0147, 0x0148, 0x0149, 0x014b, 0x014d, 0x014f, 0x0150, 0x0151, 0x0152, 0x0153, 0x015b, 0x015c, 0x015d};

  for (int k = 1; k < 84 + 17; ++k) {
    if (k >= 84) {
      vsc = ex_vsc[k - 84];
    } else {
      vsc = k;
    }

    UINT lparam = vsc << 16;
    ButtonHandle raw_button = lookup_raw_key(lparam);
    if (raw_button == ButtonHandle::none()) {
      continue;
    }

    ButtonHandle button;
    if (vsc == 0x45) {
      button = KeyboardButton::pause();

    } else if (vsc >= 0x47 && vsc <= 0x53) {
      // The numpad keys are not mapped correctly, see KB72583
      button = raw_button;

    } else {
      if (vsc == 0x145) {
        // Don't ask why - I have no idea.
        vsc = 0x45;
      }
      if (vsc & 0x0100) {
        // MapVirtualKey recognizes extended codes differently.
        vsc ^= 0xe100;
      }

      UINT vk = MapVirtualKeyA(vsc, MAPVK_VSC_TO_VK_EX);
      button = lookup_key(vk);
      //if (button == ButtonHandle::none()) {
      //  continue;
      //}
    }

    int len = GetKeyNameTextW(lparam, text, 256);
    TextEncoder enc;
    enc.set_wtext(wstring(text, len));
    map->map_button(raw_button, button, enc.get_text());
  }

  return map;
}

/**
 *
 */
void WinGraphicsWindow::
handle_raw_input(HRAWINPUT hraw) {
  LPBYTE lpb;
  UINT dwSize;

  if (hraw == 0) {
    return;
  }
  if (GetRawInputData(hraw, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER)) == -1) {
    return;
  }

  lpb = (LPBYTE)alloca(sizeof(LPBYTE) * dwSize);
  if (lpb == NULL) {
    return;
  }

  if (GetRawInputData(hraw, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
    return;
  }

  RAWINPUT *raw = (RAWINPUT *)lpb;
  if (raw->header.hDevice == 0) {
    return;
  }

  for (int i = 1; i < (int)(_input_devices.size()); ++i) {
    if (_input_device_handle[i] == raw->header.hDevice) {
      int adjx = raw->data.mouse.lLastX;
      int adjy = raw->data.mouse.lLastY;

      if (raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
        _input_devices[i].set_pointer_in_window(adjx, adjy);
      } else {
        int oldx = _input_devices[i].get_raw_pointer().get_x();
        int oldy = _input_devices[i].get_raw_pointer().get_y();
        _input_devices[i].set_pointer_in_window(oldx + adjx, oldy + adjy);
      }

      if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN) {
        _input_devices[i].button_down(MouseButton::button(0), get_message_time());
      }
      if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_UP) {
        _input_devices[i].button_up(MouseButton::button(0), get_message_time());
      }
      if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN) {
        _input_devices[i].button_down(MouseButton::button(2), get_message_time());
      }
      if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_2_UP) {
        _input_devices[i].button_up(MouseButton::button(2), get_message_time());
      }
      if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN) {
        _input_devices[i].button_down(MouseButton::button(1), get_message_time());
      }
      if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_3_UP) {
        _input_devices[i].button_up(MouseButton::button(1), get_message_time());
      }
      if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) {
        _input_devices[i].button_down(MouseButton::button(3), get_message_time());
      }
      if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP) {
        _input_devices[i].button_up(MouseButton::button(3), get_message_time());
      }
      if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) {
        _input_devices[i].button_down(MouseButton::button(4), get_message_time());
      }
      if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP) {
        _input_devices[i].button_up(MouseButton::button(4), get_message_time());
      }
    }
  }
}

/**
 *
 */
bool WinGraphicsWindow::
handle_mouse_motion(int x, int y) {
  _input_devices[0].set_pointer_in_window(x, y);
  return false;
}

/**
 *
 */
void WinGraphicsWindow::
handle_mouse_exit() {
  // note: 'mouse_motion' is considered the 'entry' event
  _input_devices[0].set_pointer_out_of_window();
}

/**
 * Loads and returns an HICON corresponding to the indicated filename.  If the
 * file cannot be loaded, returns 0.
 */
HICON WinGraphicsWindow::
get_icon(const Filename &filename) {
  // First, look for the unresolved filename in our index.
  IconFilenames::iterator fi = _icon_filenames.find(filename);
  if (fi != _icon_filenames.end()) {
    return (HICON)((*fi).second);
  }

  // If it wasn't found, resolve the filename and search for that.

  // Since we have to use a Windows call to load the image from a filename, we
  // can't load a virtual file and we can't use the virtual file system.
  Filename resolved = filename;
  if (!resolved.resolve_filename(get_model_path())) {
    // The filename doesn't exist along the search path.
    if (resolved.is_fully_qualified() && resolved.exists()) {
      // But it does exist locally, so accept it.

    } else {
      windisplay_cat.warning()
        << "Could not find icon filename " << filename << "\n";
      return 0;
    }
  }
  fi = _icon_filenames.find(resolved);
  if (fi != _icon_filenames.end()) {
    _icon_filenames[filename] = (*fi).second;
    return (HICON)((*fi).second);
  }

  Filename os = resolved.to_os_specific();

  HANDLE h = LoadImage(NULL, os.c_str(),
                       IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
  if (h == 0) {
    windisplay_cat.warning()
      << "windows icon filename '" << os << "' could not be loaded!!\n";
  }

  _icon_filenames[filename] = h;
  _icon_filenames[resolved] = h;
  return (HICON)h;
}

/**
 * Loads and returns an HCURSOR corresponding to the indicated filename.  If
 * the file cannot be loaded, returns 0.
 */
HCURSOR WinGraphicsWindow::
get_cursor(const Filename &filename) {
  // The empty filename means to disable a custom cursor.
  if (filename.empty()) {
    return 0;
  }

  // First, look for the unresolved filename in our index.
  IconFilenames::iterator fi = _cursor_filenames.find(filename);
  if (fi != _cursor_filenames.end()) {
    return (HCURSOR)((*fi).second);
  }

  // If it wasn't found, resolve the filename and search for that.

  // Since we have to use a Windows call to load the image from a filename, we
  // can't load a virtual file and we can't use the virtual file system.
  Filename resolved = filename;
  if (!resolved.resolve_filename(get_model_path())) {
    // The filename doesn't exist.
    windisplay_cat.warning()
      << "Could not find cursor filename " << filename << "\n";
    return 0;
  }
  fi = _cursor_filenames.find(resolved);
  if (fi != _cursor_filenames.end()) {
    _cursor_filenames[filename] = (*fi).second;
    return (HCURSOR)((*fi).second);
  }

  Filename os = resolved.to_os_specific();

  HANDLE h = LoadImage(NULL, os.c_str(),
                       IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE);
  if (h == 0) {
    windisplay_cat.warning()
      << "windows cursor filename '" << os << "' could not be loaded!!\n";
    show_error_message();
  }

  _cursor_filenames[filename] = h;
  _cursor_filenames[resolved] = h;
  return (HCURSOR)h;
}

static HCURSOR get_cursor(const Filename &filename);

/**
 * Registers a Window class appropriate for the indicated properties.  This
 * class may be shared by multiple windows.
 */
const WinGraphicsWindow::WindowClass &WinGraphicsWindow::
register_window_class(const WindowProperties &props) {
  WindowClass wcreg(props);
  std::wostringstream wclass_name;
  wclass_name << L"WinGraphicsWindow" << _window_class_index;
  wcreg._name = wclass_name.str();

  pair<WindowClasses::iterator, bool> found = _window_classes.insert(wcreg);
  const WindowClass &wclass = (*found.first);

  if (!found.second) {
    // We have already created a window class.
    return wclass;
  }

  // We have not yet created this window class.
  _window_class_index++;

  WNDCLASSW wc;

  HINSTANCE instance = GetModuleHandle(NULL);

  // Clear before filling in window structure!
  ZeroMemory(&wc, sizeof(wc));
  wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wc.lpfnWndProc = (WNDPROC)static_window_proc;
  wc.hInstance = instance;

  wc.hIcon = wclass._icon;

  wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  wc.lpszMenuName = NULL;
  wc.lpszClassName = wclass._name.c_str();

  if (!RegisterClassW(&wc)) {
    windisplay_cat.error()
      << "could not register window class " << wclass._name << "!" << endl;
    return wclass;
  }

  return wclass;
}

/**
 *
 */
WinGraphicsWindow::WinWindowHandle::
WinWindowHandle(WinGraphicsWindow *window, const WindowHandle &copy) :
  WindowHandle(copy),
  _window(window)
{
}

/**
 * Should be called by the WinGraphicsWindow's destructor, so that we don't
 * end up with a floating pointer should this object persist beyond the
 * lifespan of its window.
 */
void WinGraphicsWindow::WinWindowHandle::
clear_window() {
  _window = NULL;
}

/**
 * Called on a child handle to deliver a keyboard button event generated in
 * the parent window.
 */
void WinGraphicsWindow::WinWindowHandle::
receive_windows_message(unsigned int msg, int wparam, int lparam) {
  if (_window != NULL) {
    _window->receive_windows_message(msg, wparam, lparam);
  }
}


// pops up MsgBox wsystem error msg
void PrintErrorMessage(DWORD msgID) {
  LPTSTR pMessageBuffer;

  if (msgID==PRINT_LAST_ERROR)
    msgID=GetLastError();

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,msgID,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), //The user default language
                (LPTSTR) &pMessageBuffer,  // the weird ptrptr->ptr cast is intentional, see FORMAT_MESSAGE_ALLOCATE_BUFFER
                1024, NULL);
  MessageBox(GetDesktopWindow(),pMessageBuffer,_T(errorbox_title),MB_OK);
  windisplay_cat.fatal() << "System error msg: " << pMessageBuffer << endl;
  LocalFree( pMessageBuffer );
}

void
ClearToBlack(HWND hWnd, const WindowProperties &props) {
  if (!props.has_origin()) {
    if (windisplay_cat.is_debug()) {
      windisplay_cat.debug()
        << "Skipping ClearToBlack, no origin specified yet.\n";
    }
    return;
  }

  if (windisplay_cat.is_debug()) {
    windisplay_cat.debug()
      << "ClearToBlack(" << hWnd << ", " << props << ")\n";
  }
  // clear to black
  HDC hDC=GetDC(hWnd);  // GetDC is not particularly fast.  if this needs to be super-quick, we should cache GetDC's hDC
  RECT clrRect = {
    props.get_x_origin(), props.get_y_origin(),
    props.get_x_origin() + props.get_x_size(),
    props.get_y_origin() + props.get_y_size()
  };
  FillRect(hDC,&clrRect,(HBRUSH)GetStockObject(BLACK_BRUSH));
  ReleaseDC(hWnd,hDC);
  GdiFlush();
}

/**
 * Fills view_rect with the coordinates of the client area of the indicated
 * window, converted to screen coordinates.
 */
void get_client_rect_screen(HWND hwnd, RECT *view_rect) {
  GetClientRect(hwnd, view_rect);

  POINT ul, lr;
  ul.x = view_rect->left;
  ul.y = view_rect->top;
  lr.x = view_rect->right;
  lr.y = view_rect->bottom;

  ClientToScreen(hwnd, &ul);
  ClientToScreen(hwnd, &lr);

  view_rect->left = ul.x;
  view_rect->top = ul.y;
  view_rect->right = lr.x;
  view_rect->bottom = lr.y;
}

/**
 * Adds the specified Windows proc event handler to be called whenever a
 * Windows event occurs.
 *
 */
void WinGraphicsWindow::add_window_proc( const GraphicsWindowProc* wnd_proc ){
  nassertv(wnd_proc != NULL);
  _window_proc_classes.insert( (GraphicsWindowProc*)wnd_proc );
}

/**
 * Removes the specified Windows proc event handler.
 *
 */
void WinGraphicsWindow::remove_window_proc( const GraphicsWindowProc* wnd_proc ){
  nassertv(wnd_proc != NULL);
  _window_proc_classes.erase( (GraphicsWindowProc*)wnd_proc );
}

/**
 * Removes all Windows proc event handlers.
 *
 */
void WinGraphicsWindow::clear_window_procs(){
  _window_proc_classes.clear();
}

/**
 * Returns whether this window supports adding of windows proc handlers.
 *
 */
bool WinGraphicsWindow::supports_window_procs() const{
  return true;
}

/**
 * Returns whether the specified event msg is a touch message.
 *
 */
bool WinGraphicsWindow::
is_touch_event(GraphicsWindowProcCallbackData *callbackData) {
  return callbackData->get_msg() == WM_TOUCH;
}

/**
 * Returns the current number of touches on this window.
 *
 */
int WinGraphicsWindow::
get_num_touches(){
  return _num_touches;
}

/**
 * Returns the TouchInfo object describing the specified touch.
 *
 */
TouchInfo WinGraphicsWindow::
get_touch_info(int index) {
  nassertr(index >= 0 && index < MAX_TOUCHES, TouchInfo());

  TOUCHINPUT ti = _touches[index];
  POINT point;
  point.x = TOUCH_COORD_TO_PIXEL(ti.x);
  point.y = TOUCH_COORD_TO_PIXEL(ti.y);
  ScreenToClient(_hWnd, &point);

  TouchInfo ret = TouchInfo();
  ret.set_x(point.x);
  ret.set_y(point.y);
  ret.set_id(ti.dwID);
  ret.set_flags(ti.dwFlags);
  return ret;
}
