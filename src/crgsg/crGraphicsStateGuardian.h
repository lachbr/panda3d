// Filename: chromium.GraphicsStateGuardian.h
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

#ifndef CRGRAPHICSSTATEGUARDIAN_H
#define CRGRAPHICSSTATEGUARDIAN_H

//#define GSG_VERBOSE

#include "pandabase.h"

#include "graphicsStateGuardian.h"
#include "geomprimitives.h"
#include "texture.h"
#include "pixelBuffer.h"
#include "displayRegion.h"
#include "material.h"
#include "textureApplyProperty.h"
#include "depthTestProperty.h"
#include "stencilProperty.h"
#include "fog.h"
#include "pt_Light.h"
#include "depthTestAttrib.h"
#include "textureApplyAttrib.h"
#include "pointerToArray.h"

#ifdef WIN32_VC
// Must include windows.h before gl.h on NT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif


#include <GL/gl.h>
// Chromium specific
#ifdef WIN32_VC // [
#define WINDOWS 1
#endif //]
#include "cr_glwrapper.h"
#include "cr_applications.h"
#include "cr_spu.h"
///////#include "cr_glstate.h"
extern SPUDispatchTable chromium;



class PlaneNode;
class Light;

#ifdef GSG_VERBOSE
ostream &output_cr_enum(ostream &out, GLenum v);
INLINE ostream &operator << (ostream &out, GLenum v) {
  return output_cr_enum(out, v);
}
#endif


