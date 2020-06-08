#pragma once

/*
One GeomNode with a specific draw mask

Can have multiple Geoms.

Each Geom has a unique:
- RenderState
- Primtive type

One dynamic vertex buffer for each format.
*/

#include "config_bsp.h"
#include "geomNode.h"
#include "nodePath.h"
#include <unordered_map>
#include "dynamic_vertex_buffer.h"
#include "geomEnums.h"
#include "typeHandle.h"
#include "geomPrimitive.h"
#include "referenceCount.h"

class EXPCL_PANDABSP DynamicRender {
PUBLISHED:
  enum PrimitiveType {
    PT_triangles,
    PT_lines,
    PT_line_strip
  };

  class DynamicGeomEntry : public ReferenceCount {
  public:
    const RenderState *state;  // shared
    PT(GeomPrimitive) indices; // not shared
    DynamicVertexBuffer *vertices; // shared
    PT(Geom) geom;
    bool was_reset;
  };

  class DynamicRenderContext {
  public:
    DynamicRenderContext() {
      state = RenderState::make_empty();
      format = GeomVertexFormat::get_v3();
      prim_type = PrimitiveType::PT_triangles;
      draw_color = LColor(1);
      calc_hash();

      default_state = state;
      default_prim_type = prim_type;
      default_format = format;
      default_draw_color = draw_color;
    }
    const RenderState *state;
    DynamicRender::PrimitiveType prim_type;
    const GeomVertexFormat *format;
    LColor draw_color;
    size_t hash;

    void set_default() {
      state = default_state;
      prim_type = default_prim_type;
      format = default_format;
      draw_color = default_draw_color;
    }

    const RenderState *default_state;
    DynamicRender::PrimitiveType default_prim_type;
    const GeomVertexFormat *default_format;
    LColor default_draw_color;

    void calc_hash() {
      hash = 0u;
      hash = pointer_hash::add_hash(hash, state);
      hash = pointer_hash::add_hash(hash, format);
      hash = int_hash::add_hash(hash, prim_type);
    }
  };

  DynamicRender();

  void render_state(const RenderState *rs);
  void primitive_type(PrimitiveType type);
  void vertex_format(const GeomVertexFormat *format);
  void color(const LColor &color);

  void draw_rect(const LVector3 &mins, const LVector3 &maxs);

  void begin();
  void draw();

  void set_draw_mask(const BitMask32 &mask);

  NodePath get_dynamic_render_node_path() const;

  void reset();

private:
  DynamicGeomEntry *find_or_create_geom_entry();
  DynamicVertexBuffer *find_or_create_vertex_buffer(int max_vertices = 500);
  PT(GeomPrimitive) create_indices(int max_indices = 500);

private:
  PT(GeomNode) _gn;
  NodePath _np;

  bool _drawing;
  bool _modified;

  // Current render context
  DynamicRenderContext _context;

  // Current geom we are working on
  DynamicGeomEntry *_geom;

  std::unordered_map<size_t, PT(DynamicGeomEntry)> _geoms;

  std::unordered_map<const GeomVertexFormat *, PT(DynamicVertexBuffer)> _vertex_buffers;
};

INLINE NodePath DynamicRender::
get_dynamic_render_node_path() const {
  return _np;
}
