// Filename: graphicsStateGuardianBase.h
// Created by:  drose (06Oct99)
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

#ifndef GRAPHICSSTATEGUARDIANBASE_H
#define GRAPHICSSTATEGUARDIANBASE_H

#include "pandabase.h"

#include "typedReferenceCount.h"
#include "nodeTransitions.h"
#include "luse.h"

// A handful of forward references.

class RenderBuffer;
class GraphicsWindow;

class GeomContext;
class GeomNodeContext;
class GeomNode;
class Geom;
class GeomPoint;
class GeomLine;
class GeomLinestrip;
class GeomSprite;
class GeomPolygon;
class GeomQuad;
class GeomTri;
class GeomTristrip;
class GeomTrifan;
class GeomSphere;

class TextureContext;
class Texture;
class PixelBuffer;
class RenderState;
class TransformState;

class Material;
class Fog;

class TransformTransition;
class ColorMatrixTransition;
class AlphaTransformTransition;
class TexMatrixTransition;
class ColorTransition;
class TextureTransition;
class LightTransition;
class MaterialTransition;
class RenderModeTransition;
class ColorBlendTransition;
class TextureApplyTransition;
class ColorMaskTransition;
class DepthTestTransition;
class DepthWriteTransition;
class TexGenTransition;
class CullFaceTransition;
class StencilTransition;
class ClipPlaneTransition;
class TransparencyTransition;
class FogTransition;
class LinesmoothTransition;
class PointShapeTransition;
class PolygonOffsetTransition;

class TransformAttrib;
class ColorMatrixAttrib;
class AlphaTransformAttrib;
class TexMatrixAttrib;
class ColorAttrib;
class TextureAttrib;
class LightAttrib;
class MaterialAttrib;
class RenderModeAttrib;
class ColorBlendAttrib;
class TextureApplyAttrib;
class ColorWriteAttrib;
class DepthTestAttrib;
class DepthWriteAttrib;
class TexGenAttrib;
class CullFaceAttrib;
class StencilAttrib;
class ClipPlaneAttrib;
class TransparencyAttrib;
class FogAttrib;
class LinesmoothAttrib;
class PointShapeAttrib;
class PolygonOffsetAttrib;

class Node;
class GeomNode;
class PointLight;
class DirectionalLight;
class Spotlight;
class AmbientLight;

class DisplayRegion;
class Lens;
class LensNode;

