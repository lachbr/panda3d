// Filename: graphicsStateGuardian.h
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

#ifndef GRAPHICSSTATEGUARDIAN_H
#define GRAPHICSSTATEGUARDIAN_H

#include "pandabase.h"

#include "savedFrameBuffer.h"
#include "frameBufferStack.h"
#include "displayRegionStack.h"
#include "lensStack.h"

#include "graphicsStateGuardianBase.h"
#include "sceneSetup.h"
#include "luse.h"
#include "coordinateSystem.h"
#include "factory.h"
#include "pStatCollector.h"
#include "transformState.h"
#include "renderState.h"
#include "light.h"
#include "colorWriteAttrib.h"
#include "colorBlendAttrib.h"
#include "transparencyAttrib.h"
#include "config_display.h"

#include "notify.h"
#include "pvector.h"

class ClearableRegion;

////////////////////////////////////////////////////////////////////
//       Class : GraphicsStateGuardian
// Description : Encapsulates all the communication with a particular
//               instance of a given rendering backend.  Tries to
//               guarantee that redundant state-change requests are
//               not issued (hence "state guardian").
//
//               There will be one of these objects for each different
//               graphics context active in the system.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA GraphicsStateGuardian : public GraphicsStateGuardianBase {
  //
  // Interfaces all GSGs should have
  //
public:
  GraphicsStateGuardian(GraphicsWindow *win);
  virtual ~GraphicsStateGuardian();

PUBLISHED:
  void release_all_textures();
  void release_all_geoms();

public:
  INLINE bool is_closed() const;

  INLINE void set_scene(SceneSetup *scene_setup);
  INLINE SceneSetup *get_scene() const;

  virtual TextureContext *prepare_texture(Texture *tex);
  virtual void apply_texture(TextureContext *tc);
  virtual void release_texture(TextureContext *tc);

  virtual GeomNodeContext *prepare_geom_node(GeomNode *node);
  virtual void draw_geom_node(GeomNode *node, const RenderState *state,
                              GeomNodeContext *gnc);
  virtual void release_geom_node(GeomNodeContext *gnc);

  virtual GeomContext *prepare_geom(Geom *geom);
  virtual void release_geom(GeomContext *gc);

  virtual void set_state_and_transform(const RenderState *state,
                                       const TransformState *transform);

  virtual void set_color_clear_value(const Colorf &value);
  virtual void set_depth_clear_value(const float value);
  virtual void do_clear(const RenderBuffer &buffer)=0;

  void clear(ClearableRegion *clearable);
  INLINE void clear(DisplayRegion *dr);

  virtual void prepare_display_region()=0;
  virtual bool prepare_lens();

  INLINE void enable_normals(bool val) { _normals_enabled = val; }

  virtual void begin_frame();
  virtual void end_frame();

  // These functions will be queried by the GeomIssuer to determine if
  // it should issue normals, texcoords, and/or colors, based on the
  // GSG's current state.
  virtual bool wants_normals() const;
  virtual bool wants_texcoords() const;
  virtual bool wants_colors() const;

  virtual bool depth_offset_decals();
  virtual CPT(RenderState) begin_decal_base_first();
  virtual CPT(RenderState) begin_decal_nested();
  virtual CPT(RenderState) begin_decal_base_second();
  virtual void finish_decal();

  virtual void reset();

  INLINE void modify_state(const RenderState *state);
  INLINE void set_state(const RenderState *state);
  INLINE void set_transform(const TransformState *transform);

  RenderBuffer get_render_buffer(int buffer_type);

  INLINE const DisplayRegion *get_current_display_region(void) const;
  INLINE const Lens *get_current_lens() const;

  INLINE DisplayRegionStack push_display_region(const DisplayRegion *dr);
  INLINE void pop_display_region(DisplayRegionStack &node);
  INLINE FrameBufferStack push_frame_buffer(const RenderBuffer &buffer,
                                            const DisplayRegion *dr);
  INLINE void pop_frame_buffer(FrameBufferStack &node);

  INLINE LensStack push_lens(const Lens *lens);
  INLINE void pop_lens(LensStack &stack);
  INLINE bool set_lens(const Lens *lens);

  INLINE void set_coordinate_system(CoordinateSystem cs);
  INLINE CoordinateSystem get_coordinate_system() const;
  virtual CoordinateSystem get_internal_coordinate_system() const;

  virtual void issue_transform(const TransformState *transform);
  virtual void issue_color_scale(const ColorScaleAttrib *attrib);
  virtual void issue_color(const ColorAttrib *attrib);
  virtual void issue_light(const LightAttrib *attrib);
  virtual void issue_color_write(const ColorWriteAttrib *attrib);
  virtual void issue_transparency(const TransparencyAttrib *attrib);
  virtual void issue_color_blend(const ColorBlendAttrib *attrib);

  virtual void bind_light(PointLight *light, int light_id);
  virtual void bind_light(DirectionalLight *light, int light_id);
  virtual void bind_light(Spotlight *light, int light_id);

