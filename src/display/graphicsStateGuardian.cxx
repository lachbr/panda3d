// Filename: graphicsStateGuardian.cxx
// Created by:  drose (02Feb99)
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


#include "graphicsStateGuardian.h"
#include "config_display.h"
#include "textureContext.h"
#include "renderBuffer.h"
#include "colorAttrib.h"
#include "colorScaleAttrib.h"
#include "lightAttrib.h"
#include "textureAttrib.h"
#include "renderState.h"
#include "depthWriteAttrib.h"
#include "colorWriteAttrib.h"
#include "textureAttrib.h"
#include "lightAttrib.h"
#include "clipPlaneAttrib.h"
#include "light.h"
#include "planeNode.h"
#include "ambientLight.h"

#include "clockObject.h"
#include "pStatTimer.h"

#include <algorithm>

#ifndef CPPPARSER
PStatCollector GraphicsStateGuardian::_total_texusage_pcollector("Texture usage");
PStatCollector GraphicsStateGuardian::_active_texusage_pcollector("Texture usage:Active");
PStatCollector GraphicsStateGuardian::_total_geom_pcollector("Prepared Geoms");
PStatCollector GraphicsStateGuardian::_active_geom_pcollector("Prepared Geoms:Active");
PStatCollector GraphicsStateGuardian::_total_geom_node_pcollector("Prepared GeomNodes");
PStatCollector GraphicsStateGuardian::_active_geom_node_pcollector("Prepared GeomNodes:Active");
PStatCollector GraphicsStateGuardian::_total_texmem_pcollector("Texture memory");
PStatCollector GraphicsStateGuardian::_used_texmem_pcollector("Texture memory:In use");
PStatCollector GraphicsStateGuardian::_texmgrmem_total_pcollector("Texture manager");
PStatCollector GraphicsStateGuardian::_texmgrmem_resident_pcollector("Texture manager:Resident");
PStatCollector GraphicsStateGuardian::_vertices_pcollector("Vertices");
PStatCollector GraphicsStateGuardian::_vertices_tristrip_pcollector("Vertices:Triangle strips");
PStatCollector GraphicsStateGuardian::_vertices_trifan_pcollector("Vertices:Triangle fans");
PStatCollector GraphicsStateGuardian::_vertices_tri_pcollector("Vertices:Triangles");
PStatCollector GraphicsStateGuardian::_vertices_other_pcollector("Vertices:Other");
PStatCollector GraphicsStateGuardian::_state_changes_pcollector("State changes");
PStatCollector GraphicsStateGuardian::_transform_state_pcollector("State changes:Transforms");
PStatCollector GraphicsStateGuardian::_texture_state_pcollector("State changes:Textures");
PStatCollector GraphicsStateGuardian::_nodes_pcollector("Nodes");
PStatCollector GraphicsStateGuardian::_geom_nodes_pcollector("Nodes:GeomNodes");
PStatCollector GraphicsStateGuardian::_frustum_cull_volumes_pcollector("Cull volumes");
PStatCollector GraphicsStateGuardian::_frustum_cull_transforms_pcollector("Cull volumes:Transforms");

PStatCollector GraphicsStateGuardian::_set_state_pcollector("Draw:Set state");
PStatCollector GraphicsStateGuardian::_draw_primitive_pcollector("Draw:Primitive");

#endif

TypeHandle GraphicsStateGuardian::_type_handle;
TypeHandle GraphicsStateGuardian::GsgWindow::_type_handle;

GraphicsStateGuardian::GsgFactory *GraphicsStateGuardian::_factory = NULL;

GraphicsStateGuardian::GsgWindow::~GsgWindow(void) {}

TypeHandle GraphicsStateGuardian::GsgWindow::get_class_type(void) {
  return _type_handle;
}

void GraphicsStateGuardian::GsgWindow::init_type(void) {
  GsgParam::init_type();
  register_type(_type_handle, "GraphicsStateGuardian::GsgWindow",
                GsgParam::get_class_type());
}

TypeHandle GraphicsStateGuardian::GsgWindow::get_type(void) const {
  return get_class_type();
}