////////////////////////////////////////////////////////////////////
//       Class : CRGraphicsStateGuardian
// Description : A GraphicsStateGuardian specialized for rendering
//               into OpenGL contexts.  There should be no GL calls
//               outside of this object.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDACR CRGraphicsStateGuardian : public GraphicsStateGuardian {
public:
  CRGraphicsStateGuardian(GraphicsWindow *win);
  ~CRGraphicsStateGuardian();

  virtual void reset();

  virtual void clear(const RenderBuffer &buffer);
  virtual void clear(const RenderBuffer &buffer, const DisplayRegion* region);

  virtual void prepare_display_region();
  virtual bool prepare_lens();

  virtual void render_frame();
  virtual void render_scene(Node *root, LensNode *projnode);
  virtual void render_subgraph(RenderTraverser *traverser,
                               Node *subgraph, LensNode *projnode,
                               const AllTransitionsWrapper &net_trans);
  virtual void render_subgraph(RenderTraverser *traverser,
                               Node *subgraph,
                               const AllTransitionsWrapper &net_trans);

  virtual void draw_point(GeomPoint *geom, GeomContext *gc);
  virtual void draw_line(GeomLine *geom, GeomContext *gc);
  virtual void draw_linestrip(GeomLinestrip *geom, GeomContext *gc);
  virtual void draw_sprite(GeomSprite *geom, GeomContext *gc);
  virtual void draw_polygon(GeomPolygon *geom, GeomContext *gc);
  virtual void draw_quad(GeomQuad *geom, GeomContext *gc);
  virtual void draw_tri(GeomTri *geom, GeomContext *gc);
  virtual void draw_tristrip(GeomTristrip *geom, GeomContext *gc);
  virtual void draw_trifan(GeomTrifan *geom, GeomContext *gc);
  virtual void draw_sphere(GeomSphere *geom, GeomContext *gc);

  virtual TextureContext *prepare_texture(Texture *tex);
  virtual void apply_texture(TextureContext *tc);
  virtual void release_texture(TextureContext *tc);

  virtual GeomNodeContext *prepare_geom_node(GeomNode *node);
  virtual void draw_geom_node(GeomNode *node, GeomNodeContext *gnc);
  virtual void release_geom_node(GeomNodeContext *gnc);

  virtual void copy_texture(TextureContext *tc, const DisplayRegion *dr);
  virtual void copy_texture(TextureContext *tc, const DisplayRegion *dr,
                            const RenderBuffer &rb);
  virtual void draw_texture(TextureContext *tc, const DisplayRegion *dr);
  virtual void draw_texture(TextureContext *tc, const DisplayRegion *dr,
                            const RenderBuffer &rb);

  virtual void texture_to_pixel_buffer(TextureContext *tc, PixelBuffer *pb);
  virtual void texture_to_pixel_buffer(TextureContext *tc, PixelBuffer *pb,
                                       const DisplayRegion *dr);

  virtual void copy_pixel_buffer(PixelBuffer *pb, const DisplayRegion *dr);
  virtual void copy_pixel_buffer(PixelBuffer *pb, const DisplayRegion *dr,
                                 const RenderBuffer &rb);
  virtual void draw_pixel_buffer(PixelBuffer *pb, const DisplayRegion *dr,
                                 const NodeTransitions& na=NodeTransitions());
  virtual void draw_pixel_buffer(PixelBuffer *pb, const DisplayRegion *dr,
                                 const RenderBuffer &rb,
                                 const NodeTransitions& na=NodeTransitions());

  virtual void apply_material(const Material *material);
  virtual void apply_fog(Fog *fog);

  virtual void apply_light(PointLight* light);
  virtual void apply_light(DirectionalLight* light);
  virtual void apply_light(Spotlight* light);
  virtual void apply_light(AmbientLight* light);

  virtual void issue_transform(const TransformTransition *attrib);
  virtual void issue_color_transform(const ColorMatrixTransition *attrib);
  virtual void issue_alpha_transform(const AlphaTransformTransition *attrib);
  virtual void issue_tex_matrix(const TexMatrixTransition *attrib);
  virtual void issue_color(const ColorTransition *attrib);
  virtual void issue_texture(const TextureTransition *attrib);
  virtual void issue_light(const LightTransition *attrib);
  virtual void issue_material(const MaterialTransition *attrib);
  virtual void issue_render_mode(const RenderModeTransition *attrib);
  virtual void issue_color_blend(const ColorBlendTransition *attrib);
  virtual void issue_texture_apply(const TextureApplyTransition *attrib);
  virtual void issue_color_mask(const ColorMaskTransition *attrib);
  virtual void issue_depth_test(const DepthTestTransition *attrib);
  virtual void issue_depth_write(const DepthWriteTransition *attrib);
  virtual void issue_tex_gen(const TexGenTransition *attrib);
  virtual void issue_cull_face(const CullFaceTransition *attrib);
  virtual void issue_stencil(const StencilTransition *attrib);
  virtual void issue_clip_plane(const ClipPlaneTransition *attrib);
  virtual void issue_transparency(const TransparencyTransition *attrib);
  virtual void issue_fog(const FogTransition *attrib);
  virtual void issue_linesmooth(const LinesmoothTransition *attrib);
  virtual void issue_point_shape(const PointShapeTransition *attrib);
  virtual void issue_polygon_offset(const PolygonOffsetTransition *attrib);

  virtual void issue_transform(const TransformState *transform);
  virtual void issue_texture(const TextureAttrib *attrib);
  virtual void issue_texture_apply(const TextureApplyAttrib *attrib);
  virtual void issue_cull_face(const CullFaceAttrib *attrib);
  virtual void issue_transparency(const TransparencyAttrib *attrib);
  virtual void issue_color_write(const ColorWriteAttrib *attrib);
  virtual void issue_depth_test(const DepthTestAttrib *attrib);
  virtual void issue_depth_write(const DepthWriteAttrib *attrib);

  virtual bool wants_normals(void) const;
  virtual bool wants_texcoords(void) const;

  virtual void begin_decal(GeomNode *base_geom, AllTransitionsWrapper &attrib);
  virtual void end_decal(GeomNode *base_geom);

  virtual CoordinateSystem get_internal_coordinate_system() const;
  virtual float compute_distance_to(const LPoint3f &point) const;

  void print_gfx_visual();

  //For those interested in what the guardian thinks is the current
  //enabled/disable GL State compared to what GL says it is
  void dump_state(void);

  //Methods for extracting the current color and alpha transformations
  INLINE const LMatrix4f &get_current_color_mat() const;
  INLINE const float &get_current_alpha_offset() const;
  INLINE const float &get_current_alpha_scale() const;

  void issue_transformed_color(const Colorf &color) const;

