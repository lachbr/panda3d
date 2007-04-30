// Filename: winGraphicsWindow.cxx
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

#include "winGraphicsWindow.h"
#include "config_windisplay.h"
#include "winGraphicsPipe.h"

#include "graphicsPipe.h"
#include "keyboardButton.h"
#include "mouseButton.h"
#include "clockObject.h"
#include "config_util.h"
#include "throw_event.h"

#include <tchar.h>




TypeHandle WinGraphicsWindow::_type_handle;

WinGraphicsWindow::WindowHandles WinGraphicsWindow::_window_handles;
WinGraphicsWindow *WinGraphicsWindow::_creating_window = NULL;

WinGraphicsWindow *WinGraphicsWindow::_cursor_window = NULL;
bool WinGraphicsWindow::_cursor_hidden = false;

// These are used to save the previous state of the fancy Win2000
// effects that interfere with rendering when the mouse wanders into a
// window's client area.
bool WinGraphicsWindow::_got_saved_params = false;
int WinGraphicsWindow::_saved_mouse_trails;
BOOL WinGraphicsWindow::_saved_cursor_shadow;
BOOL WinGraphicsWindow::_saved_mouse_vanish;

WinGraphicsWindow::IconFilenames WinGraphicsWindow::_icon_filenames;
WinGraphicsWindow::IconFilenames WinGraphicsWindow::_cursor_filenames;

WinGraphicsWindow::WindowClasses WinGraphicsWindow::_window_classes;
int WinGraphicsWindow::_window_class_index = 0;

static const char * const errorbox_title = "Panda3D Error";

////////////////////////////////////////////////////////////////////
//
// These static variables contain pointers to the Raw Input
// functions, which are dynamically extracted from USER32.DLL
//
////////////////////////////////////////////////////////////////////

typedef WINUSERAPI UINT (WINAPI *tGetRawInputDeviceList)
  (OUT PRAWINPUTDEVICELIST pRawInputDeviceList, IN OUT PUINT puiNumDevices, IN UINT cbSize);
typedef WINUSERAPI UINT(WINAPI *tGetRawInputData)
  (IN HRAWINPUT hRawInput, IN UINT uiCommand, OUT LPVOID pData, IN OUT PUINT pcbSize, IN UINT cbSizeHeader);
typedef WINUSERAPI UINT(WINAPI *tGetRawInputDeviceInfoA)
  (IN HANDLE hDevice, IN UINT uiCommand, OUT LPVOID pData, IN OUT PUINT pcbSize);
typedef WINUSERAPI BOOL (WINAPI *tRegisterRawInputDevices)
  (IN PCRAWINPUTDEVICE pRawInputDevices, IN UINT uiNumDevices, IN UINT cbSize);