////////////////////////////////////////////////////////////////////
//       Class : GraphicsStateGuardianBase
// Description : This is a base class for the GraphicsStateGuardian
//               class, which is itself a base class for the various
//               GSG's for different platforms.  This class contains
//               all the function prototypes to support the
//               double-dispatch of GSG to geoms, transitions, etc.  It
//               lives in a separate class in its own package so we
//               can avoid circular build dependency problems.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA GraphicsStateGuardianBase : public TypedReferenceCount {
public:
  // These functions will be queried by the GeomIssuer to determine if
  // it should issue normals, texcoords, and/or colors, based on the
  // GSG's current state.
  virtual bool wants_normals(void) const=0;
  virtual bool wants_texcoords(void) const=0;
  virtual bool wants_colors(void) const=0;

  // These are some general interface functions; they're defined here
  // mainly to make it easy to call these from code in some directory
  // that display depends on.
  virtual TextureContext *prepare_texture(Texture *tex)=0;
  virtual void apply_texture(TextureContext *tc)=0;
  virtual void release_texture(TextureContext *tc)=0;

  virtual GeomNodeContext *prepare_geom_node(GeomNode *node)=0;
  virtual void draw_geom_node(GeomNode *node, GeomNodeContext *gnc)=0;
  virtual void release_geom_node(GeomNodeContext *gnc)=0;

  virtual GeomContext *prepare_geom(Geom *geom)=0;
  virtual void release_geom(GeomContext *gc)=0;

  virtual void set_state_and_transform(const RenderState *state,
                                       const TransformState *transform)=0;

  // This function may only be called during a render traversal; it
  // will compute the distance to the indicated point, assumed to be
  // in modelview coordinates, from the camera plane.  This is a
  // virtual function because different GSG's may define the modelview
  // coordinate space differently.
  virtual float compute_distance_to(const LPoint3f &point) const=0;

  // These are used to implement decals.  If polygon_offset_decals()
  // returns true, none of the remaining functions will be called,
  // since polygon offsets can be used to implement decals fully (and
  // usually faster).
  virtual bool polygon_offset_decals()=0;
  virtual CPT(RenderState) begin_decal_base_first()=0;
  virtual CPT(RenderState) begin_decal_nested()=0;
  virtual CPT(RenderState) begin_decal_base_second()=0;
  virtual void finish_decal()=0;

  // Defined here are some internal interface functions for the
  // GraphicsStateGuardian.  These are here to support
  // double-dispatching from Geoms and NodeTransitions, and are
  // intended to be invoked only directly by the appropriate Geom and
  // NodeTransition types.  They're public only because it would be too
  // inconvenient to declare each of those types to be friends of this
  // class.

  virtual void draw_point(GeomPoint *geom, GeomContext *gc)=0;
  virtual void draw_line(GeomLine *geom, GeomContext *gc)=0;
  virtual void draw_linestrip(GeomLinestrip *geom, GeomContext *gc)=0;
  virtual void draw_sprite(GeomSprite *geom, GeomContext *gc)=0;
  virtual void draw_polygon(GeomPolygon *geom, GeomContext *gc)=0;
  virtual void draw_quad(GeomQuad *geom, GeomContext *gc)=0;
  virtual void draw_tri(GeomTri *geom, GeomContext *gc)=0;
  virtual void draw_tristrip(GeomTristrip *geom, GeomContext *gc)=0;
  virtual void draw_trifan(GeomTrifan *geom, GeomContext *gc)=0;
  virtual void draw_sphere(GeomSphere *geom, GeomContext *gc)=0;

  virtual void copy_texture(TextureContext *tc, const DisplayRegion *dr)=0;
  virtual void copy_texture(TextureContext *tc, const DisplayRegion *dr,
                            const RenderBuffer &rb)=0;
  virtual void draw_texture(TextureContext *tc, const DisplayRegion *dr)=0;
  virtual void draw_texture(TextureContext *tc, const DisplayRegion *dr,
                            const RenderBuffer &rb)=0;

  virtual void texture_to_pixel_buffer(TextureContext *tc, PixelBuffer *pb)=0;
  virtual void texture_to_pixel_buffer(TextureContext *tc, PixelBuffer *pb,
                                const DisplayRegion *dr)=0;

  virtual void copy_pixel_buffer(PixelBuffer *pb, const DisplayRegion *dr)=0;
  virtual void copy_pixel_buffer(PixelBuffer *pb, const DisplayRegion *dr,
                                 const RenderBuffer &rb)=0;
  virtual void draw_pixel_buffer(PixelBuffer *pb, const DisplayRegion *dr,
                                 const NodeTransitions& na=NodeTransitions())=0;
  virtual void draw_pixel_buffer(PixelBuffer *pb, const DisplayRegion *dr,
                                 const RenderBuffer &rb,
                                 const NodeTransitions& na=NodeTransitions())=0;

  virtual void apply_material(const Material *material)=0;
  virtual void apply_fog(Fog *fog)=0;

  virtual void apply_light(PointLight *light)=0;
  virtual void apply_light(DirectionalLight *light)=0;
  virtual void apply_light(Spotlight *light)=0;
  virtual void apply_light(AmbientLight *light)=0;

  virtual void issue_transform(const TransformTransition *) { }
  virtual void issue_color_transform(const ColorMatrixTransition *) { }
  virtual void issue_alpha_transform(const AlphaTransformTransition *) { }
  virtual void issue_tex_matrix(const TexMatrixTransition *) { }
  virtual void issue_color(const ColorTransition *) { }
  virtual void issue_texture(const TextureTransition *) { }
  virtual void issue_light(const LightTransition *) { }
  virtual void issue_material(const MaterialTransition *) { }
  virtual void issue_render_mode(const RenderModeTransition *) { }
  virtual void issue_color_blend(const ColorBlendTransition *) { }
  virtual void issue_texture_apply(const TextureApplyTransition *) { }
  virtual void issue_color_mask(const ColorMaskTransition *) { }
  virtual void issue_depth_test(const DepthTestTransition *) { }
  virtual void issue_depth_write(const DepthWriteTransition *) { }
  virtual void issue_tex_gen(const TexGenTransition *) { }
  virtual void issue_cull_face(const CullFaceTransition *) { }
  virtual void issue_stencil(const StencilTransition *) { }
  virtual void issue_clip_plane(const ClipPlaneTransition *) { }
  virtual void issue_transparency(const TransparencyTransition *) { }
  virtual void issue_fog(const FogTransition *) { }
  virtual void issue_linesmooth(const LinesmoothTransition *) { }
  virtual void issue_point_shape(const PointShapeTransition *) { }
  virtual void issue_polygon_offset(const PolygonOffsetTransition *) { }

  virtual void issue_transform(const TransformAttrib *) { }
  virtual void issue_color_transform(const ColorMatrixAttrib *) { }
  virtual void issue_alpha_transform(const AlphaTransformAttrib *) { }
  virtual void issue_tex_matrix(const TexMatrixAttrib *) { }
  virtual void issue_color(const ColorAttrib *) { }
  virtual void issue_texture(const TextureAttrib *) { }
  virtual void issue_light(const LightAttrib *) { }
  virtual void issue_material(const MaterialAttrib *) { }
  virtual void issue_render_mode(const RenderModeAttrib *) { }
  virtual void issue_color_blend(const ColorBlendAttrib *) { }
  virtual void issue_texture_apply(const TextureApplyAttrib *) { }
  virtual void issue_color_write(const ColorWriteAttrib *) { }
  virtual void issue_depth_test(const DepthTestAttrib *) { }
  virtual void issue_depth_write(const DepthWriteAttrib *) { }
  virtual void issue_tex_gen(const TexGenAttrib *) { }
  virtual void issue_cull_face(const CullFaceAttrib *) { }
  virtual void issue_stencil(const StencilAttrib *) { }
  virtual void issue_clip_plane(const ClipPlaneAttrib *) { }
  virtual void issue_transparency(const TransparencyAttrib *) { }
  virtual void issue_fog(const FogAttrib *) { }
  virtual void issue_linesmooth(const LinesmoothAttrib *) { }
  virtual void issue_point_shape(const PointShapeAttrib *) { }
  virtual void issue_polygon_offset(const PolygonOffsetAttrib *) { }

PUBLISHED:
  static TypeHandle get_class_type() {
    return _type_handle;
  }

public:
  static void init_type() {
    TypedReferenceCount::init_type();
    register_type(_type_handle, "GraphicsStateGuardianBase",
                  TypedReferenceCount::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

#endif
