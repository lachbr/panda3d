// Filename: pandaFramework.cxx
// Created by:  drose (02Apr02)
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

#include "pandaFramework.h"
#include "clockObject.h"
#include "pStatClient.h"
#include "eventQueue.h"
#include "dataGraphTraverser.h"
#include "interactiveGraphicsPipe.h"
#include "collisionNode.h"
#include "config_framework.h"

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
PandaFramework::
PandaFramework() :
  _event_handler(EventQueue::get_global_event_queue())
{
  _is_open = false;
  _made_default_pipe = false;
  _data_root = NodePath("data");
  _window_title = "Panda";
  _start_time = 0.0;
  _frame_count = 0;
  _wireframe_enabled = false;
  _texture_enabled = true;
  _two_sided_enabled = false;
  _lighting_enabled = false;
  _default_keys_enabled = false;
  _exit_flag = false;
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::Destructor
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
PandaFramework::
~PandaFramework() {
  if (_is_open) {
    close_framework();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::open_framework
//       Access: Public
//  Description: Should be called once at the beginning of the
//               application to initialize Panda (and the framework)
//               for use.  The command-line arguments should be passed
//               in so Panda can remove any arguments that it
//               recognizes as special control parameters.
////////////////////////////////////////////////////////////////////
void PandaFramework::
open_framework(int &argc, char **&argv) {
  if (_is_open) {
    return;
  }

  _is_open = true;

  reset_frame_rate();
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::close_framework
//       Access: Public
//  Description: Should be called at the end of an application to
//               close Panda.  This is optional, as the destructor
//               will do the same thing.
////////////////////////////////////////////////////////////////////
void PandaFramework::
close_framework() {
  if (!_is_open) {
    return;
  }

  close_all_windows();
  // We should define this function on GraphicsEngine.
  //  _engine.remove_all_windows();
  _event_handler.remove_all_hooks();

  _is_open = false;
  _made_default_pipe = false;
  _default_pipe.clear();

  _start_time = 0.0;
  _frame_count = 0;
  _wireframe_enabled = false;
  _two_sided_enabled = false;
  _lighting_enabled = false;
  _default_keys_enabled = false;
  _exit_flag = false;
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::get_default_pipe
//       Access: Public
//  Description: Returns the default pipe.  This is the GraphicsPipe
//               that all windows in the framework will be created on,
//               unless otherwise specified in open_window().  It is
//               usually the primary graphics interface on the local
//               machine.
//
//               If the default pipe has not yet been created, this
//               creates it.
//
//               The return value is the default pipe, or NULL if no
//               default pipe could be created.
////////////////////////////////////////////////////////////////////
GraphicsPipe *PandaFramework::
get_default_pipe() {
  nassertr(_is_open, NULL);
  if (!_made_default_pipe) {
    make_default_pipe();
    _made_default_pipe = true;
  }
  return _default_pipe;
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::get_default_window_props
//       Access: Public, Virtual
//  Description: Fills in the indicated window properties structure
//               according to the normal window properties for this
//               application.
////////////////////////////////////////////////////////////////////
void PandaFramework::
get_default_window_props(GraphicsWindow::Properties &props) {
  props._xorg = 0;
  props._yorg = 0;
  props._xsize = win_width;
  props._ysize = win_height;
  props._fullscreen = fullscreen;
  props._title = _window_title;

  props.set_clear_color(Colorf(win_background_r, win_background_g, 
                               win_background_b, 1.0f));
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::open_window
//       Access: Public
//  Description: Opens a new window, using the default parameters.
//               Returns the new WindowFramework if successful, or
//               NULL if not.
////////////////////////////////////////////////////////////////////
WindowFramework *PandaFramework::
open_window(GraphicsPipe *pipe) {
  nassertr(_is_open, NULL);

  GraphicsWindow::Properties props;
  get_default_window_props(props);

  return open_window(props, pipe);
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::open_window
//       Access: Public
//  Description: Opens a new window using the indicated properties.
//               (You may initialize the properties to their default
//               values by calling get_default_window_props() first.)
//
//               Returns the new WindowFramework if successful, or
//               NULL if not.
////////////////////////////////////////////////////////////////////
WindowFramework *PandaFramework::
open_window(const GraphicsWindow::Properties &props, GraphicsPipe *pipe) {
  if (pipe == (GraphicsPipe *)NULL) {
    pipe = get_default_pipe();
    if (pipe == (GraphicsPipe *)NULL) {
      // Can't get a pipe.
      return NULL;
    }
  }

  nassertr(_is_open, NULL);
  WindowFramework *wf = make_window_framework();
  _windows.push_back(wf);

  GraphicsWindow *win = wf->open_window(props, pipe);
  if (win != (GraphicsWindow *)NULL) {
    _engine.add_window(win);
  }

  return wf;
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::close_window
//       Access: Public
//  Description: Closes the nth window and removes it from the list.
////////////////////////////////////////////////////////////////////
void PandaFramework::
close_window(int n) {
  nassertv(n >= 0 && n < (int)_windows.size());
  WindowFramework *wf = _windows[n];

  GraphicsWindow *win = wf->get_graphics_window();
  if (win != (GraphicsWindow *)NULL) {
    _engine.remove_window(win);
  }

  wf->close_window();
  delete wf;
  _windows.erase(_windows.begin() + n);
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::close_all_windows
//       Access: Public
//  Description: Closes all currently open windows and empties the
//               list of windows.
////////////////////////////////////////////////////////////////////
void PandaFramework::
close_all_windows() {
  Windows::iterator wi;
  for (wi = _windows.begin(); wi != _windows.end(); ++wi) {
    WindowFramework *wf = (*wi);

    GraphicsWindow *win = wf->get_graphics_window();
    if (win != (GraphicsWindow *)NULL) {
      _engine.remove_window(win);
    }
    
    wf->close_window();
    delete wf;
  }

  _windows.clear();
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::get_models
//       Access: Public
//  Description: Returns the root of the scene graph normally reserved
//               for parenting models and such.  This scene graph may
//               be instanced to each window's render tree as the
//               window is created.
////////////////////////////////////////////////////////////////////
const NodePath &PandaFramework::
get_models() {
  if (_models.is_empty()) {
    _models = NodePath("models");
  }
  return _models;
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::report_frame_rate
//       Access: Public
//  Description: Reports the currently measured average frame rate to
//               the indicated ostream.
////////////////////////////////////////////////////////////////////
void PandaFramework::
report_frame_rate(ostream &out) const {
  double now = ClockObject::get_global_clock()->get_frame_time();
  double delta = now - _start_time;
  
  int frame_count = ClockObject::get_global_clock()->get_frame_count();
  int num_frames = frame_count - _frame_count;
  if (num_frames > 0) {
    out << num_frames << " frames in " << delta << " seconds.\n";
    double fps = ((double)num_frames) / delta;
    out << fps << " fps average (" << 1000.0 / fps << "ms)\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::reset_frame_rate
//       Access: Public
//  Description: Resets the frame rate computation.
////////////////////////////////////////////////////////////////////
void PandaFramework::
reset_frame_rate() {
  _start_time = ClockObject::get_global_clock()->get_frame_time();
  _frame_count = ClockObject::get_global_clock()->get_frame_count();
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::set_wireframe
//       Access: Public
//  Description: Sets the wireframe state on all windows.
////////////////////////////////////////////////////////////////////
void PandaFramework::
set_wireframe(bool enable) {
  Windows::iterator wi;
  for (wi = _windows.begin(); wi != _windows.end(); ++wi) {
    WindowFramework *wf = (*wi);
    wf->set_wireframe(enable);
  }

  _wireframe_enabled = enable;
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::set_texture
//       Access: Public
//  Description: Sets the texture state on all windows.
////////////////////////////////////////////////////////////////////
void PandaFramework::
set_texture(bool enable) {
  Windows::iterator wi;
  for (wi = _windows.begin(); wi != _windows.end(); ++wi) {
    WindowFramework *wf = (*wi);
    wf->set_texture(enable);
  }

  _texture_enabled = enable;
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::set_two_sided
//       Access: Public
//  Description: Sets the two_sided state on all windows.
////////////////////////////////////////////////////////////////////
void PandaFramework::
set_two_sided(bool enable) {
  Windows::iterator wi;
  for (wi = _windows.begin(); wi != _windows.end(); ++wi) {
    WindowFramework *wf = (*wi);
    wf->set_two_sided(enable);
  }

  _two_sided_enabled = enable;
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::set_lighting
//       Access: Public
//  Description: Sets the lighting state on all windows.
////////////////////////////////////////////////////////////////////
void PandaFramework::
set_lighting(bool enable) {
  Windows::iterator wi;
  for (wi = _windows.begin(); wi != _windows.end(); ++wi) {
    WindowFramework *wf = (*wi);
    wf->set_lighting(enable);
  }

  _lighting_enabled = enable;
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::hide_collision_solids
//       Access: Public
//  Description: Hides any collision solids which are visible in the
//               indicated scene graph.  Returns the number of
//               collision solids hidden.
////////////////////////////////////////////////////////////////////
int PandaFramework::
hide_collision_solids(NodePath node) {
  int num_changed = 0;

  if (node.node()->is_of_type(CollisionNode::get_class_type())) {
    if (!node.is_hidden()) {
      node.hide();
      num_changed++;
    }
  }

  int num_children = node.get_num_children();
  for (int i = 0; i < num_children; i++) {
    num_changed += hide_collision_solids(node.get_child(i));
  }

  return num_changed;
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::show_collision_solids
//       Access: Public
//  Description: Shows any collision solids which are directly hidden
//               in the indicated scene graph.  Returns the number of
//               collision solids shown.
////////////////////////////////////////////////////////////////////
int PandaFramework::
show_collision_solids(NodePath node) {
  int num_changed = 0;

  if (node.node()->is_of_type(CollisionNode::get_class_type())) {
    if (node.get_hidden_ancestor() == node) {
      node.show();
      num_changed++;
    }
  }

  int num_children = node.get_num_children();
  for (int i = 0; i < num_children; i++) {
    num_changed += show_collision_solids(node.get_child(i));
  }

  return num_changed;
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::set_highlight
//       Access: Public
//  Description: Sets the indicated node (normally a node within the
//               get_models() tree) up as the highlighted node.
//               Certain operations affect the highlighted node only.
////////////////////////////////////////////////////////////////////
void PandaFramework::
set_highlight(const NodePath &node) {
  clear_highlight();
  _highlight = node;
  if (!_highlight.is_empty()) {
    framework_cat.info(false) << _highlight << "\n";
    _highlight.show_bounds();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::clear_highlight
//       Access: Public
//  Description: Unhighlights the currently highlighted node, if any.
////////////////////////////////////////////////////////////////////
void PandaFramework::
clear_highlight() {
  if (!_highlight.is_empty()) {
    _highlight.hide_bounds();
    _highlight = NodePath();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::enable_default_keys
//       Access: Public
//  Description: Sets callbacks on the event handler to handle all of
//               the normal viewer keys, like t to toggle texture, ESC
//               or q to quit, etc.
////////////////////////////////////////////////////////////////////
void PandaFramework::
enable_default_keys() {
  if (!_default_keys_enabled) {
    do_enable_default_keys();
    _default_keys_enabled = true;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::do_frame
//       Access: Public, Virtual
//  Description: Renders one frame and performs all associated
//               processing.  Returns true if we should continue
//               rendering, false if we should exit.  This is normally
//               called only from main_loop().
////////////////////////////////////////////////////////////////////
bool PandaFramework::
do_frame() {
  nassertr(_is_open, false);
  DataGraphTraverser dg_trav;
  dg_trav.traverse(_data_root.node());
  _event_handler.process_events();
  _engine.render_frame();

  return !_exit_flag;
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::main_loop
//       Access: Public
//  Description: Called to yield control to the panda framework.  This
//               function does not return until set_exit_flag() has
//               been called.
////////////////////////////////////////////////////////////////////
void PandaFramework::
main_loop() {
  while (do_frame()) {
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::make_window_framework
//       Access: Protected, Virtual
//  Description: Creates a new WindowFramework object.  This is
//               provided as a hook so derived PandaFramework classes
//               can create custom WindowFramework objects.
////////////////////////////////////////////////////////////////////
WindowFramework *PandaFramework::
make_window_framework() {
  return new WindowFramework(this);
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::make_default_pipe
//       Access: Protected, Virtual
//  Description: Creates the default GraphicsPipe that will contain
//               all windows that are not opened on a specific pipe.
////////////////////////////////////////////////////////////////////
void PandaFramework::
make_default_pipe() {
  // We use the GraphicsPipe factory to make us a renderable pipe
  // without knowing exactly what kind of pipes we have available.  We
  // don't care, so long as we can render to it interactively.

  // This depends on the shared library or libraries (DLL's to you
  // Windows folks) that have been loaded in at runtime from the
  // load-display Configrc variable.
  GraphicsPipe::resolve_modules();

  nout << "Known pipe types:" << endl;
  GraphicsPipe::get_factory().write_types(nout, 2);

  _default_pipe = GraphicsPipe::get_factory().
    make_instance(InteractiveGraphicsPipe::get_class_type());

  if (_default_pipe == (GraphicsPipe*)NULL) {
    nout << "No interactive pipe is available!  Check your Configrc!\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::do_enable_default_keys
//       Access: Protected, Virtual
//  Description: The implementation of enable_default_keys().
////////////////////////////////////////////////////////////////////
void PandaFramework::
do_enable_default_keys() {
  _event_handler.add_hook("escape", event_esc, this);
  _event_handler.add_hook("q", event_esc, this);
  _event_handler.add_hook("f", event_f, this);
  _event_handler.add_hook("w", event_w, this);
  _event_handler.add_hook("t", event_t, this);
  _event_handler.add_hook("b", event_b, this);
  _event_handler.add_hook("l", event_l, this);
  _event_handler.add_hook("c", event_c, this);
  _event_handler.add_hook("shift-c", event_C, this);
  _event_handler.add_hook("shift-b", event_B, this);
  _event_handler.add_hook("shift-l", event_L, this);
  _event_handler.add_hook("h", event_h, this);
  _event_handler.add_hook("arrow_up", event_arrow_up, this);
  _event_handler.add_hook("arrow_down", event_arrow_down, this);
  _event_handler.add_hook("arrow_left", event_arrow_left, this);
  _event_handler.add_hook("arrow_right", event_arrow_right, this);
  _event_handler.add_hook("shift-s", event_S, this);
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::event_esc
//       Access: Protected, Static
//  Description: Default handler for ESC or q key: exit the
//               application.
////////////////////////////////////////////////////////////////////
void PandaFramework::
event_esc(CPT_Event, void *data) {
  PandaFramework *self = (PandaFramework *)data;
  self->_exit_flag = true;
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::event_f
//       Access: Protected, Static
//  Description: Default handler for f key: report and reset frame
//               rate.
////////////////////////////////////////////////////////////////////
void PandaFramework::
event_f(CPT_Event, void *data) {
  PandaFramework *self = (PandaFramework *)data;
  self->report_frame_rate(nout);
  self->reset_frame_rate();
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::event_w
//       Access: Protected, Static
//  Description: Default handler for w key: toggle wireframe.
////////////////////////////////////////////////////////////////////
void PandaFramework::
event_w(CPT_Event, void *data) {
  PandaFramework *self = (PandaFramework *)data;
  self->set_wireframe(!self->get_wireframe());
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::event_t
//       Access: Protected, Static
//  Description: Default handler for t key: toggle texture.
////////////////////////////////////////////////////////////////////
void PandaFramework::
event_t(CPT_Event, void *data) {
  PandaFramework *self = (PandaFramework *)data;
  self->set_texture(!self->get_texture());
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::event_b
//       Access: Protected, Static
//  Description: Default handler for b key: toggle backface (two-sided
//               rendering).
////////////////////////////////////////////////////////////////////
void PandaFramework::
event_b(CPT_Event, void *data) {
  PandaFramework *self = (PandaFramework *)data;
  self->set_two_sided(!self->get_two_sided());
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::event_l
//       Access: Protected, Static
//  Description: Default handler for l key: toggle lighting.
////////////////////////////////////////////////////////////////////
void PandaFramework::
event_l(CPT_Event, void *data) {
  PandaFramework *self = (PandaFramework *)data;
  self->set_lighting(!self->get_lighting());
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::event_c
//       Access: Protected, Static
//  Description: Default handler for c key: center the trackball over
//               the scene, or over the highlighted part of the scene.
////////////////////////////////////////////////////////////////////
void PandaFramework::
event_c(CPT_Event, void *data) {
  PandaFramework *self = (PandaFramework *)data;

  NodePath node = self->get_highlight();
  if (node.is_empty()) {
    node = self->get_models();
  }

  Windows::iterator wi;
  for (wi = self->_windows.begin(); wi != self->_windows.end(); ++wi) {
    WindowFramework *wf = (*wi);
    wf->center_trackball(node);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::event_C
//       Access: Protected, Static
//  Description: Default handler for shift-C key: toggle the showing
//               of collision solids.
////////////////////////////////////////////////////////////////////
void PandaFramework::
event_C(CPT_Event, void *data) {
  PandaFramework *self = (PandaFramework *)data;

  NodePath node = self->get_highlight();
  if (node.is_empty()) {
    node = self->get_models();
  }

  if (self->hide_collision_solids(node) == 0) {
    self->show_collision_solids(node);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::event_B
//       Access: Protected, Static
//  Description: Default handler for shift-B key: describe the
//               bounding volume of the currently selected object, or
//               the entire scene.
////////////////////////////////////////////////////////////////////
void PandaFramework::
event_B(CPT_Event, void *data) {
  PandaFramework *self = (PandaFramework *)data;

  NodePath node = self->get_highlight();
  if (node.is_empty()) {
    node = self->get_models();
  }

  node.get_bounds()->write(nout);
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::event_L
//       Access: Protected, Static
//  Description: Default handler for shift-L key: list the contents of
//               the scene graph, or the highlighted node.
////////////////////////////////////////////////////////////////////
void PandaFramework::
event_L(CPT_Event, void *data) {
  PandaFramework *self = (PandaFramework *)data;

  NodePath node = self->get_highlight();
  if (node.is_empty()) {
    node = self->get_models();
  }

  node.ls();
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::event_h
//       Access: Protected, Static
//  Description: Default handler for h key: toggle highlight mode.  In
//               this mode, you can walk the scene graph with the
//               arrow keys to highlight different nodes.
////////////////////////////////////////////////////////////////////
void PandaFramework::
event_h(CPT_Event, void *data) {
  PandaFramework *self = (PandaFramework *)data;
  
  if (self->has_highlight()) {
    self->clear_highlight();
  } else {
    self->set_highlight(self->get_models());
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::event_arrow_up
//       Access: Protected, Static
//  Description: Default handler for up arrow key: in highlight mode,
//               move the highlight to the node's parent.
////////////////////////////////////////////////////////////////////
void PandaFramework::
event_arrow_up(CPT_Event, void *data) {
  PandaFramework *self = (PandaFramework *)data;

  if (self->has_highlight()) {
    NodePath node = self->get_highlight();
    if (node.has_parent() && node != self->get_models()) {
      self->set_highlight(node.get_parent());
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::event_arrow_down
//       Access: Protected, Static
//  Description: Default handler for up arrow key: in highlight mode,
//               move the highlight to the node's first child.
////////////////////////////////////////////////////////////////////
void PandaFramework::
event_arrow_down(CPT_Event, void *data) {
  PandaFramework *self = (PandaFramework *)data;

  if (self->has_highlight()) {
    NodePath node = self->get_highlight();
    if (node.get_num_children() > 0) {
      self->set_highlight(node.get_child(0));
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::event_arrow_left
//       Access: Protected, Static
//  Description: Default handler for up arrow key: in highlight mode,
//               move the highlight to the node's nearest sibling on
//               the left.
////////////////////////////////////////////////////////////////////
void PandaFramework::
event_arrow_left(CPT_Event, void *data) {
  PandaFramework *self = (PandaFramework *)data;

  if (self->has_highlight()) {
    NodePath node = self->get_highlight();
    NodePath parent = node.get_parent();
    if (node.has_parent() && node != self->get_models()) {
      int index = parent.node()->find_child(node.node());
      nassertv(index >= 0);
      int sibling = index - 1;
      if (sibling >= 0) {
        self->set_highlight(NodePath(parent, parent.node()->get_child(sibling)));
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::event_arrow_right
//       Access: Protected, Static
//  Description: Default handler for up arrow key: in highlight mode,
//               move the highlight to the node's nearest sibling on
//               the right.
////////////////////////////////////////////////////////////////////
void PandaFramework::
event_arrow_right(CPT_Event, void *data) {
  PandaFramework *self = (PandaFramework *)data;

  if (self->has_highlight()) {
    NodePath node = self->get_highlight();
    NodePath parent = node.get_parent();
    if (node.has_parent() && node != self->get_models()) {
      int index = parent.node()->find_child(node.node());
      nassertv(index >= 0);
      int num_children = parent.node()->get_num_children();
      int sibling = index + 1;
      if (sibling < num_children) {
        self->set_highlight(NodePath(parent, parent.node()->get_child(sibling)));
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PandaFramework::event_S
//       Access: Protected, Static
//  Description: Default handler for shift-S key: activate stats.
////////////////////////////////////////////////////////////////////
void PandaFramework::
event_S(CPT_Event, void *data) {
#ifdef DO_PSTATS
  nout << "Connecting to stats host" << endl;
  PStatClient::connect();
#else
  nout << "Stats host not supported." << endl;
#endif
}