protected:
  void free_pointers();
  virtual PT(SavedFrameBuffer) save_frame_buffer(const RenderBuffer &buffer,
                                                 CPT(DisplayRegion) dr);
  virtual void restore_frame_buffer(SavedFrameBuffer *frame_buffer);

  INLINE void activate();

  INLINE void call_glClearColor(GLclampf red, GLclampf green, GLclampf blue,
                                GLclampf alpha);
  INLINE void call_glClearDepth(GLclampd depth);
  INLINE void call_glClearStencil(GLint s);
  INLINE void call_glClearAccum(GLclampf red, GLclampf green, GLclampf blue,
                                GLclampf alpha);
  INLINE void call_glDrawBuffer(GLenum mode);
  INLINE void call_glReadBuffer(GLenum mode);
  INLINE void call_glShadeModel(GLenum mode);
  INLINE void call_glBlendFunc(GLenum sfunc, GLenum dfunc);
  INLINE void call_glCullFace(GLenum mode);
  INLINE void call_glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
  INLINE void call_glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
  INLINE void call_glLightModelAmbient(const Colorf& color);
  INLINE void call_glLightModelLocal(GLboolean local);
  INLINE void call_glLightModelTwoSide(GLboolean twoside);
  INLINE void call_glStencilFunc(GLenum func,GLint refval,GLuint mask);
  INLINE void call_glStencilOp(GLenum fail,GLenum zfail,GLenum pass);
  INLINE void call_glClipPlane(GLenum plane, const double equation[4]);
  INLINE void call_glLineWidth(GLfloat width);
  INLINE void call_glPointSize(GLfloat size);
  INLINE void call_glFogMode(GLint mode);
  INLINE void call_glFogStart(GLfloat start);
  INLINE void call_glFogEnd(GLfloat end);
  INLINE void call_glFogDensity(GLfloat density);
  INLINE void call_glFogColor(const Colorf &color);
  INLINE void call_glAlphaFunc(GLenum func, GLclampf ref);
  INLINE void call_glPolygonMode(GLenum mode);

  INLINE void set_pack_alignment(GLint alignment);
  INLINE void set_unpack_alignment(GLint alignment);

  INLINE void enable_multisample(bool val);
  INLINE void enable_line_smooth(bool val);
  INLINE void enable_point_smooth(bool val);
  INLINE void enable_lighting(bool val);
  INLINE void enable_light(int light, bool val);
  INLINE void enable_texturing(bool val);
  INLINE void enable_scissor(bool val);
  INLINE void enable_dither(bool val);
  INLINE void enable_stencil_test(bool val);
  INLINE void enable_clip_plane(int clip_plane, bool val);
  INLINE void enable_multisample_alpha_one(bool val);
  INLINE void enable_multisample_alpha_mask(bool val);
  INLINE void enable_blend(bool val);
  INLINE void enable_depth_test(bool val);
  INLINE void enable_fog(bool val);
  INLINE void enable_alpha_test(bool val);
  INLINE void enable_polygon_offset(bool val);

  INLINE GLenum get_light_id(int index) const;
  INLINE GLenum get_clip_plane_id(int index) const;

  INLINE void issue_scene_graph_color();

  void set_draw_buffer(const RenderBuffer &rb);
  void set_read_buffer(const RenderBuffer &rb);

  void bind_texture(TextureContext *tc);
  void specify_texture(Texture *tex);
  bool apply_texture_immediate(Texture *tex);

  GLenum get_texture_wrap_mode(Texture::WrapMode wm);
  GLenum get_texture_filter_type(Texture::FilterType ft);
  GLenum get_image_type(PixelBuffer::Type type);
  GLenum get_external_image_format(PixelBuffer::Format format);
  GLenum get_internal_image_format(PixelBuffer::Format format);
  GLint get_texture_apply_mode_type( TextureApplyProperty::Mode am ) const;
  GLint get_texture_apply_mode_type(TextureApplyAttrib::Mode am) const;
  GLenum get_depth_func_type(DepthTestProperty::Mode m) const;
  GLenum get_depth_func_type(DepthTestAttrib::Mode m) const;
  GLenum get_stencil_func_type(StencilProperty::Mode m) const;
  GLenum get_stencil_action_type(StencilProperty::Action a) const;
  GLenum get_fog_mode_type(Fog::Mode m) const;