protected:
  INLINE Light *get_light(int light_id) const;
  virtual bool slot_new_light(int light_id);
  virtual void enable_lighting(bool enable);
  virtual void set_ambient_light(const Colorf &color);
  virtual void enable_light(int light_id, bool enable);
  virtual void begin_bind_lights();
  virtual void end_bind_lights();

  virtual void set_blend_mode(ColorWriteAttrib::Mode color_write_mode,
                              ColorBlendAttrib::Mode color_blend_mode,
                              TransparencyAttrib::Mode transparency_mode);

  virtual PT(SavedFrameBuffer) save_frame_buffer(const RenderBuffer &buffer,
                                                 CPT(DisplayRegion) dr)=0;
  virtual void restore_frame_buffer(SavedFrameBuffer *frame_buffer)=0;

  bool mark_prepared_texture(TextureContext *tc);
  bool unmark_prepared_texture(TextureContext *tc);
  bool mark_prepared_geom(GeomContext *gc);
  bool unmark_prepared_geom(GeomContext *gc);
  bool mark_prepared_geom_node(GeomNodeContext *gnc);
  bool unmark_prepared_geom_node(GeomNodeContext *gnc);

  virtual void close_gsg();

#ifdef DO_PSTATS
  // These functions are used to update the active texture memory
  // usage record (and other frame-based measurements) in Pstats.
  void init_frame_pstats();
  void add_to_texture_record(TextureContext *tc);
  void add_to_geom_record(GeomContext *gc);
  void add_to_geom_node_record(GeomNodeContext *gnc);
  void record_state_change(TypeHandle type);
  pset<TextureContext *> _current_textures;
  pset<GeomContext *> _current_geoms;
  pset<GeomNodeContext *> _current_geom_nodes;
#else
  INLINE void init_frame_pstats() { }
  INLINE void add_to_texture_record(TextureContext *) { }
  INLINE void add_to_geom_record(GeomContext *) { }
  INLINE void add_to_geom_node_record(GeomNodeContext *) { }
  INLINE void record_state_change(TypeHandle) { }
#endif

  static CPT(RenderState) get_unlit_state();
  static CPT(RenderState) get_untextured_state();

protected:
  PT(SceneSetup) _scene_setup;

  CPT(RenderState) _state;
  CPT(TransformState) _transform;

  int _buffer_mask;
  Colorf _color_clear_value;
  float _depth_clear_value;
  bool _stencil_clear_value;
  Colorf _accum_clear_value;
  int _clear_buffer_type;

  int _display_region_stack_level;
  int _frame_buffer_stack_level;
  int _lens_stack_level;

  GraphicsWindow *_win;

  CPT(DisplayRegion) _current_display_region;
  CPT(Lens) _current_lens;

  // This is used by wants_normals()
  bool _normals_enabled;

  CoordinateSystem _coordinate_system;

  Colorf _scene_graph_color;
  bool _has_scene_graph_color;
  bool _scene_graph_color_stale;
  bool _vertex_colors_enabled;
  bool _lighting_enabled;

  enum ColorTransform {
    CT_offset  = 0x01,
    CT_scale   = 0x02,
  };
  int _color_transform_enabled;  // Zero or more of ColorTransform bits, above.
  LVecBase4f _current_color_offset;
  LVecBase4f _current_color_scale;

  ColorWriteAttrib::Mode _color_write_mode;
  ColorBlendAttrib::Mode _color_blend_mode;
  TransparencyAttrib::Mode _transparency_mode;