TypeHandle GraphicsStateGuardian::GsgWindow::force_init_type(void) {
  init_type();
  return get_class_type();
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
GraphicsStateGuardian::
GraphicsStateGuardian(GraphicsWindow *win) {
  _win = win;
  _coordinate_system = default_coordinate_system;
  _current_display_region = (DisplayRegion*)0L;
  _current_lens = (Lens *)NULL;
  reset();
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::Destructor
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
GraphicsStateGuardian::
~GraphicsStateGuardian() {
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::reset
//       Access: Public, Virtual
//  Description: Resets all internal state as if the gsg were newly
//               created.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
reset() {
  _display_region_stack_level = 0;
  _frame_buffer_stack_level = 0;
  _lens_stack_level = 0;

  _state = RenderState::make_empty();
  _transform = TransformState::make_identity();

  _buffer_mask = 0;
  _color_clear_value.set(gsg_clear_r, gsg_clear_g, gsg_clear_b, 0.0);
  _depth_clear_value = 1.0;
  _stencil_clear_value = 0.0;
  _accum_clear_value.set(0.0, 0.0, 0.0, 0.0);
  _clear_buffer_type = RenderBuffer::T_back | RenderBuffer::T_depth;
  _normals_enabled = false;

  //Color and alpha transform variables
  _color_transform_enabled = 0;
  _current_color_offset.set(0.0f, 0.0f, 0.0f, 0.0f);
  _current_color_scale.set(1.0f, 1.0f, 1.0f, 1.0f);

  _color_write_mode = ColorWriteAttrib::M_on;
  _color_blend_mode = ColorBlendAttrib::M_none;
  _transparency_mode = TransparencyAttrib::M_none;

  _has_scene_graph_color = false;
  _scene_graph_color_stale = false;
  _vertex_colors_enabled = true;
  _lighting_enabled = false;
  _lighting_enabled_this_frame = false;

  _clip_planes_enabled = false;
  _clip_planes_enabled_this_frame = false;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::get_render_buffer
//       Access: Public
//  Description: Returns a RenderBuffer object suitable for operating
//               on the requested set of buffers.  buffer_type is the
//               union of all the desired RenderBuffer::Type values.
////////////////////////////////////////////////////////////////////
RenderBuffer GraphicsStateGuardian::
get_render_buffer(int buffer_type) {
  return RenderBuffer(this, buffer_type & _buffer_mask);
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::release_all_textures
//       Access: Public
//  Description: Frees the resources for all textures associated with
//               this GSG.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
release_all_textures() {
  // We must get a copy of the _prepared_textures list first, because
  // each call to release_texture() will remove that texture from the
  // list, and we don't want to traverse a list while we're modifying
  // it!

  Textures temp = _prepared_textures;
  for (Textures::const_iterator ti = temp.begin();
       ti != temp.end();
       ++ti) {
    release_texture(*ti);
  }

  // Now that we've released all of the textures, the
  // _prepared_textures list should have completely emptied itself.
  nassertv(_prepared_textures.empty());
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::release_all_geoms
//       Access: Public
//  Description: Frees the resources for all Geoms and GeomNodes
//               associated with this GSG.  Warning!  This may make
//               the Geoms unrenderable, if the Panda-level
//               information has been deleted.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
release_all_geoms() {
  // As above, for both Geoms and GeomNodes.

  Geoms temp_geoms = _prepared_geoms;
  for (Geoms::const_iterator gi = temp_geoms.begin();
       gi != temp_geoms.end();
       ++gi) {
    release_geom(*gi);
  }

  GeomNodes temp_geom_nodes = _prepared_geom_nodes;
  for (GeomNodes::const_iterator gni = temp_geom_nodes.begin();
       gni != temp_geom_nodes.end();
       ++gni) {
    release_geom_node(*gni);
  }

  nassertv(_prepared_geoms.empty());
  nassertv(_prepared_geom_nodes.empty());
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::prepare_texture
//       Access: Public, Virtual
//  Description: Prepares the indicated texture for retained-mode
//               rendering.  In the future, this texture may be
//               applied simply by calling apply_texture() with the
//               value returned by this function.
////////////////////////////////////////////////////////////////////
TextureContext *GraphicsStateGuardian::
prepare_texture(Texture *) {
  return (TextureContext *)NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::apply_texture
//       Access: Public, Virtual
//  Description: Applies the texture previously indicated via a call
//               to prepare_texture() to the graphics state, so that
//               geometry rendered in the future will be rendered with
//               the given texture.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
apply_texture(TextureContext *) {
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::release_texture
//       Access: Public, Virtual
//  Description: Frees the resources previously allocated via a call
//               to prepare_texture(), including deleting the
//               TextureContext itself, if necessary.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
release_texture(TextureContext *) {
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::prepare_geom_node
//       Access: Public, Virtual
//  Description: Prepares the indicated GeomNode for retained-mode
//               rendering.  If this function returns non-NULL, the
//               value returned will be passed back to a future call
//               to draw_geom_node(), which is expected to draw the
//               contents of the node.
////////////////////////////////////////////////////////////////////
GeomNodeContext *GraphicsStateGuardian::
prepare_geom_node(GeomNode *) {
  return (GeomNodeContext *)NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::draw_geom_node
//       Access: Public, Virtual
//  Description: Draws a GeomNode previously indicated by a call to
//               prepare_geom_node().
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
draw_geom_node(GeomNode *node, const RenderState *state,
               GeomNodeContext *) {
#if 0   // temporarily disabled until ported to new scene graph
  int num_geoms = node->get_num_geoms();
  for (int i = 0; i < num_geoms; i++) {
    node->get_geom(i)->draw(this);
  }
#endif   // temporarily disabled until ported to new scene graph
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::release_geom_node
//       Access: Public, Virtual
//  Description: Frees the resources previously allocated via a call
//               to prepare_geom_node(), including deleting the
//               GeomNodeContext itself, if necessary.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
release_geom_node(GeomNodeContext *) {
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::prepare_geom
//       Access: Public, Virtual
//  Description: Prepares the indicated Geom for retained-mode
//               rendering.  The value returned by this function will
//               be passed back into future calls to draw_tristrip(),
//               etc., along with the Geom pointer.
////////////////////////////////////////////////////////////////////
GeomContext *GraphicsStateGuardian::
prepare_geom(Geom *) {
  return (GeomContext *)NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::release_geom
//       Access: Public, Virtual
//  Description: Frees the resources previously allocated via a call
//               to prepare_geom(), including deleting the GeomContext
//               itself, if necessary.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
release_geom(GeomContext *) {
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::set_state_and_transform
//       Access: Public, Virtual
//  Description: Simultaneously resets the render state and the
//               transform state.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
set_state_and_transform(const RenderState *state,
                        const TransformState *transform) {
  set_state(state);
  set_transform(transform);
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::set_color_clear_value
//       Access: Public
//  Description: Sets the color that the next do_clear() command will set
//               the color buffer to
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
set_color_clear_value(const Colorf& value) {
  _color_clear_value = value;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::set_depth_clear_value
//       Access: Public
//  Description: Sets the depth that the next do_clear() command will set
//               the depth buffer to
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
set_depth_clear_value(const float value) {
  _depth_clear_value = value;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::clear
//       Access: Public
//  Description: Clears the framebuffer within the current
//               DisplayRegion, according to the flags indicated by
//               the given ClearableRegion object.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
clear(ClearableRegion *clearable) {
  int clear_buffer_type = 0;
  if (clearable->get_clear_color_active()) {
    clear_buffer_type |= RenderBuffer::T_back;
    set_color_clear_value(clearable->get_clear_color());
  }
  if (clearable->get_clear_depth_active()) {
    clear_buffer_type |= RenderBuffer::T_depth;
    set_depth_clear_value(clearable->get_clear_depth());
  }

  if (clear_buffer_type != 0) {
    prepare_display_region();
    do_clear(get_render_buffer(clear_buffer_type));
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::prepare_lens
//       Access: Public, Virtual
//  Description: Makes the current lens (whichever lens was most
//               recently specified with push_lens()) active, so that
//               it will transform future rendered geometry.  Normally
//               this is only called from the draw process, and
//               usually it is called immediately after a call to
//               push_lens().
//
//               The return value is true if the lens is acceptable,
//               false if it is not.
////////////////////////////////////////////////////////////////////
bool GraphicsStateGuardian::
prepare_lens() {
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::begin_frame
//       Access: Public, Virtual
//  Description: Called before each frame is rendered, to allow the
//               GSG a chance to do any internal cleanup before
//               beginning the frame.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
begin_frame() {
  // Undo any lighting we had enabled last frame, to force the lights
  // to be reissued, in case their parameters or positions have
  // changed between frames.
  if (_lighting_enabled_this_frame) {
    for (int i = 0; i < (int)_light_info.size(); i++) {
      if (_light_info[i]._enabled) {
        enable_light(i, false);
        _light_info[i]._enabled = false;
      }
      _light_info[i]._light = (Light *)NULL;
    }

    // Also force the lighting state to unlit, so that issue_light()
    // will be guaranteed to be called next frame even if we have the
    // same set of light pointers we had this frame.
    modify_state(get_unlit_state());

    _lighting_enabled_this_frame = false;
  }

  // Ditto for the clipping planes.
  if (_clip_planes_enabled_this_frame) {
    for (int i = 0; i < (int)_clip_plane_info.size(); i++) {
      if (_clip_plane_info[i]._enabled) {
        enable_clip_plane(i, false);
        _clip_plane_info[i]._enabled = false;
      }
      _clip_plane_info[i]._plane = (PlaneNode *)NULL;
    }

    modify_state(get_unclipped_state());

    _clip_planes_enabled_this_frame = false;
  }

#ifdef DO_PSTATS
  // For Pstats to track our current texture memory usage, we have to
  // reset the set of current textures each frame.
  init_frame_pstats();

  // But since we don't get sent a new issue_texture() unless our
  // texture state has changed, we have to be sure to clear the
  // current texture state now.  A bit unfortunate, but probably not
  // measurably expensive.
  modify_state(get_untextured_state());
#endif
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::end_frame
//       Access: Public, Virtual
//  Description: Called after each frame is rendered, to allow the
//               GSG a chance to do any internal cleanup after
//               rendering the frame, and before the window flips.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
end_frame() {
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::wants_normals
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
bool GraphicsStateGuardian::
wants_normals() const {
  return _normals_enabled;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::wants_texcoords
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
bool GraphicsStateGuardian::
wants_texcoords() const {
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::wants_colors
//       Access: Public, Virtual
//  Description: Returns true if the GSG should issue geometry color
//               commands, false otherwise.
////////////////////////////////////////////////////////////////////
bool GraphicsStateGuardian::
wants_colors() const {
  return _vertex_colors_enabled;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::depth_offset_decals
//       Access: Public, Virtual
//  Description: Returns true if this GSG can implement decals using a
//               DepthOffsetAttrib, or false if that is unreliable
//               and the three-step rendering process should be used
//               instead.
////////////////////////////////////////////////////////////////////
bool GraphicsStateGuardian::
depth_offset_decals() {
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::begin_decal_base_first
//       Access: Public, Virtual
//  Description: Called during draw to begin a three-step rendering
//               phase to draw decals.  The first step,
//               begin_decal_base_first(), is called prior to drawing the
//               base geometry.  It should set up whatever internal
//               state is appropriate, as well as returning a
//               RenderState object that should be applied to the base
//               geometry for rendering.
////////////////////////////////////////////////////////////////////
CPT(RenderState) GraphicsStateGuardian::
begin_decal_base_first() {
  // Turn off writing the depth buffer to render the base geometry.
  static CPT(RenderState) decal_base_first;
  if (decal_base_first == (const RenderState *)NULL) {
    decal_base_first = RenderState::make
      (DepthWriteAttrib::make(DepthWriteAttrib::M_off),
       RenderState::get_max_priority());
  }
  return decal_base_first;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::begin_decal_nested
//       Access: Public, Virtual
//  Description: Called during draw to begin a three-step rendering
//               phase to draw decals.  The second step,
//               begin_decal_nested(), is called after drawing the
//               base geometry and prior to drawing any of the nested
//               decal geometry that is to be applied to the base
//               geometry.
////////////////////////////////////////////////////////////////////
CPT(RenderState) GraphicsStateGuardian::
begin_decal_nested() {
  // We should keep the depth buffer off during this operation, so
  // that decals on decals will render properly.
  static CPT(RenderState) decal_nested;
  if (decal_nested == (const RenderState *)NULL) {
    decal_nested = RenderState::make
      (DepthWriteAttrib::make(DepthWriteAttrib::M_off),
       RenderState::get_max_priority());
  }
  return decal_nested;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::begin_decal_base_second
//       Access: Public, Virtual
//  Description: Called during draw to begin a three-step rendering
//               phase to draw decals.  The third step,
//               begin_decal_base_second(), is called after drawing the
//               base geometry and the nested decal geometry, and
//               prior to drawing the base geometry one more time (if
//               needed).
//
//               It should return a RenderState object appropriate for
//               rendering the base geometry the second time, or NULL
//               if it is not necessary to re-render the base
//               geometry.
////////////////////////////////////////////////////////////////////
CPT(RenderState) GraphicsStateGuardian::
begin_decal_base_second() {
  // Now let the depth buffer go back on, but turn off writing the
  // color buffer to render the base geometry after the second pass.
  // Also, turn off texturing since there's no need for it now.
  static CPT(RenderState) decal_base_second;
  if (decal_base_second == (const RenderState *)NULL) {
    decal_base_second = RenderState::make
      (ColorWriteAttrib::make(ColorWriteAttrib::M_off),
       // On reflection, we need to leave texturing on so the alpha
       // test mechanism can work (if it is enabled, e.g. we are
       // rendering an object with M_dual transparency).
       //       TextureAttrib::make_off(),
       RenderState::get_max_priority());
  }
  return decal_base_second;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::finish_decal
//       Access: Public, Virtual
//  Description: Called during draw to clean up after decals are
//               finished.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
finish_decal() {
  // No need to do anything special here.
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::get_internal_coordinate_system
//       Access: Public, Virtual
//  Description: Should be overridden by derived classes to return the
//               coordinate system used internally by the GSG, if any
//               one particular coordinate system is used.  The
//               default, CS_default, indicates that the GSG can use
//               any coordinate system.
//
//               If this returns other than CS_default, the
//               GraphicsEngine will automatically convert all
//               transforms into the indicated coordinate system.
////////////////////////////////////////////////////////////////////
CoordinateSystem GraphicsStateGuardian::
get_internal_coordinate_system() const {
  return CS_default;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::issue_transform
//       Access: Public, Virtual
//  Description: Sends the indicated transform matrix to the graphics
//               API to be applied to future vertices.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
issue_transform(const TransformState *) {
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::issue_color_scale
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
issue_color_scale(const ColorScaleAttrib *attrib) {
  _current_color_scale = attrib->get_scale();

  if (_current_color_scale == LVecBase4f(1.0f, 1.0f, 1.0f, 1.0f)) {
    _color_transform_enabled &= ~CT_scale;
  } else {
    _color_transform_enabled |= CT_scale;
  }

  _scene_graph_color_stale = _has_scene_graph_color;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::issue_color
//       Access: Public, Virtual
//  Description: This method is defined in the base class because it
//               is likely that this functionality will be used for
//               all (or at least most) kinds of
//               GraphicsStateGuardians--it's not specific to any one
//               rendering backend.
//
//               The ColorAttribute just changes the interpretation of
//               the color on the vertices, and fiddles with
//               _vertex_colors_enabled, etc.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
issue_color(const ColorAttrib *attrib) {
  switch (attrib->get_color_type()) {
  case ColorAttrib::T_flat:
    // Color attribute flat: it specifies a scene graph color that
    // overrides the vertex color.
    _scene_graph_color = attrib->get_color();
    _has_scene_graph_color = true;
    _vertex_colors_enabled = false;
    _scene_graph_color_stale = true;
    break;

  case ColorAttrib::T_off:
    // Color attribute off: it specifies that no scene graph color is
    // in effect, and vertex color is not important either.
    _has_scene_graph_color = false;
    _scene_graph_color_stale = false;
    _vertex_colors_enabled = false;
    break;

  case ColorAttrib::T_vertex:
    // Color attribute vertex: it specifies that vertex color should
    // be revealed.
    _has_scene_graph_color = false;
    _scene_graph_color_stale = false;
    _vertex_colors_enabled = true;
    break;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::issue_light
//       Access: Public, Virtual
//  Description: The default implementation of issue_light() assumes
//               we have a limited number of hardware lights
//               available.  This function assigns each light to a
//               different hardware light id, trying to keep each
//               light associated with the same id where possible, but
//               reusing id's when necessary.  When it is no longer
//               possible to reuse existing id's (e.g. all id's are in
//               use), slot_new_light() is called to prepare the next
//               sequential light id.
//
//               It will call apply_light() each time a light is
//               assigned to a particular id for the first time in a
//               given frame, and it will subsequently call
//               enable_light() to enable or disable each light as the
//               frame is rendered, as well as enable_lighting() to
//               enable or disable overall lighting.
//
//               If this model of hardware lights with id's does not
//               apply to a particular graphics engine, it should
//               override this function to do something more
//               appropriate instead.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
issue_light(const LightAttrib *attrib) {
  // Initialize the current ambient light total and newly enabled
  // light list
  Colorf cur_ambient_light(0.0f, 0.0f, 0.0f, 1.0f);
  int i;
  int max_lights = (int)_light_info.size();
  for (i = 0; i < max_lights; i++) {
    _light_info[i]._next_enabled = false;
  }

  bool any_bound = false;

  int num_enabled = 0;
  int num_lights = attrib->get_num_lights();
  if (attrib->get_operation() == LightAttrib::O_remove) {
    num_lights = 0;
  }
  for (int li = 0; li < num_lights; li++) {
    Light *light = attrib->get_light(li);
    nassertv(light != (Light *)NULL);

    num_enabled++;

    // Lighting should be enabled before we apply any lights.
    enable_lighting(true);
    _lighting_enabled = true;
    _lighting_enabled_this_frame = true;

    if (light->get_type() == AmbientLight::get_class_type()) {
      // Ambient lights don't require specific light ids; simply add
      // in the ambient contribution to the current total
      cur_ambient_light += light->get_color();
        
    } else {
      // Check to see if this light has already been bound to an id
      int cur_light_id = -1;
      for (i = 0; i < max_lights; i++) {
        if (_light_info[i]._light == light) {
          // Light has already been bound to an id, we only need to
          // enable the light, not reapply it.
          cur_light_id = -2;
          enable_light(i, true);
          _light_info[i]._enabled = true;
          _light_info[i]._next_enabled = true;
          break;
        }
      }
        
      // See if there are any unbound light ids
      if (cur_light_id == -1) {
        for (i = 0; i < max_lights; i++) {
          if (_light_info[i]._light == (Light *)NULL) {
            _light_info[i]._light = light;
            cur_light_id = i;
            break;
          }
        }
      }
        
      // If there were no unbound light ids, see if we can replace
      // a currently unused but previously bound id
      if (cur_light_id == -1) {
        for (i = 0; i < max_lights; i++) {
          if (!attrib->has_light(_light_info[i]._light)) {
            _light_info[i]._light = light;
            cur_light_id = i;
            break;
          }
        }
      }

      // If we *still* don't have a light id, slot a new one.
      if (cur_light_id == -1) {
        if (slot_new_light(max_lights)) {
          cur_light_id = max_lights;
          _light_info.push_back(LightInfo());
          max_lights++;
          nassertv(max_lights == (int)_light_info.size());
        }
      }
        
      if (cur_light_id >= 0) {
        enable_light(cur_light_id, true);
        _light_info[cur_light_id]._enabled = true;
        _light_info[cur_light_id]._next_enabled = true;
        
        if (!any_bound) {
          begin_bind_lights();
          any_bound = true;
        }

        // This is the first time this frame that this light has been
        // bound to this particular id.
        light->bind(this, cur_light_id);

      } else if (cur_light_id == -1) {
        gsg_cat.warning()
          << "Failed to bind " << *light << " to id.\n";
      }
    }
  }

  // Disable all unused lights
  for (i = 0; i < max_lights; i++) {
    if (!_light_info[i]._next_enabled) {
      enable_light(i, false);
      _light_info[i]._enabled = false;
    }
  }

  // If no lights were enabled, disable lighting
  if (num_enabled == 0) {
    enable_lighting(false);
    _lighting_enabled = false;
  } else {
    set_ambient_light(cur_ambient_light);
  }

  if (any_bound) {
    end_bind_lights();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::issue_color_write
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
issue_color_write(const ColorWriteAttrib *attrib) {
  _color_write_mode = attrib->get_mode();
  set_blend_mode(_color_write_mode, _color_blend_mode, _transparency_mode);
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::issue_transparency
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
issue_transparency(const TransparencyAttrib *attrib) {
  _transparency_mode = attrib->get_mode();
  set_blend_mode(_color_write_mode, _color_blend_mode, _transparency_mode);
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::issue_color_blend
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
issue_color_blend(const ColorBlendAttrib *attrib) {
  _color_blend_mode = attrib->get_mode();
  set_blend_mode(_color_write_mode, _color_blend_mode, _transparency_mode);
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::issue_clip_plane
//       Access: Public, Virtual
//  Description: This is fundametically similar to issue_light(), with
//               calls to slot_new_clip_plane(), apply_clip_plane(),
//               and enable_clip_planes(), as appropriate.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
issue_clip_plane(const ClipPlaneAttrib *attrib) {
  int i;
  int max_planes = (int)_clip_plane_info.size();
  for (i = 0; i < max_planes; i++) {
    _clip_plane_info[i]._next_enabled = false;
  }

  bool any_bound = false;

  int num_enabled = 0;
  int num_planes = attrib->get_num_planes();
  if (attrib->get_operation() == ClipPlaneAttrib::O_remove) {
    num_planes = 0;
  }
  for (int li = 0; li < num_planes; li++) {
    PlaneNode *plane = attrib->get_plane(li);
    nassertv(plane != (PlaneNode *)NULL);

    num_enabled++;

    // Planeing should be enabled before we apply any planes.
    enable_clip_planes(true);
    _clip_planes_enabled = true;
    _clip_planes_enabled_this_frame = true;

    // Check to see if this plane has already been bound to an id
    int cur_plane_id = -1;
    for (i = 0; i < max_planes; i++) {
      if (_clip_plane_info[i]._plane == plane) {
        // Plane has already been bound to an id, we only need to
        // enable the plane, not reapply it.
        cur_plane_id = -2;
        enable_clip_plane(i, true);
        _clip_plane_info[i]._enabled = true;
        _clip_plane_info[i]._next_enabled = true;
        break;
      }
    }
        
    // See if there are any unbound plane ids
    if (cur_plane_id == -1) {
      for (i = 0; i < max_planes; i++) {
        if (_clip_plane_info[i]._plane == (PlaneNode *)NULL) {
          _clip_plane_info[i]._plane = plane;
          cur_plane_id = i;
          break;
        }
      }
    }
        
    // If there were no unbound plane ids, see if we can replace
    // a currently unused but previously bound id
    if (cur_plane_id == -1) {
      for (i = 0; i < max_planes; i++) {
        if (!attrib->has_plane(_clip_plane_info[i]._plane)) {
          _clip_plane_info[i]._plane = plane;
          cur_plane_id = i;
          break;
        }
      }
    }

    // If we *still* don't have a plane id, slot a new one.
    if (cur_plane_id == -1) {
      if (slot_new_clip_plane(max_planes)) {
        cur_plane_id = max_planes;
        _clip_plane_info.push_back(ClipPlaneInfo());
        max_planes++;
        nassertv(max_planes == (int)_clip_plane_info.size());
      }
    }
        
    if (cur_plane_id >= 0) {
      enable_clip_plane(cur_plane_id, true);
      _clip_plane_info[cur_plane_id]._enabled = true;
      _clip_plane_info[cur_plane_id]._next_enabled = true;
      
      if (!any_bound) {
        begin_bind_clip_planes();
        any_bound = true;
      }
      
      // This is the first time this frame that this plane has been
      // bound to this particular id.
      bind_clip_plane(plane, cur_plane_id);
      
    } else if (cur_plane_id == -1) {
      gsg_cat.warning()
        << "Failed to bind " << *plane << " to id.\n";
    }
  }

  // Disable all unused planes
  for (i = 0; i < max_planes; i++) {
    if (!_clip_plane_info[i]._next_enabled) {
      enable_clip_plane(i, false);
      _clip_plane_info[i]._enabled = false;
    }
  }

  // If no planes were enabled, disable clip planes in general.
  if (num_enabled == 0) {
    enable_clip_planes(false);
    _clip_planes_enabled = false;
  }

  if (any_bound) {
    end_bind_clip_planes();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::bind_light
//       Access: Public, Virtual
//  Description: Called the first time a particular light has been
//               bound to a given id within a frame, this should set
//               up the associated hardware light with the light's
//               properties.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
bind_light(PointLight *light, int light_id) {
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::bind_light
//       Access: Public, Virtual
//  Description: Called the first time a particular light has been
//               bound to a given id within a frame, this should set
//               up the associated hardware light with the light's
//               properties.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
bind_light(DirectionalLight *light, int light_id) {
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::bind_light
//       Access: Public, Virtual
//  Description: Called the first time a particular light has been
//               bound to a given id within a frame, this should set
//               up the associated hardware light with the light's
//               properties.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
bind_light(Spotlight *light, int light_id) {
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::slot_new_light
//       Access: Protected, Virtual
//  Description: This will be called by the base class before a
//               particular light id will be used for the first time.
//               It is intended to allow the derived class to reserve
//               any additional resources, if required, for the new
//               light; and also to indicate whether the hardware
//               supports this many simultaneous lights.
//
//               The return value should be true if the additional
//               light is supported, or false if it is not.
////////////////////////////////////////////////////////////////////
bool GraphicsStateGuardian::
slot_new_light(int light_id) {
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::enable_lighting
//       Access: Protected, Virtual
//  Description: Intended to be overridden by a derived class to
//               enable or disable the use of lighting overall.  This
//               is called by issue_light() according to whether any
//               lights are in use or not.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
enable_lighting(bool enable) {
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::set_ambient_light
//       Access: Protected, Virtual
//  Description: Intended to be overridden by a derived class to
//               indicate the color of the ambient light that should
//               be in effect.  This is called by issue_light() after
//               all other lights have been enabled or disabled.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
set_ambient_light(const Colorf &color) {
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::enable_light
//       Access: Protected, Virtual
//  Description: Intended to be overridden by a derived class to
//               enable the indicated light id.  A specific Light will
//               already have been bound to this id via bind_light().
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
enable_light(int light_id, bool enable) {
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::begin_bind_lights
//       Access: Protected, Virtual
//  Description: Called immediately before bind_light() is called,
//               this is intended to provide the derived class a hook
//               in which to set up some state (like transform) that
//               might apply to several lights.
//
//               The sequence is: begin_bind_lights() will be called,
//               then one or more bind_light() calls, then
//               end_bind_lights().
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
begin_bind_lights() {
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::end_bind_lights
//       Access: Protected, Virtual
//  Description: Called after before bind_light() has been called one
//               or more times (but before any geometry is issued or
//               additional state is changed), this is intended to
//               clean up any temporary changes to the state that may
//               have been made by begin_bind_lights().
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
end_bind_lights() {
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::slot_new_clip_plane
//       Access: Protected, Virtual
//  Description: This will be called by the base class before a
//               particular clip plane id will be used for the first
//               time.  It is intended to allow the derived class to
//               reserve any additional resources, if required, for
//               the new clip plane; and also to indicate whether the
//               hardware supports this many simultaneous clipping
//               planes.
//
//               The return value should be true if the additional
//               plane is supported, or false if it is not.
////////////////////////////////////////////////////////////////////
bool GraphicsStateGuardian::
slot_new_clip_plane(int plane_id) {
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::enable_clip_planes
//       Access: Protected, Virtual
//  Description: Intended to be overridden by a derived class to
//               enable or disable the use of clipping planes overall.
//               This is called by issue_clip_plane() according to
//               whether any planes are in use or not.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
enable_clip_planes(bool enable) {
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::enable_clip_plane
//       Access: Protected, Virtual
//  Description: Intended to be overridden by a derived class to
//               enable the indicated plane id.  A specific PlaneNode
//               will already have been bound to this id via
//               bind_clip_plane().
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
enable_clip_plane(int plane_id, bool enable) {
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::begin_bind_clip_planes
//       Access: Protected, Virtual
//  Description: Called immediately before bind_clip_plane() is called,
//               this is intended to provide the derived class a hook
//               in which to set up some state (like transform) that
//               might apply to several planes.
//
//               The sequence is: begin_bind_clip_planes() will be
//               called, then one or more bind_clip_plane() calls,
//               then end_bind_clip_planes().
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
begin_bind_clip_planes() {
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::bind_clip_plane
//       Access: Public, Virtual
//  Description: Called the first time a particular clipping plane has been
//               bound to a given id within a frame, this should set
//               up the associated hardware (or API) clipping plane
//               with the plane's properties.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
bind_clip_plane(PlaneNode *plane, int plane_id) {
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::end_bind_clip_planes
//       Access: Protected, Virtual
//  Description: Called after before bind_clip_plane() has been called one
//               or more times (but before any geometry is issued or
//               additional state is changed), this is intended to
//               clean up any temporary changes to the state that may
//               have been made by begin_bind_clip_planes().
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
end_bind_clip_planes() {
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::set_blend_mode
//       Access: Protected, Virtual
//  Description: Called after any of these three blending states have
//               changed; this function is responsible for setting the
//               appropriate color blending mode based on the given
//               properties.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
set_blend_mode(ColorWriteAttrib::Mode, ColorBlendAttrib::Mode,
               TransparencyAttrib::Mode) {
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::mark_prepared_texture
//       Access: Protected
//  Description: This is intended to be called from within
//               prepare_texture().  It adds the indicated
//               TextureContext pointer to the _prepared_textures set,
//               and returns true if it was successfully added
//               (i.e. it was not already in the set).
////////////////////////////////////////////////////////////////////
bool GraphicsStateGuardian::
mark_prepared_texture(TextureContext *tc) {
  bool prepared = _prepared_textures.insert(tc).second;
#ifdef DO_PSTATS
  if (prepared) {
    _total_texusage_pcollector.add_level(tc->estimate_texture_memory());
  }
#endif
  return prepared;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::unmark_prepared_texture
//       Access: Protected
//  Description: This is intended to be called from within
//               release_texture().  It removes the indicated
//               TextureContext pointer from the _prepared_textures
//               set, and returns true if it was successfully removed
//               (i.e. it had been in the set).
////////////////////////////////////////////////////////////////////
bool GraphicsStateGuardian::
unmark_prepared_texture(TextureContext *tc) {
  bool removed = (_prepared_textures.erase(tc) != 0);
#ifdef DO_PSTATS
  if (removed) {
    _total_texusage_pcollector.sub_level(tc->estimate_texture_memory());
  }
#endif
  return removed;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::mark_prepared_geom
//       Access: Protected
//  Description: This is intended to be called from within
//               prepare_geom().  It adds the indicated GeomContext
//               pointer to the _prepared_geoms set, and returns true
//               if it was successfully added (i.e. it was not already
//               in the set).
////////////////////////////////////////////////////////////////////
bool GraphicsStateGuardian::
mark_prepared_geom(GeomContext *gc) {
  bool prepared = _prepared_geoms.insert(gc).second;
#ifdef DO_PSTATS
  if (prepared) {
    _total_geom_pcollector.add_level(1);
  }
#endif
  return prepared;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::unmark_prepared_geom
//       Access: Protected
//  Description: This is intended to be called from within
//               release_geom().  It removes the indicated GeomContext
//               pointer from the _prepared_geoms set, and returns
//               true if it was successfully removed (i.e. it had been
//               in the set).
////////////////////////////////////////////////////////////////////
bool GraphicsStateGuardian::
unmark_prepared_geom(GeomContext *gc) {
  bool removed = (_prepared_geoms.erase(gc) != 0);
#ifdef DO_PSTATS
  if (removed) {
    _total_geom_pcollector.sub_level(1);
  }
#endif
  return removed;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::mark_prepared_geom_node
//       Access: Protected
//  Description: This is intended to be called from within
//               prepare_geom_node().  It adds the indicated
//               GeomNodeContext pointer to the _prepared_geom_nodes
//               set, and returns true if it was successfully added
//               (i.e. it was not already in the set).
////////////////////////////////////////////////////////////////////
bool GraphicsStateGuardian::
mark_prepared_geom_node(GeomNodeContext *gnc) {
  bool prepared = _prepared_geom_nodes.insert(gnc).second;
#ifdef DO_PSTATS
  if (prepared) {
    _total_geom_node_pcollector.add_level(1);
  }
#endif
  return prepared;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::unmark_prepared_geom_node
//       Access: Protected
//  Description: This is intended to be called from within
//               release_geom_node().  It removes the indicated
//               GeomNodeContext pointer from the _prepared_geom_nodes
//               set, and returns true if it was successfully removed
//               (i.e. it had been in the set).
////////////////////////////////////////////////////////////////////
bool GraphicsStateGuardian::
unmark_prepared_geom_node(GeomNodeContext *gnc) {
  bool removed = (_prepared_geom_nodes.erase(gnc) != 0);
#ifdef DO_PSTATS
  if (removed) {
    _total_geom_node_pcollector.sub_level(1);
  }
#endif
  return removed;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::close_gsg
//       Access: Protected, Virtual
//  Description: This is called by the associated GraphicsWindow when
//               close_window() is called.  It should null out the
//               _win pointer and possibly free any open resources
//               associated with the GSG.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
close_gsg() {
  _win = (GraphicsWindow *)NULL;
}

#ifdef DO_PSTATS

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::init_frame_pstats
//       Access: Protected
//  Description: Initializes the relevant PStats data at the beginning
//               of the frame.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
init_frame_pstats() {
  _current_textures.clear();
  _current_geoms.clear();
  _current_geom_nodes.clear();
  _active_texusage_pcollector.clear_level();
  _active_geom_pcollector.clear_level();
  _active_geom_node_pcollector.clear_level();

  // Also clear out our other counters while we're here.
  _vertices_tristrip_pcollector.clear_level();
  _vertices_trifan_pcollector.clear_level();
  _vertices_tri_pcollector.clear_level();
  _vertices_other_pcollector.clear_level();

  _state_changes_pcollector.clear_level();
  _transform_state_pcollector.clear_level();
  _texture_state_pcollector.clear_level();

  _nodes_pcollector.clear_level();
  _geom_nodes_pcollector.clear_level();

  // Not to mention the view-frustum-cull counters.
  _frustum_cull_volumes_pcollector.clear_level();
  _frustum_cull_transforms_pcollector.clear_level();
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::add_to_texture_record
//       Access: Protected
//  Description: Records that the indicated texture has been applied
//               this frame, and thus must be present in current
//               texture memory.  This function is only used to update
//               the PStats current_texmem collector; it gets compiled
//               out if we aren't using PStats.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
add_to_texture_record(TextureContext *tc) {
  if (PStatClient::is_connected()) {
    if (_current_textures.insert(tc).second) {
      _active_texusage_pcollector.add_level(tc->estimate_texture_memory());
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::add_to_geom_record
//       Access: Protected
//  Description: Records that the indicated Geom has been drawn this
//               frame.  This function is only used to update the
//               PStats current_texmem collector; it gets compiled out
//               if we aren't using PStats.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
add_to_geom_record(GeomContext *gc) {
  if (PStatClient::is_connected()) {
    if (gc != (GeomContext *)NULL && _current_geoms.insert(gc).second) {
      _active_geom_pcollector.add_level(1);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::add_to_geom_node_record
//       Access: Protected
//  Description: Records that the indicated GeomNode has been drawn
//               this frame.  This function is only used to update the
//               PStats current_texmem collector; it gets compiled out
//               if we aren't using PStats.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
add_to_geom_node_record(GeomNodeContext *gnc) {
  if (PStatClient::is_connected()) {
    if (gnc != (GeomNodeContext *)NULL && 
        _current_geom_nodes.insert(gnc).second) {
      _active_geom_node_pcollector.add_level(1);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::record_state_change
//       Access: Protected
//  Description: Indicates a state change request for a property of
//               the given type.
////////////////////////////////////////////////////////////////////
void GraphicsStateGuardian::
record_state_change(TypeHandle type) {
  _state_changes_pcollector.add_level(1);

  // We can't use the get_class_type() methods since we don't have
  // those header files available yet.
  string name = type.get_name();
  if (name == "TransformTransition") {
    _transform_state_pcollector.add_level(1);
  } else if (name == "TextureTransition") {
    _texture_state_pcollector.add_level(1);
  }
}
#endif  // DO_PSTATS

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::get_unlit_state
//       Access: Protected, Static
//  Description: 
////////////////////////////////////////////////////////////////////
CPT(RenderState) GraphicsStateGuardian::
get_unlit_state() {
  static CPT(RenderState) state = NULL;
  if (state == (const RenderState *)NULL) {
    state = RenderState::make(LightAttrib::make_all_off());
  }
  return state;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::get_unclipped_state
//       Access: Protected, Static
//  Description: 
////////////////////////////////////////////////////////////////////
CPT(RenderState) GraphicsStateGuardian::
get_unclipped_state() {
  static CPT(RenderState) state = NULL;
  if (state == (const RenderState *)NULL) {
    state = RenderState::make(ClipPlaneAttrib::make_all_off());
  }
  return state;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::get_untextured_state
//       Access: Protected, Static
//  Description: 
////////////////////////////////////////////////////////////////////
CPT(RenderState) GraphicsStateGuardian::
get_untextured_state() {
  static CPT(RenderState) state = NULL;
  if (state == (const RenderState *)NULL) {
    state = RenderState::make(TextureAttrib::make_off());
  }
  return state;
}

void GraphicsStateGuardian::
traverse_prepared_textures(bool (*pertex_callbackfn)(TextureContext *,void *),void *callback_arg) {
  for (Textures::const_iterator ti = _prepared_textures.begin(); ti != _prepared_textures.end();
       ++ti) {
    bool bResult=(*pertex_callbackfn)(*ti,callback_arg);
    if(!bResult)
      return;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::get_factory
//       Access: Public, Static
//  Description: Returns the factory object that can be used to
//               register new kinds of GraphicsStateGuardian objects that may
//               be created.
////////////////////////////////////////////////////////////////////
GraphicsStateGuardian::GsgFactory &GraphicsStateGuardian::
get_factory() {
  if (_factory == (GsgFactory *)NULL) {
    _factory = new GsgFactory;
  }
  return (*_factory);
}

void GraphicsStateGuardian::
read_priorities(void) {
  GsgFactory &factory = get_factory();
  if (factory.get_num_preferred() == 0) {
    Config::ConfigTable::Symbol::iterator i;
    for (i = preferred_gsg_begin(); i != preferred_gsg_end(); ++i) {
      ConfigString type_name = (*i).Val();
      TypeHandle type = TypeRegistry::ptr()->find_type(type_name);
      if (type == TypeHandle::none()) {
        gsg_cat.warning()
          << "Unknown type requested for GSG preference: " << type_name
          << "\n";
      } else {
        gsg_cat.debug()
          << "Specifying type " << type << " for GSG preference.\n";
        factory.add_preferred(type);
      }
    }
  }
}