#ifndef NDEBUG
  void build_phony_mipmaps(Texture *tex);
  void build_phony_mipmap_level(int level, int xsize, int ysize);
  void save_mipmap_images(Texture *tex);
#endif

  GLclampf _clear_color_red, _clear_color_green, _clear_color_blue,
    _clear_color_alpha;
  GLclampd _clear_depth;
  GLint _clear_stencil;
  GLclampf _clear_accum_red, _clear_accum_green, _clear_accum_blue,
    _clear_accum_alpha;
  GLenum _draw_buffer_mode;
  GLenum _read_buffer_mode;
  GLenum _shade_model_mode;
  GLint _scissor_x;
  GLint _scissor_y;
  GLsizei _scissor_width;
  GLsizei _scissor_height;
  GLint _viewport_x;
  GLint _viewport_y;
  GLsizei _viewport_width;
  GLsizei _viewport_height;
  GLboolean _lmodel_local;
  GLboolean _lmodel_twoside;
  Colorf _lmodel_ambient;
  Colorf _material_ambient;
  Colorf _material_diffuse;
  Colorf _material_specular;
  float _material_shininess;
  Colorf _material_emission;
  GLenum _stencil_func;
  GLenum _stencil_op;
  GLfloat _line_width;
  GLfloat _point_size;
  GLenum _blend_source_func;
  GLenum _blend_dest_func;
  GLboolean _depth_mask;
  GLint _fog_mode;
  GLfloat _fog_start;
  GLfloat _fog_end;
  GLfloat _fog_density;
  Colorf _fog_color;
  GLenum _alpha_func;
  GLclampf _alpha_func_ref;
  GLenum _polygon_mode;

  GLint _pack_alignment;
  GLint _unpack_alignment;

  bool _multisample_enabled;
  bool _line_smooth_enabled;
  bool _point_smooth_enabled;
  bool _scissor_enabled;
  bool _lighting_enabled;
  bool _lighting_enabled_this_frame;
  bool _texturing_enabled;
  bool _dither_enabled;
  bool _stencil_test_enabled;
  bool _multisample_alpha_one_enabled;
  bool _multisample_alpha_mask_enabled;
  bool _blend_enabled;
  bool _depth_test_enabled;
  bool _fog_enabled;
  bool _dithering_enabled;
  bool _alpha_test_enabled;
  bool _polygon_offset_enabled;
  int _decal_level;

  class LightInfo {
  public:
    INLINE LightInfo();
    PT_Light _light;
    bool _enabled;
    bool _next_enabled;
  };

  int _max_lights;
  LightInfo *_light_info;          // LightInfo[_max_lights]
  int _cur_light_id;

  LMatrix4f _current_projection_mat;
  int _projection_mat_stack_count;

  int _max_clip_planes;
  PTA(PlaneNode *)_available_clip_plane_ids;  // pPlaneNode[_max_clip_planes]
  bool *_clip_plane_enabled;              // bool[_max_clip_planes]
  bool *_cur_clip_plane_enabled;          // bool[_max_clip_planes]
  int _cur_clip_plane_id;

  CPT(DisplayRegion) _actual_display_region;

  int _pass_number;

public:
  static GraphicsStateGuardian *
      make_GlGraphicsStateGuardian(const FactoryParams &params);

  static TypeHandle get_class_type(void);
  static void init_type(void);
  virtual TypeHandle get_type(void) const;
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

  static PStatCollector _vertices_display_list_pcollector;

private:
  static TypeHandle _type_handle;
};

#ifdef DO_PSTATS
#define DO_PSTATS_STUFF(XX) XX;
#else
#define DO_PSTATS_STUFF(XX)
#endif

#include "crGraphicsStateGuardian.I"

#endif