static tGetRawInputDeviceList    pGetRawInputDeviceList;
static tGetRawInputData          pGetRawInputData;
static tGetRawInputDeviceInfoA   pGetRawInputDeviceInfoA;
static tRegisterRawInputDevices  pRegisterRawInputDevices;

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
WinGraphicsWindow::
WinGraphicsWindow(GraphicsPipe *pipe,
                  const string &name,
                  const FrameBufferProperties &fb_prop,
                  const WindowProperties &win_prop,
                  int flags,
                  GraphicsStateGuardian *gsg,
                  GraphicsOutput *host) : 
  GraphicsWindow(pipe, name, fb_prop, win_prop, flags, gsg, host)
{
  initialize_input_devices();
  _hWnd = (HWND)0;
  _ime_open = false;
  _ime_active = false;
  _ime_composition_w = false;
  _tracking_mouse_leaving = false;
  _maximized = false;
  _cursor = 0;
  memset(_keyboard_state, 0, sizeof(BYTE) * num_virtual_keys);
  _lost_keypresses = false;
  _lshift_down = false;
  _rshift_down = false;
  _lcontrol_down = false;
  _rcontrol_down = false;
  _lalt_down = false;
  _ralt_down = false;
  _hparent = NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::Destructor
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
WinGraphicsWindow::
~WinGraphicsWindow() {
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::move_pointer
//       Access: Published, Virtual
//  Description: Forces the pointer to the indicated position within
//               the window, if possible.  
//
//               Returns true if successful, false on failure.  This
//               may fail if the mouse is not currently within the
//               window, or if the API doesn't support this operation.
////////////////////////////////////////////////////////////////////
bool WinGraphicsWindow::
move_pointer(int device, int x, int y) {
  // First, indicate that the IME is no longer active, so that it won't
  // send the string through WM_IME_COMPOSITION.  But we still leave
  // _ime_open true, so that it also won't send the string through WM_CHAR.
  _ime_active = false;

  // Note: this is not thread-safe; it should be called only from App.
  // Probably not an issue.
  nassertr(device == 0, false);
  if (!_properties.get_foreground() )
//      !_input_devices[0].get_pointer().get_in_window()) 
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
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::close_ime
//       Access: Published, Virtual
//  Description: Forces the ime window to close, if any
//
////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::begin_flip
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
void WinGraphicsWindow::
begin_flip() {
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::process_events
//       Access: Public, Virtual
//  Description: Do whatever processing is necessary to ensure that
//               the window responds to user events.  Also, honor any
//               requests recently made via request_properties()
//
//               This function is called only within the window
//               thread.
////////////////////////////////////////////////////////////////////
void WinGraphicsWindow::
process_events() {
  GraphicsWindow::process_events();

  // We can't treat the message loop specially just because the window
  // is minimized, because we might be reading messages queued up for
  // some other window, which is not minimized.
  /*
  if (!_window_active) {
      // Get 1 msg at a time until no more are left and we block and sleep,
      // or message changes _return_control_to_app or !_window_active status

      while(!_window_active && (!_return_control_to_app)) {
          process_1_event();
      }
      _return_control_to_app = false;

  } else 
  */

  MSG msg;
    
  // Handle all the messages on the queue in a row.  Some of these
  // might be for another window, but they will get dispatched
  // appropriately.
  while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
    process_1_event();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::set_properties_now
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
//               The properties that have been applied are cleared
//               from the structure by this function; so on return,
//               whatever remains in the properties structure are
//               those that were unchanged for some reason (probably
//               because the underlying interface does not support
//               changing that property on an open window).
////////////////////////////////////////////////////////////////////
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
    SetWindowText(_hWnd, title.c_str());
    properties.clear_title();
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

  if (properties.has_foreground() && properties.get_foreground()) 
  {
    if (!SetActiveWindow(_hWnd)) 
    {
      windisplay_cat.warning()  << "SetForegroundWindow() failed!\n";

    } 
    else 
    {
      _properties.set_foreground(true);
    }

    properties.clear_foreground();
  }
}


////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::close_window
//       Access: Protected, Virtual
//  Description: Closes the window right now.  Called from the window
//               thread.
////////////////////////////////////////////////////////////////////
void WinGraphicsWindow::
close_window() {
  set_cursor_out_of_window();
  DestroyWindow(_hWnd);

  if (is_fullscreen()) {
    // revert to default display mode.
    ChangeDisplaySettings(NULL, 0x0);
  }

  // Remove the window handle from our global map.
  _window_handles.erase(_hWnd);
  _hWnd = (HWND)0;

  GraphicsWindow::close_window();
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::open_window
//       Access: Protected, Virtual
//  Description: Opens the window right now.  Called from the window
//               thread.  Returns true if the window is successfully
//               opened, or false if there was a problem.
////////////////////////////////////////////////////////////////////
bool WinGraphicsWindow::
open_window() {
  if (_properties.has_cursor_filename()) {
    _cursor = get_cursor(_properties.get_cursor_filename());
  }
  if (_cursor == 0) {
    _cursor = LoadCursor(NULL, IDC_ARROW);
  }
  bool want_foreground = (!_properties.has_foreground() || _properties.get_foreground());

  HWND old_foreground_window = GetForegroundWindow();

  // Store the current window pointer in _creating_window, so we can
  // call CreateWindow() and know which window it is sending events to
  // even before it gives us a handle.  Warning: this is not thread
  // safe!
  _creating_window = this;
  bool opened;
  if (is_fullscreen()) {
    opened = open_fullscreen_window();
  } else {
    opened = open_regular_window();
  }
  _creating_window = (WinGraphicsWindow *)NULL;

  if (!opened) {
    return false;
  }

  // Now that we have a window handle, store it in our global map, so
  // future messages for this window can be routed properly.
  _window_handles.insert(WindowHandles::value_type(_hWnd, this));
  
  // move window to top of zorder.
  SetWindowPos(_hWnd, HWND_TOP, 0,0,0,0, 
               SWP_NOMOVE | SWP_NOSENDCHANGING | SWP_NOSIZE);
  
  // need to do twice to override any minimized flags in StartProcessInfo
  ShowWindow(_hWnd, SW_SHOWNORMAL);
  ShowWindow(_hWnd, SW_SHOWNORMAL);

  HWND new_foreground_window = _hWnd;
  if (!want_foreground) {
    // If we specifically requested the window not to be on top,
    // restore the previous foreground window (if we can).
    new_foreground_window = old_foreground_window;
  }

  if (!SetActiveWindow(new_foreground_window)) {
    windisplay_cat.warning()
      << "SetActiveWindow() failed!\n";
  }

  // Let's aggressively call SetForegroundWindow() in addition to
  // SetActiveWindow().  It seems to work in some cases to make the
  // window come to the top, where SetActiveWindow doesn't work.
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

  // Check the version of the OS we are running.  If we are running
  // win2000, we must use ImmGetCompositionStringW() to report the
  // characters returned by the IME, since WM_CHAR and
  // ImmGetCompositionStringA() both just return question marks.
  // However, this function doesn't work for Win98; on this OS, we
  // have to use ImmGetCompositionStringA() instead, which returns an
  // encoded string in shift-jis (which we then have to decode).

  // For now, this is user-configurable, to allow testing of this code
  // on both OS's.  After we verify that truth of the above claim, we
  // should base this decision on GetVersionEx() or maybe
  // VerifyVersionInfo().
  _ime_composition_w = ime_composition_w;
  
  // need to re-evaluate above in light of this, it seems that
  // ImmGetCompositionStringW should work on both:
  //     The Input Method Editor and Unicode Windows 98/Me, Windows
  //     NT/2000/XP: Windows supports a Unicode interface for the
  //     IME, in addition to the ANSI interface originally supported.
  //     Windows 98/Me supports all the Unicode functions except
  //     ImmIsUIMessage.  Also, all the messages in Windows 98/Me are
  //     ANSI based.  Since Windows 98/Me does not support Unicode
  //     messages, applications can use ImmGetCompositionString to
  //     receive Unicode characters from a Unicode based IME on
  //     Windows 98/Me.  There are two issues involved with Unicode
  //     handling and the IME.  One is that the Unicode versions of
  //     IME routines return the size of a buffer in bytes rather
  //     than 16-bit Unicode characters,and the other is the IME
  //     normally returns Unicode characters (rather than DBCS) in
  //     the WM_CHAR and WM_IME_CHAR messages.  Use RegisterClassW
  //     to cause the WM_CHAR and WM_IME_CHAR messages to return
  //     Unicode characters in the wParam parameter rather than DBCS
  //     characters.  This is only available under Windows NT; it is
  //     stubbed out in Windows 95/98/Me.

  // Registers to receive the WM_INPUT messages
  if ((pRegisterRawInputDevices)&&(_input_devices.size() > 1)) {
    RAWINPUTDEVICE Rid;
    Rid.usUsagePage = 0x01; 
    Rid.usUsage = 0x02; 
    Rid.dwFlags = 0;// RIDEV_NOLEGACY;   // adds HID mouse and also ignores legacy mouse messages
    Rid.hwndTarget = _hWnd;
    pRegisterRawInputDevices(&Rid, 1, sizeof (Rid));
  }
  
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::initialize_input_devices
//       Access: Private
//  Description: Creates the array of input devices.  The first
//               one is always the system mouse and keyboard.
//               Each subsequent one is a raw mouse device. Also
//               initializes a parallel array, _input_device_handle,
//               with the win32 handle of each raw input device.
////////////////////////////////////////////////////////////////////

void WinGraphicsWindow::
initialize_input_devices() {
  UINT nInputDevices;
  PRAWINPUTDEVICELIST pRawInputDeviceList;

  nassertv(_input_devices.size() == 0);
  
  // Clear the handle array, and set up the system keyboard/mouse
  memset(_input_device_handle, 0, sizeof(_input_device_handle));
  GraphicsWindowInputDevice device =
    GraphicsWindowInputDevice::pointer_and_keyboard("keyboard/mouse");
  _input_devices.push_back(device);
  
  // Try initializing the Raw Input function pointers.
  if (pRegisterRawInputDevices==0) {
    HMODULE user32 = LoadLibrary("user32.dll");
    if (user32) {
      pRegisterRawInputDevices = (tRegisterRawInputDevices)GetProcAddress(user32,"RegisterRawInputDevices");
      pGetRawInputDeviceList   = (tGetRawInputDeviceList)  GetProcAddress(user32,"GetRawInputDeviceList");
      pGetRawInputDeviceInfoA  = (tGetRawInputDeviceInfoA) GetProcAddress(user32,"GetRawInputDeviceInfoA");
      pGetRawInputData         = (tGetRawInputData)        GetProcAddress(user32,"GetRawInputData");
    }
  }
  
  if (pRegisterRawInputDevices==0) return;
  if (pGetRawInputDeviceList==0) return;
  if (pGetRawInputDeviceInfoA==0) return;
  if (pGetRawInputData==0) return;

  // Get the number of devices.
  if (pGetRawInputDeviceList(NULL, &nInputDevices, sizeof(RAWINPUTDEVICELIST)) != 0)
    return;
  
  // Allocate the array to hold the DeviceList
  pRawInputDeviceList = (PRAWINPUTDEVICELIST)alloca(sizeof(RAWINPUTDEVICELIST) * nInputDevices);
  if (pRawInputDeviceList==0) return;

  // Fill the Array
  if (pGetRawInputDeviceList(pRawInputDeviceList, &nInputDevices, sizeof(RAWINPUTDEVICELIST)) == -1)
    return;
  
  // Loop through all raw devices and find the raw mice
  for (int i = 0; i < (int)nInputDevices; i++) {
    if (pRawInputDeviceList[i].dwType == RIM_TYPEMOUSE) {
      // Fetch information about specified mouse device.
      UINT nSize;
      if (pGetRawInputDeviceInfoA(pRawInputDeviceList[i].hDevice, RIDI_DEVICENAME, (LPVOID)0, &nSize) != 0)
        return;
      char *psName = (char*)alloca(sizeof(TCHAR) * nSize);
      if (psName == 0) return;
      if (pGetRawInputDeviceInfoA(pRawInputDeviceList[i].hDevice, RIDI_DEVICENAME, (LPVOID)psName, &nSize) < 0)
        return;

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
          GraphicsWindowInputDevice device = GraphicsWindowInputDevice::pointer_only(psName);
          device.set_pointer_in_window(0,0);
          _input_devices.push_back(device);
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::fullscreen_minimized
//       Access: Protected, Virtual
//  Description: This is a hook for derived classes to do something
//               special, if necessary, when a fullscreen window has
//               been minimized.  The given WindowProperties struct
//               will be applied to this window's properties after
//               this function returns.
////////////////////////////////////////////////////////////////////
void WinGraphicsWindow::
fullscreen_minimized(WindowProperties &) {
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::fullscreen_restored
//       Access: Protected, Virtual
//  Description: This is a hook for derived classes to do something
//               special, if necessary, when a fullscreen window has
//               been restored after being minimized.  The given
//               WindowProperties struct will be applied to this
//               window's properties after this function returns.
////////////////////////////////////////////////////////////////////
void WinGraphicsWindow::
fullscreen_restored(WindowProperties &) {
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::do_reshape_request
//       Access: Protected, Virtual
//  Description: Called from the window thread in response to a request
//               from within the code (via request_properties()) to
//               change the size and/or position of the window.
//               Returns true if the window is successfully changed,
//               or false if there was a problem.
////////////////////////////////////////////////////////////////////
bool WinGraphicsWindow::
do_reshape_request(int x_origin, int y_origin, bool has_origin,
                   int x_size, int y_size) {
  if (windisplay_cat.is_debug()) {
    windisplay_cat.debug()
      << "Got reshape request (" << x_origin << ", " << y_origin
      << ", " << has_origin << ", " << x_size << ", " << y_size << ")\n";
  }
  if (!is_fullscreen()) {
    // Compute the appropriate size and placement for the window,
    // including decorations.
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

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::handle_reshape
//       Access: Protected, Virtual
//  Description: Called in the window thread when the window size or
//               location is changed, this updates the properties
//               structure accordingly.
////////////////////////////////////////////////////////////////////
void WinGraphicsWindow::
handle_reshape() {
  RECT view_rect;
  GetClientRect(_hWnd, &view_rect);
  ClientToScreen(_hWnd, (POINT*)&view_rect.left);   // translates top,left pnt
  ClientToScreen(_hWnd, (POINT*)&view_rect.right);  // translates right,bottom pnt
  
  WindowProperties properties;
  properties.set_size((view_rect.right - view_rect.left), 
                      (view_rect.bottom - view_rect.top));

  // _props origin should reflect upper left of view rectangle
  properties.set_origin(view_rect.left, view_rect.top);
  
  if (windisplay_cat.is_spam()) {
    windisplay_cat.spam()
      << "reshape to origin: (" << properties.get_x_origin() << "," 
      << properties.get_y_origin() << "), size: (" << properties.get_x_size()
      << "," << properties.get_y_size() << ")\n";
  }

  adjust_z_order();
  system_changed_properties(properties);
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::do_fullscreen_resize
//       Access: Protected, Virtual
//  Description: Called in the window thread to resize a fullscreen
//               window.
////////////////////////////////////////////////////////////////////
bool WinGraphicsWindow::
do_fullscreen_resize(int x_size, int y_size) {
  HWND hDesktopWindow = GetDesktopWindow();
  HDC scrnDC = GetDC(hDesktopWindow);
  DWORD dwFullScreenBitDepth = GetDeviceCaps(scrnDC, BITSPIXEL);
  ReleaseDC(hDesktopWindow, scrnDC);

  // resize will always leave screen bitdepth unchanged

  // allowing resizing of lowvidmem cards to > 640x480.  why?  I'll
  // assume check was already done by caller, so he knows what he
  // wants

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
      << chg_result << ") for specified res (" << x_size << " x "
      << y_size << " x " << dwFullScreenBitDepth << "), " 
      << dm.dmDisplayFrequency << "Hz\n";
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

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::reconsider_fullscreen_size
//       Access: Protected, Virtual
//  Description: Called before creating a fullscreen window to give
//               the driver a chance to adjust the particular
//               resolution request, if necessary.
////////////////////////////////////////////////////////////////////
void WinGraphicsWindow::
reconsider_fullscreen_size(DWORD &, DWORD &, DWORD &) {
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::support_overlay_window
//       Access: Protected, Virtual
//  Description: Some windows graphics contexts (e.g. DirectX)
//               require special support to enable the displaying of
//               an overlay window (particularly the IME window) over
//               the fullscreen graphics window.  This is a hook for
//               the window to enable or disable that mode when
//               necessary.
////////////////////////////////////////////////////////////////////
void WinGraphicsWindow::
support_overlay_window(bool) {
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::open_fullscreen_window
//       Access: Private
//  Description: Creates a fullscreen-style window.
////////////////////////////////////////////////////////////////////
bool WinGraphicsWindow::
open_fullscreen_window()
{
  //  from MSDN:
  //  An OpenGL window has its own pixel format. Because of this, only
  //  device contexts retrieved for the client area of an OpenGL
  //  window are allowed to draw into the window. As a result, an
  //  OpenGL window should be created with the WS_CLIPCHILDREN and
  //  WS_CLIPSIBLINGS styles. Additionally, the window class attribute
  //  should not include the CS_PARENTDC style.
  DWORD window_style = 
    WS_POPUP | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

  if (!_properties.has_size()) {
    // Just pick a stupid default size if one isn't specified.
    _properties.set_size(640, 480);
  }

  HWND hDesktopWindow = GetDesktopWindow();
  HDC scrnDC = GetDC(hDesktopWindow);
  DWORD cur_bitdepth = GetDeviceCaps(scrnDC, BITSPIXEL);
  //  DWORD drvr_ver = GetDeviceCaps(scrnDC, DRIVERVERSION);
  //  DWORD cur_scrnwidth = GetDeviceCaps(scrnDC, HORZRES);
  //  DWORD cur_scrnheight = GetDeviceCaps(scrnDC, VERTRES);
  ReleaseDC(hDesktopWindow, scrnDC);

  DWORD dwWidth = _properties.get_x_size();
  DWORD dwHeight = _properties.get_y_size();
  DWORD dwFullScreenBitDepth = cur_bitdepth;
  
  reconsider_fullscreen_size(dwWidth, dwHeight, dwFullScreenBitDepth);
  if (!find_acceptable_display_mode(dwWidth, dwHeight, dwFullScreenBitDepth,
                                    _fullscreen_display_mode)) {
    windisplay_cat.error() 
      << "Videocard has no supported display resolutions at specified res ("
      << dwWidth << " x " << dwHeight << " x " << dwFullScreenBitDepth <<")\n";
    return false;
  }

  string title;
  if (_properties.has_title()) {
    title = _properties.get_title();
  }

  // I'd prefer to CreateWindow after DisplayChange in case it messes
  // up GL somehow, but I need the window's black background to cover
  // up the desktop during the mode change
  const WindowClass &wclass = register_window_class(_properties);
  HINSTANCE hinstance = GetModuleHandle(NULL);
  
  _hWnd = CreateWindow(wclass._name.c_str(), title.c_str(), window_style,
                       0, 0, dwWidth, dwHeight, 
                       hDesktopWindow, NULL, hinstance, 0);
  if (!_hWnd) {
    windisplay_cat.error()
      << "CreateWindow() failed!" << endl;
    show_error_message();
    return false;
  }
   
  int chg_result = ChangeDisplaySettings(&_fullscreen_display_mode, 
                                         CDS_FULLSCREEN);
  if (chg_result != DISP_CHANGE_SUCCESSFUL) {
    windisplay_cat.error()
      << "ChangeDisplaySettings failed (error code: "
      << chg_result << ") for specified res (" << dwWidth
      << " x " << dwHeight << " x " << dwFullScreenBitDepth
      << "), " << _fullscreen_display_mode.dmDisplayFrequency  << "Hz\n";
    return false;
  }

  _properties.set_origin(0, 0);
  _properties.set_size(dwWidth, dwHeight);
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::open_regular_window
//       Access: Private
//  Description: Creates a non-fullscreen window, on the desktop.
////////////////////////////////////////////////////////////////////
bool WinGraphicsWindow::
open_regular_window() {
  //  from MSDN:
  //  An OpenGL window has its own pixel format. Because of this, only
  //  device contexts retrieved for the client area of an OpenGL
  //  window are allowed to draw into the window. As a result, an
  //  OpenGL window should be created with the WS_CLIPCHILDREN and
  //  WS_CLIPSIBLINGS styles. Additionally, the window class attribute
  //  should not include the CS_PARENTDC style.
  DWORD window_style = 
    WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

  if (!_properties.get_undecorated()) {
    window_style |= (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);

    if (!_properties.get_fixed_size()) {
      window_style |= (WS_SIZEBOX | WS_MAXIMIZEBOX);
    } else {
      window_style |= WS_BORDER;
    }
  }

  int x_origin = 0;
  int y_origin = 0;
  if (_properties.has_origin()) {
    x_origin = _properties.get_x_origin();
    y_origin = _properties.get_y_origin();
  }

  int x_size = 100;
  int y_size = 100;
  if (_properties.has_size()) {
    x_size = _properties.get_x_size();
    y_size = _properties.get_y_size();
  }

  RECT win_rect;
  SetRect(&win_rect, x_origin, y_origin,
          x_origin + x_size, y_origin + y_size);
  
  // compute window size based on desired client area size
  if (!AdjustWindowRect(&win_rect, window_style, FALSE)) {
    windisplay_cat.error()
      << "AdjustWindowRect failed!" << endl;
    return false;
  }

  string title;
  if (_properties.has_title()) {
    title = _properties.get_title();
  }

  if (_properties.has_origin()) {
    x_origin = win_rect.left;
    y_origin = win_rect.top;

  } else {
    x_origin = CW_USEDEFAULT;
    y_origin = CW_USEDEFAULT;
  }

  const WindowClass &wclass = register_window_class(_properties);
  HINSTANCE hinstance = GetModuleHandle(NULL);


    _hparent = NULL;
    if(_properties.has_parent_window())
        _hparent = (HWND) _properties.get_parent_window();


    if(!_hparent)
    {
        _hWnd = CreateWindow(wclass._name.c_str(), title.c_str(), window_style, 
                       x_origin, y_origin,
                       win_rect.right - win_rect.left,
                       win_rect.bottom - win_rect.top,
                       NULL, NULL, hinstance, 0);

    }
    else
    {
          x_origin = 0;
          y_origin = 0;


        if (_properties.has_origin()) {
            x_origin = _properties.get_x_origin();
            y_origin = _properties.get_y_origin();
        }

      _hWnd = CreateWindow(wclass._name.c_str(), title.c_str(), 
                       WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS ,
                       //x_origin, y_origin, 
                       x_origin,y_origin,
                       x_size, y_size,
                       _hparent, NULL, hinstance, 0);


      if(_hWnd)
      {
          // join or keyboard state with the parents
        AttachThreadInput(GetWindowThreadProcessId(_hparent,NULL), GetCurrentThreadId(),TRUE);
        // set us as the focus window for keyboard input
        SetFocus(_hWnd);
        _properties.set_foreground(true);
      }
  }


  if (!_hWnd) {
    windisplay_cat.error()
      << "CreateWindow() failed!" << endl;
    show_error_message();
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::adjust_z_order
//       Access: Private
//  Description: Adjusts the Z-order of a window after it has been
//               moved.
////////////////////////////////////////////////////////////////////
void WinGraphicsWindow::
adjust_z_order() {
  WindowProperties::ZOrder z_order = _properties.get_z_order();
  adjust_z_order(z_order, z_order);
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::adjust_z_order
//       Access: Private
//  Description: Adjusts the Z-order of a window after it has been
//               moved.
////////////////////////////////////////////////////////////////////
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
        // If we aren't changing the window order, don't move it to
        // the top.
        (last_z_order != WindowProperties::Z_bottom ||
         _properties.get_foreground())
        // If the window was previously on the bottom, but it doesn't
        // have focus now, don't move it to the top; it will get moved
        // the next time we get focus.
        ) {
      order = HWND_TOP;
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

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::track_mouse_leaving
//       Access: Private
//  Description: Intended to be called whenever mouse motion is
//               detected within the window, this indicates that the
//               mouse is within the window and tells Windows that we
//               want to be told when the mouse leaves the window.
////////////////////////////////////////////////////////////////////
void WinGraphicsWindow::
track_mouse_leaving(HWND hwnd) {
  // Note: could use _TrackMouseEvent in comctrl32.dll (part of IE
  // 3.0+) which emulates TrackMouseEvent on w95, but that requires
  // another 500K of memory to hold that DLL, which is lame just to
  // support w95, which probably has other issues anyway
  WinGraphicsPipe *winpipe;
  DCAST_INTO_V(winpipe, _pipe);

  if (winpipe->_pfnTrackMouseEvent != NULL) {
    TRACKMOUSEEVENT tme = {
      sizeof(TRACKMOUSEEVENT),
      TME_LEAVE,
      hwnd,
      0
    };

    // tell win32 to post WM_MOUSELEAVE msgs
    BOOL bSucceeded = winpipe->_pfnTrackMouseEvent(&tme);  
    
    if ((!bSucceeded) && windisplay_cat.is_debug()) {
      windisplay_cat.debug()
        << "TrackMouseEvent failed!, LastError=" << GetLastError() << endl;
    }
    
    _tracking_mouse_leaving = true;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::window_proc
//       Access: Private
//  Description: This is the nonstatic window_proc function.  It is
//               called to handle window events for this particular
//               window.
////////////////////////////////////////////////////////////////////
LONG WinGraphicsWindow::
window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  if (windisplay_cat.is_spam()) {
    windisplay_cat.spam()
      << ClockObject::get_global_clock()->get_real_time() 
      << " window_proc(" << (void *)this << ", " << hwnd << ", "
      << msg << ", " << wparam << ", " << lparam << ")\n";
  }
  WindowProperties properties;
  int button = -1;

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
        
      case WM_CLOSE:
        // This is a message from the system indicating that the user
        // has requested to close the window (e.g. alt-f4).
        {
          string close_request_event = get_close_request_event();
          if (!close_request_event.empty()) {
            // In this case, the app has indicated a desire to intercept
            // the request and process it directly.
            throw_event(close_request_event);
            return 0;
            
          } else {
            // In this case, the default case, the app does not intend
            // to service the request, so we do by closing the window.
            close_window();
            properties.set_open(false);
            system_changed_properties(properties);
            
            // TODO: make sure we release the GSG properly.
          }
        }
        break;
    
      case WM_ACTIVATE:
        properties.set_minimized((wparam & 0xffff0000) != 0);
        if ((wparam & 0xffff) != WA_INACTIVE) 
        {
          properties.set_foreground(true);
          if (is_fullscreen()) 
          {
            // When a fullscreen window goes active, it automatically gets
            // un-minimized.
            ChangeDisplaySettings(&_fullscreen_display_mode, CDS_FULLSCREEN);
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
            // When a fullscreen window goes inactive, it automatically
            // gets minimized.
            properties.set_minimized(true);
    
            // It seems order is important here.  We must minimize the
            // window before restoring the display settings, or risk
            // losing the graphics context.
            ShowWindow(_hWnd, SW_MINIMIZE);
            GdiFlush();
            ChangeDisplaySettings(NULL, 0x0);
            fullscreen_minimized(properties);
          }
        }

        adjust_z_order();
        system_changed_properties(properties);
        break;
    
      case WM_SIZE:
        // for maximized, unmaximize, need to call resize code
        // artificially since no WM_EXITSIZEMOVE is generated.
        if (wparam == SIZE_MAXIMIZED) {
          _maximized = true;
          handle_reshape();
    
        } else if (wparam == SIZE_RESTORED && _maximized) {
          // SIZE_RESTORED might mean we restored to its original size
          // before the maximize, but it might also be called while the
          // user is resizing the window by hand.  Checking the _maximized
          // flag that we set above allows us to differentiate the two
          // cases.
          _maximized = false;
          handle_reshape();
        }
        break;
    
      case WM_EXITSIZEMOVE:
        handle_reshape();
        break;

      case WM_WINDOWPOSCHANGED:
        adjust_z_order();
        break;
    
      case WM_LBUTTONDOWN:
        if (_lost_keypresses) {
          resend_lost_keypresses();
        }
        SetCapture(hwnd);
        _input_devices[0].set_pointer_in_window(translate_mouse(LOWORD(lparam)), translate_mouse(HIWORD(lparam)));
        _input_devices[0].button_down(MouseButton::button(0), get_message_time());
        break;
        
      case WM_MBUTTONDOWN:
        if (_lost_keypresses) {
          resend_lost_keypresses();
        }
        SetCapture(hwnd);
        _input_devices[0].set_pointer_in_window(translate_mouse(LOWORD(lparam)), translate_mouse(HIWORD(lparam)));
        _input_devices[0].button_down(MouseButton::button(1), get_message_time());
        break;

      case WM_RBUTTONDOWN:
        if (_lost_keypresses) {
          resend_lost_keypresses();
        }
        SetCapture(hwnd);
        _input_devices[0].set_pointer_in_window(translate_mouse(LOWORD(lparam)), translate_mouse(HIWORD(lparam)));
        _input_devices[0].button_down(MouseButton::button(2), get_message_time());
        break;

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
        break;
    
      case WM_LBUTTONUP:
        if (_lost_keypresses) {
          resend_lost_keypresses();
        }
        ReleaseCapture();
        _input_devices[0].button_up(MouseButton::button(0), get_message_time());
        break;

      case WM_MBUTTONUP:
        if (_lost_keypresses) {
          resend_lost_keypresses();
        }
        ReleaseCapture();
        _input_devices[0].button_up(MouseButton::button(1), get_message_time());
        break;

      case WM_RBUTTONUP:
        if (_lost_keypresses) {
          resend_lost_keypresses();
        }
        ReleaseCapture();
        _input_devices[0].button_up(MouseButton::button(2), get_message_time());
        break;

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
        break;

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
        //if (::SendMessage(hwnd, WM_IME_CONTROL, IMC_CLOSESTATUSWINDOW, 0))
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
            //if (0) {
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
        /*
        else if (_ime_open && (wparam == IMN_OPENCANDIDATE)) {
          HIMC hIMC = ImmGetContext(hwnd);
          nassertr(hIMC != 0, 0);
          DWORD need_byte;
          LPCANDIDATELIST pCanList;
          char pBuff[1024];
          need_byte = ImmGetCandidateListW(hIMC, 0, NULL, 0);
          windisplay_cat.debug() << "need byte " << need_byte << endl;
          need_byte = ImmGetCandidateListW(hIMC, 0, (LPCANDIDATELIST)pBuff, need_byte);
          pCanList = (LPCANDIDATELIST)pBuff;
          windisplay_cat.debug() << "need byte " << need_byte << " ,size " << pCanList->dwSize 
                                 << " ,count " << pCanList->dwCount
                                 << " ,selected " << pCanList->dwSelection << endl;
          wstring candidate_str;
          for (DWORD i=0; i<pCanList->dwCount; ++i) {
            // lets append all the candidate strings together
            windisplay_cat.debug() << "string offset " << pCanList->dwOffset[i] << endl;
            candidate_str.append((wchar_t*)((char*)pCanList + pCanList->dwOffset[i]));
          }
          windisplay_cat.debug() << "concatenated string " << (wchar_t*)candidate_str.c_str() << endl;
          //_input_devices[0].candidate(candidate_str, 0, 0);
          ImmReleaseContext(hwnd, hIMC);
        }
        */
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

          // If the ime window is not marked as active at this point, we
          // must be in the process of closing it down (in close_ime), and
          // we don't want to send the current composition string in that
          // case.  But we do need to return 0 to tell windows not to try
          // to send the composition string through WM_CHAR messages.
          if (!_ime_active)
            return 0;

          HIMC hIMC = ImmGetContext(hwnd);
          nassertr(hIMC != 0, 0);
          
          static const int max_ime_result = 128;
          static char ime_result[max_ime_result];
          
          DWORD result_size = 0;
          const int max_t = 256;
          wchar_t can_t[max_t];
          size_t cursor_pos, delta_start;
          
          if (_ime_composition_w) {
            // Since ImmGetCompositionStringA() doesn't seem to work
            // for Win2000 (it always returns question mark
            // characters), we have to use ImmGetCompositionStringW()
            // on this OS.  This is actually the easier of the two
            // functions to use.
            
            if (lparam & GCS_RESULTSTR) {
              windisplay_cat.debug() << "GCS_RESULTSTR\n";
              result_size = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR,
                                                     ime_result, max_ime_result);
              
              // Add this string into the text buffer of the application.
              
              // ImmGetCompositionStringW() returns a string, but it's
              // filled in with wstring data: every two characters defines a
              // 16-bit unicode char.  The docs aren't clear on the
              // endianness of this.  I guess it's safe to assume all Win32
              // machines are little-endian.
              for (DWORD i = 0; i < result_size; i += 2) {
                int result =
                  ((int)(unsigned char)ime_result[i + 1] << 8) |
                  (unsigned char)ime_result[i];
                _input_devices[0].keystroke(result);
              }
            }
            if (lparam & GCS_COMPSTR) {
              windisplay_cat.debug() << "GCS_COMPSTR\n";
              /*
                result_size = ImmGetCompositionStringW(hIMC, GCS_COMPREADSTR, can_t, max_t);
                windisplay_cat.debug() << "got readstr of size " << result_size << endl;
              */
              
              result_size = ImmGetCompositionStringW(hIMC, GCS_CURSORPOS, can_t, max_t);
              cursor_pos = result_size&0xffff;
              windisplay_cat.debug() << "got cursorpos at " << cursor_pos  << endl;
              
              result_size = ImmGetCompositionStringW(hIMC, GCS_DELTASTART, can_t, max_t);
              delta_start = result_size&0xffff;
              windisplay_cat.debug() << "got deltastart at " << delta_start << endl;
              
              /*
                result_size = ImmGetCompositionStringW(hIMC, GCS_COMPATTR, can_t, max_t);
                windisplay_cat.debug() << "got compatr of size " << result_size << endl;
                
                result_size = ImmGetCompositionStringW(hIMC, GCS_COMPREADSTR, can_t, max_t);
                windisplay_cat.debug() << "got compreadstr of size " << result_size << endl;
                
                result_size = ImmGetCompositionStringW(hIMC, GCS_COMPCLAUSE, can_t, max_t);
                windisplay_cat.debug() << "got compclause of size " << result_size << endl;
              */
              
              result_size = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, can_t, max_t);
              windisplay_cat.debug() << "got compstr of size " << result_size << endl;
              
              can_t[result_size/sizeof(wchar_t)] = '\0';
              
              _input_devices[0].candidate(can_t, min(cursor_pos, delta_start), max(cursor_pos, delta_start), cursor_pos);
            }
          } else {
            if (lparam & GCS_RESULTSTR) {
              // On the other hand, ImmGetCompositionStringW() doesn't
              // work on Win95 or Win98; for these OS's we must use
              // ImmGetCompositionStringA().
              result_size = ImmGetCompositionStringA(hIMC, GCS_RESULTSTR,
                                                     ime_result, max_ime_result);
              
              // ImmGetCompositionStringA() returns an encoded ANSI
              // string, which we now have to map to wide-character
              // Unicode.
              static const int max_wide_result = 128;
              static wchar_t wide_result[max_wide_result];
              
              int wide_size =
                MultiByteToWideChar(CP_ACP, 0,
                                    ime_result, result_size,
                                    wide_result, max_wide_result);
              if (wide_size == 0) {
                show_error_message();
              }
              for (int i = 0; i < wide_size; i++) {
                _input_devices[0].keystroke(wide_result[i]);
              }
            }
          }
          
          ImmReleaseContext(hwnd, hIMC);
        }
        break;
        
      case WM_CHAR:
        // Ignore WM_CHAR messages if we have the IME open, since
        // everything will come in through WM_IME_COMPOSITION.  (It's
        // supposed to come in through WM_CHAR, too, but there seems to
        // be a bug in Win2000 in that it only sends question mark
        // characters through here.)
        if (!_ime_open) {
          _input_devices[0].keystroke(wparam);
        }
        break;
        /*
     case WM_PAINT:
       // draw ime window on top
       PAINTSTRUCT ps; 
       HDC hdc; 
       //RECT rc; 
       hdc = BeginPaint(hwnd, &ps); 
       EndPaint(hwnd, &ps); 
       if (_ime_active) {
         hdc = BeginPaint(_ime_hWnd, &ps);
         EndPaint(_ime_hWnd, &ps);
       }
       break;       
        */
    
      case WM_SYSKEYDOWN: 
        if (_lost_keypresses) {
          resend_lost_keypresses();
        }
        if (windisplay_cat.is_debug()) {
          windisplay_cat.debug()
            << "keydown: " << wparam << " (" << lookup_key(wparam) << ")\n";
        }
        {
          // Alt and F10 are sent as WM_SYSKEYDOWN instead of WM_KEYDOWN
          // want to use defwindproc on Alt syskey so std windows cmd
          // Alt-F4 works, etc
          POINT point;
          GetCursorPos(&point);
          ScreenToClient(hwnd, &point);
          handle_keypress(lookup_key(wparam), point.x, point.y, 
                          get_message_time());

          // wparam does not contain left/right information for SHIFT,
          // CONTROL, or ALT, so we have to track their status and do
          // the right thing.  We'll send the left/right specific key
          // event along with the general key event.
          //
          // Key repeating is not being handled consistently for LALT
          // and RALT, but from comments below, it's only being handled
          // the way it is for backspace, so we'll leave it as is.
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
            // bypass default windproc F10 behavior (it activates the main
            // menu, but we have none)
            return 0;
          }
        }
        break;
    
      case WM_SYSCOMMAND:
        if (wparam == SC_KEYMENU) {
          // if Alt is released (alone w/o other keys), defwindproc will
          // send this command, which will 'activate' the title bar menu
          // (we have none) and give focus to it.  we dont want this to
          // happen, so kill this msg.

          // Note that the WM_SYSKEYUP message for Alt has already
          // been sent (if it is going to be), so ignoring this
          // special message does no harm.
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

        // If this bit is not zero, this is just a keyrepeat echo; we
        // ignore these for handle_keypress (we respect keyrepeat only
        // for handle_keystroke).
        if ((lparam & 0x40000000) == 0) {
          POINT point;
          GetCursorPos(&point);
          ScreenToClient(hwnd, &point);
          handle_keypress(lookup_key(wparam), point.x, point.y,
                          get_message_time());

          // wparam does not contain left/right information for SHIFT,
          // CONTROL, or ALT, so we have to track their status and do
          // the right thing.  We'll send the left/right specific key
          // event along with the general key event.
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

          // Handle Cntrl-V paste from clipboard.  Is there a better way
          // to detect this hotkey?
          if ((wparam=='V') && (GetKeyState(VK_CONTROL) < 0) &&
              !_input_devices.empty()) {
            HGLOBAL hglb;
            char *lptstr;
    
            if (IsClipboardFormatAvailable(CF_TEXT) && OpenClipboard(NULL)) {
              // Maybe we should support CF_UNICODETEXT if it is available
              // too?
              hglb = GetClipboardData(CF_TEXT);
              if (hglb!=NULL) {
                lptstr = (char *) GlobalLock(hglb);
                if (lptstr != NULL)  {
                  char *pChar;
                  for (pChar=lptstr; *pChar!=NULL; pChar++) {
                    _input_devices[0].keystroke((uchar)*pChar);
                  }
                  GlobalUnlock(hglb);
                }
              }
              CloseClipboard();
            }
          }
        } else {
          // Actually, for now we'll respect the repeat anyway, just
          // so we support backspace properly.  Rethink later.
          POINT point;
          GetCursorPos(&point);
          ScreenToClient(hwnd, &point);
          handle_keypress(lookup_key(wparam), point.x, point.y,
                          get_message_time());

          // wparam does not contain left/right information for SHIFT,
          // CONTROL, or ALT, so we have to track their status and do
          // the right thing.  We'll send the left/right specific key
          // event along with the general key event.
          //
          // If the user presses LSHIFT and then RSHIFT, the RSHIFT event
          // will come in with the keyrepeat flag on (i.e. it will end up
          // in this block).  The logic below should detect this correctly
          // and only send the RSHIFT event.  Note that the CONTROL event
          // will be sent twice, once for each keypress.  Since keyrepeats
          // are currently being sent simply as additional keypress events,
          // that should be okay for now.
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

        // wparam does not contain left/right information for SHIFT,
        // CONTROL, or ALT, so we have to track their status and do
        // the right thing.  We'll send the left/right specific key
        // event along with the general key event.
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
        if (windisplay_cat.is_debug()) 
        {
          windisplay_cat.debug()
            << "killfocus\n";
        }
        if (!_lost_keypresses) 
        {
          // Record the current state of the keyboard when the focus is
          // lost, so we can check it for changes when we regain focus.
          GetKeyboardState(_keyboard_state);
          if (windisplay_cat.is_debug()) {
            // Report the set of keys that are held down at the time of
            // the killfocus event.
            for (int i = 0; i < num_virtual_keys; i++) 
            {
              if (i != VK_SHIFT && i != VK_CONTROL && i != VK_MENU) 
              {
                if ((_keyboard_state[i] & 0x80) != 0) 
                {
                  windisplay_cat.debug()
                    << "on killfocus, key is down: " << i
                    << " (" << lookup_key(i) << ")\n";
                }
              }
            }
          }

          if (!hold_keys_across_windows) 
          {
            // If we don't want to remember the keystate while the
            // window focus is lost, then generate a keyup event
            // right now for each key currently held.
            double message_time = get_message_time();
            for (int i = 0; i < num_virtual_keys; i++) 
            {
              if (i != VK_SHIFT && i != VK_CONTROL && i != VK_MENU) 
              {
                if ((_keyboard_state[i] & 0x80) != 0) 
                {
                  handle_keyrelease(lookup_key(i), message_time);
                  _keyboard_state[i] &= ~0x80;
                }
              }
            }
          }
          
          // Now set the flag indicating that some keypresses from now
          // on may be lost.
          _lost_keypresses = true;
        }
        break;
    
      case WM_SETFOCUS: 
        // You would think that this would be a good time to call
        // resend_lost_keypresses(), but it turns out that we get
        // WM_SETFOCUS slightly before Windows starts resending key
        // up/down events to us.

        // In particular, if the user restored focus using alt-tab,
        // then at this point the keyboard state will indicate that
        // both the alt and tab keys are held down.  However, there is
        // a small window of opportunity for the user to release these
        // keys before Windows starts telling us about keyup events.
        // Thus, if we record the fact that alt and tab are being held
        // down now, we may miss the keyup events for them, and they
        // can get "stuck" down.

        // So we have to defer calling resend_lost_keypresses() until
        // we know Windows is ready to send us key up/down events.  I
        // don't know when we can guarantee that, except when we
        // actually do start to receive key up/down events, so that
        // call is made there.

        if (windisplay_cat.is_debug()) {
          windisplay_cat.debug()
            << "setfocus\n";
        }

        if (_lost_keypresses) {
          resend_lost_keypresses();
        }
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
  }

  return DefWindowProc(hwnd, msg, wparam, lparam);
}


////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::static_window_proc
//       Access: Private, Static
//  Description: This is attached to the window class for all
//               WinGraphicsWindow windows; it is called to handle
//               window events.
////////////////////////////////////////////////////////////////////
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
  return DefWindowProc(hwnd, msg, wparam, lparam);
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::process_1_event
//       Access: Private, Static
//  Description: Handles one event from the message queue.
////////////////////////////////////////////////////////////////////
void WinGraphicsWindow::
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

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::resend_lost_keypresses
//       Access: Private, Static
//  Description: Called when the keyboard focus has been restored to
//               the window after it has been lost for a time, this
//               rechecks the keyboard state and generates key up/down
//               messages for keys that have changed state in the
//               meantime.
////////////////////////////////////////////////////////////////////
void WinGraphicsWindow::
resend_lost_keypresses() {
  nassertv(_lost_keypresses);
  if (windisplay_cat.is_debug()) {
    windisplay_cat.debug()
      << "resending lost keypresses\n";
  }

  BYTE new_keyboard_state[num_virtual_keys];
  GetKeyboardState(new_keyboard_state);

  double message_time = get_message_time();
  for (int i = 0; i < num_virtual_keys; i++) {
    // Filter out these particular three.  We don't want to test
    // these, because these are virtual duplicates for
    // VK_LSHIFT/VK_RSHIFT, etc.; and the left/right equivalent is
    // also in the table.  If we respect both VK_LSHIFT as well as
    // VK_SHIFT, we'll generate two keyboard messages when
    // VK_LSHIFT changes state.
    if (i != VK_SHIFT && i != VK_CONTROL && i != VK_MENU) {
      if (((new_keyboard_state[i] ^ _keyboard_state[i]) & 0x80) != 0) {
        // This key has changed state.
        if ((new_keyboard_state[i] & 0x80) != 0) {
          // The key is now held down.
          if (windisplay_cat.is_debug()) {
            windisplay_cat.debug()
              << "key has gone down: " << i << " (" << lookup_key(i) << ")\n";
          }
          
          // Roger
          //handle_keyresume(lookup_key(i), message_time);
          // resume does not seem to work and sending the pointer position seems to 
          // weird ot some cursor controls
           ButtonHandle key = lookup_key(i);
           if (key != ButtonHandle::none())
                _input_devices[0].button_down(key, message_time);


        } else {
          // The key is now released.
          if (windisplay_cat.is_debug()) {
            windisplay_cat.debug()
              << "key has gone up: " << i << " (" << lookup_key(i) << ")\n";
          }
          handle_keyrelease(lookup_key(i), message_time);
        }
      } else {
        // This key is in the same state.
        if (windisplay_cat.is_debug()) {
          if ((new_keyboard_state[i] & 0x80) != 0) {
            windisplay_cat.debug()
              << "key is still down: " << i << " (" << lookup_key(i) << ")\n";
          }
        }
      }
    }
  }

  // Keypresses are no longer lost.
  _lost_keypresses = false;
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::update_cursor_window
//       Access: Private, Static
//  Description: Changes _cursor_window from its current value to the
//               indicated value.  This also changes the cursor
//               properties appropriately.
////////////////////////////////////////////////////////////////////
void WinGraphicsWindow::
update_cursor_window(WinGraphicsWindow *to_window) {
  bool hide_cursor = false;
  if (to_window == (WinGraphicsWindow *)NULL) {
    // We are leaving a graphics window; we should restore the Win2000
    // effects.
    if (_got_saved_params) {
      SystemParametersInfo(SPI_SETMOUSETRAILS, NULL, 
                           (PVOID)_saved_mouse_trails, NULL);
      SystemParametersInfo(SPI_SETCURSORSHADOW, NULL, 
                           (PVOID)_saved_cursor_shadow, NULL);
      SystemParametersInfo(SPI_SETMOUSEVANISH, NULL,
                           (PVOID)_saved_mouse_vanish, NULL);
      _got_saved_params = false;
    }

  } else {
    const WindowProperties &to_props = to_window->get_properties();
    hide_cursor = to_props.get_cursor_hidden();

    // We are entering a graphics window; we should save and disable
    // the Win2000 effects.  These don't work at all well over a 3-D
    // window.

    // These parameters are only defined for Win2000/XP, but they
    // should just cause a silent error on earlier OS's, which is OK.
    if (!_got_saved_params) {
      SystemParametersInfo(SPI_GETMOUSETRAILS, NULL, 
                           &_saved_mouse_trails, NULL);
      SystemParametersInfo(SPI_GETCURSORSHADOW, NULL, 
                           &_saved_cursor_shadow, NULL);
      SystemParametersInfo(SPI_GETMOUSEVANISH, NULL, 
                           &_saved_mouse_vanish, NULL);
      _got_saved_params = true;

      SystemParametersInfo(SPI_SETMOUSETRAILS, NULL, (PVOID)0, NULL);
      SystemParametersInfo(SPI_SETCURSORSHADOW, NULL, (PVOID)false, NULL);
      SystemParametersInfo(SPI_SETMOUSEVANISH, NULL, (PVOID)false, NULL);
    }

    SetCursor(to_window->_cursor);
  }
  
  hide_or_show_cursor(hide_cursor);

  _cursor_window = to_window;
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::hide_or_show_cursor
//       Access: Private, Static
//  Description: Hides or shows the mouse cursor according to the
//               indicated parameter.  This is normally called when
//               the mouse wanders into or out of a window with the
//               cursor_hidden properties.
////////////////////////////////////////////////////////////////////
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

// dont pick any video modes < MIN_REFRESH_RATE Hz
#define MIN_REFRESH_RATE 60
// EnumDisplaySettings may indicate 0 or 1 for refresh rate, which means use driver default rate (assume its >min_refresh_rate)
#define ACCEPTABLE_REFRESH_RATE(RATE) ((RATE >= MIN_REFRESH_RATE) || (RATE==0) || (RATE==1))

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::find_acceptable_display_mode
//       Access: Private, Static
//  Description: Looks for a fullscreen mode that meets the specified
//               size and bitdepth requirements.  Returns true if a
//               suitable mode is found, false otherwise.
////////////////////////////////////////////////////////////////////
bool WinGraphicsWindow::
find_acceptable_display_mode(DWORD dwWidth, DWORD dwHeight, DWORD bpp,
                             DEVMODE &dm) {
  int modenum = 0;

  while (1) {
    ZeroMemory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);
    
    if (!EnumDisplaySettings(NULL, modenum, &dm)) {
      break;
    }
    
    if ((dm.dmPelsWidth == dwWidth) && (dm.dmPelsHeight == dwHeight) &&
        (dm.dmBitsPerPel == bpp) && 
        ACCEPTABLE_REFRESH_RATE(dm.dmDisplayFrequency)) {
      return true;
    }
    modenum++;
  }
  
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::show_error_message
//       Access: Private, Static
//  Description: Pops up a dialog box with the indicated Windows error
//               message ID (or the last error message generated) for
//               meaningful display to the user.
////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::lookup_key
//       Access: Private
//  Description: Translates the keycode reported by Windows to an
//               appropriate Panda ButtonHandle.
////////////////////////////////////////////////////////////////////
ButtonHandle WinGraphicsWindow::
lookup_key(WPARAM wparam) const {
  // First, check for a few buttons that we filter out when the IME
  // window is open.
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

  // Now check for the rest of the buttons, including the ones that
  // we allow through even when the IME window is open.
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
      // We used to try to remap lowercase to uppercase keys
      // here based on the state of the shift and/or caps lock
      // keys.  But that's a mistake, and doesn't allow for
      // international or user-defined keyboards; let Windows
      // do that mapping.

      // Nowadays, we make a distinction between a "button"
      // and a "keystroke".  A button corresponds to a
      // physical button on the keyboard and has a down and up
      // event associated.  A keystroke may or may not
      // correspond to a physical button, but will be some
      // Unicode character and will not have a corresponding
      // up event.
      return KeyboardButton::ascii_key(tolower(key));
    }
    break;
  }
  return ButtonHandle::none();
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::handle_raw_input
//       Access: Private
//  Description:
////////////////////////////////////////////////////////////////////
void WinGraphicsWindow::
handle_raw_input(HRAWINPUT hraw) {
  LPBYTE lpb;
  UINT dwSize;

  if (hraw == 0) {
    return;
  }
  if (pGetRawInputData(hraw, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER)) == -1) {
    return;
  }
  
  lpb = (LPBYTE)alloca(sizeof(LPBYTE) * dwSize);
  if (lpb == NULL) {
    return;
  }
  
  if (pGetRawInputData(hraw, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
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
        int oldx = _input_devices[i].get_pointer().get_x();
        int oldy = _input_devices[i].get_pointer().get_y();
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

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::handle_mouse_motion
//       Access: Private
//  Description:
////////////////////////////////////////////////////////////////////
bool WinGraphicsWindow::
handle_mouse_motion(int x, int y) {
  _input_devices[0].set_pointer_in_window(x, y);
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::handle_mouse_exit
//       Access: Private
//  Description:
////////////////////////////////////////////////////////////////////
void WinGraphicsWindow::
handle_mouse_exit() {
  // note: 'mouse_motion' is considered the 'entry' event
  _input_devices[0].set_pointer_out_of_window();
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::get_icon
//       Access: Private, Static
//  Description: Loads and returns an HICON corresponding to the
//               indicated filename.  If the file cannot be loaded,
//               returns 0.
////////////////////////////////////////////////////////////////////
HICON WinGraphicsWindow::
get_icon(const Filename &filename) {
  // First, look for the unresolved filename in our index.
  IconFilenames::iterator fi = _icon_filenames.find(filename);
  if (fi != _icon_filenames.end()) {
    return (HICON)((*fi).second);
  }

  // If it wasn't found, resolve the filename and search for that.

  // Since we have to use a Windows call to load the image from a
  // filename, we can't load a virtual file and we can't use the
  // virtual file system.
  Filename resolved = filename;
  if (!resolved.resolve_filename(model_path)) {
    // The filename doesn't exist.
    windisplay_cat.warning()
      << "Could not find icon filename " << filename << "\n";
    return 0;
  }
  fi = _icon_filenames.find(resolved);
  if (fi != _icon_filenames.end()) {
    _icon_filenames[filename] = (*fi).second;
    return (HICON)((*fi).second);
  }

  Filename os = resolved.to_os_specific();

  HANDLE h = LoadImage(NULL, os.c_str(),
                       IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
  if (h == 0) {
    windisplay_cat.warning()
      << "windows icon filename '" << os << "' could not be loaded!!\n";
  }

  _icon_filenames[filename] = h;
  _icon_filenames[resolved] = h;
  return (HICON)h;
}

////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::get_cursor
//       Access: Private, Static
//  Description: Loads and returns an HCURSOR corresponding to the
//               indicated filename.  If the file cannot be loaded,
//               returns 0.
////////////////////////////////////////////////////////////////////
HCURSOR WinGraphicsWindow::
get_cursor(const Filename &filename) {
  // First, look for the unresolved filename in our index.
  IconFilenames::iterator fi = _cursor_filenames.find(filename);
  if (fi != _cursor_filenames.end()) {
    return (HCURSOR)((*fi).second);
  }

  // If it wasn't found, resolve the filename and search for that.

  // Since we have to use a Windows call to load the image from a
  // filename, we can't load a virtual file and we can't use the
  // virtual file system.
  Filename resolved = filename;
  if (!resolved.resolve_filename(model_path)) {
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
  
////////////////////////////////////////////////////////////////////
//     Function: WinGraphicsWindow::register_window_class
//       Access: Private, Static
//  Description: Registers a Window class appropriate for the
//               indicated properties.  This class may be shared by
//               multiple windows.
////////////////////////////////////////////////////////////////////
const WinGraphicsWindow::WindowClass &WinGraphicsWindow::
register_window_class(const WindowProperties &props) {
  pair<WindowClasses::iterator, bool> found = 
    _window_classes.insert(WindowClass(props));
  WindowClass &wclass = (*found.first);

  if (!found.second) {
    // We have already created a window class.
    return wclass;
  }

  // We have not yet created this window class.
  ostringstream wclass_name;
  wclass_name << "WinGraphicsWindow" << _window_class_index;
  _window_class_index++;
  wclass._name = wclass_name.str();

  WNDCLASS wc;

  HINSTANCE instance = GetModuleHandle(NULL);

  // Clear before filling in window structure!
  ZeroMemory(&wc, sizeof(WNDCLASS));
  wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wc.lpfnWndProc = (WNDPROC)static_window_proc;
  wc.hInstance = instance;

  wc.hIcon = wclass._icon;

  wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  wc.lpszMenuName = NULL;
  wc.lpszClassName = wclass._name.c_str();
  
  if (!RegisterClass(&wc)) {
    windisplay_cat.error()
      << "could not register window class " << wclass._name << "!" << endl;
    return wclass;
  }

  return wclass;
}

// pops up MsgBox w/system error msg
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

////////////////////////////////////////////////////////////////////
//     Function: get_client_rect_screen
//  Description: Fills view_rect with the coordinates of the client
//               area of the indicated window, converted to screen
//               coordinates.
////////////////////////////////////////////////////////////////////
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