public:
  // Statistics
  static PStatCollector _total_texusage_pcollector;
  static PStatCollector _active_texusage_pcollector;
  static PStatCollector _total_geom_pcollector;
  static PStatCollector _active_geom_pcollector;
  static PStatCollector _total_geom_node_pcollector;
  static PStatCollector _active_geom_node_pcollector;
  static PStatCollector _total_texmem_pcollector;
  static PStatCollector _used_texmem_pcollector;
  static PStatCollector _texmgrmem_total_pcollector;
  static PStatCollector _texmgrmem_resident_pcollector;
  static PStatCollector _vertices_pcollector;
  static PStatCollector _vertices_tristrip_pcollector;
  static PStatCollector _vertices_trifan_pcollector;
  static PStatCollector _vertices_tri_pcollector;
  static PStatCollector _vertices_other_pcollector;
  static PStatCollector _state_changes_pcollector;
  static PStatCollector _transform_state_pcollector;
  static PStatCollector _texture_state_pcollector;
  static PStatCollector _nodes_pcollector;
  static PStatCollector _geom_nodes_pcollector;
  static PStatCollector _frustum_cull_volumes_pcollector;
  static PStatCollector _frustum_cull_transforms_pcollector;
  static PStatCollector _set_state_pcollector;
  static PStatCollector _draw_primitive_pcollector;

private:
  class LightInfo {
  public:
    INLINE LightInfo();
    PT(Light) _light;
    bool _enabled;
    bool _next_enabled;
  };

  pvector<LightInfo> _light_info;
  bool _lighting_enabled_this_frame;

  // NOTE: on win32 another DLL (e.g. libpandadx.dll) cannot access
  // these sets directly due to exported template issue
  typedef pset<TextureContext *> Textures;
  Textures _prepared_textures;  
  typedef pset<GeomContext *> Geoms;
  Geoms _prepared_geoms;  
  typedef pset<GeomNodeContext *> GeomNodes;
  GeomNodes _prepared_geom_nodes;  

public:
  void traverse_prepared_textures(bool (*pertex_callbackfn)(TextureContext *,void *),void *callback_arg);

// factory stuff
public:
  typedef Factory<GraphicsStateGuardian> GsgFactory;
  typedef FactoryParam GsgParam;

  // Make a factory parameter type for the window pointer
  class EXPCL_PANDA GsgWindow : public GsgParam {
  public:
    INLINE GsgWindow(GraphicsWindow* w) : GsgParam(), _w(w) {}
    virtual ~GsgWindow(void);
    INLINE GraphicsWindow* get_window(void) { return _w; }
  public:
    static TypeHandle get_class_type(void);
    static void init_type(void);
    virtual TypeHandle get_type(void) const;
    virtual TypeHandle force_init_type(void);
  private:
    GraphicsWindow* _w;
    static TypeHandle _type_handle;

    INLINE GsgWindow(void) : GsgParam() {}
  };

  static GsgFactory &get_factory();

private:
  static void read_priorities(void);
  static GsgFactory *_factory;

public:
  INLINE GraphicsWindow* get_window(void) const { return _win; }

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }

public:
  static void init_type() {
    GraphicsStateGuardianBase::init_type();
    register_type(_type_handle, "GraphicsStateGuardian",
                  GraphicsStateGuardianBase::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;

  friend class GraphicsPipe;
  friend class GraphicsWindow;
};

#include "graphicsStateGuardian.I"

#endif
