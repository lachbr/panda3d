#ifndef DYNAMICRENDER_H
#define DYNAMICRENDER_H

#include "config_bsp.h"
#include "pandaNode.h"
#include "simpleHashMap.h"
#include "stl_compares.h"
#include "geom.h"
#include "geomVertexFormat.h"
#include "geomVertexData.h"
#include "renderState.h"
#include "cullTraverser.h"
#include "cullTraverserData.h"
#include "geomVertexWriter.h"

#include <unordered_map>

/**
 * Node that draws dynamically filled Geoms. Children of this node are
 * responsible for filling in vertices and indices of Geoms that are fetched by
 * RenderState, vertex format, and primitive type. When all of the node's
 * children have been traversed, it adds all its dynamic Geoms for draw. The
 * next time this node is visited, all the Geoms are reset.
 */
class EXPCL_PANDABSP DynamicRender : public PandaNode {
  DECLARE_CLASS(DynamicRender, PandaNode);

PUBLISHED:
  DynamicRender(const std::string &name);

public:
  virtual bool cull_callback(CullTraverser *trav, CullTraverserData &data);

  virtual bool safe_to_combine() const;
  virtual bool safe_to_flatten() const;

public:

  enum PrimitiveType {
    PT_triangles,
    PT_lines,
    PT_line_strips,
  };

  class VertexData : public ReferenceCount {
  public:
    VertexData(const GeomVertexFormat *format);

    bool lock(int num_vertices, int &first_vertex);
    void unlock();
    void get_writer(const InternalName *name, GeomVertexWriter &writer);

    void reset();

  public:
    PT(GeomVertexData) _vertex_data;
    const GeomVertexFormat *_format;
    bool _locked;
    int _locked_vertices;
    int _position;
  };

  // This differentiates meshes
  class MeshEntry {
  public:
    MeshEntry(PrimitiveType pt, const GeomVertexFormat *fmt, const RenderState *st) {
      primitive_type = pt;
      format = fmt;
      state = st;

      hash = 0;
      hash = int_hash::add_hash(hash, pt);
      hash = pointer_hash::add_hash(hash, fmt);
      hash = pointer_hash::add_hash(hash, st);
    }

    PrimitiveType primitive_type;
    const GeomVertexFormat *format;
    const RenderState *state;

    size_t hash;

    class Hasher {
    public:
      size_t operator ()(const MeshEntry &entry) const {
        return entry.hash;
      }
    };

    class Compare {
    public:
      bool operator ()(const MeshEntry &a, const MeshEntry &b) const {
        return a.hash == b.hash;
      }
    };

  };

  // A dynamic mesh
  class Mesh : public ReferenceCount {
  private:
    void generate_triangle_indices();
    void generate_line_indices();
    void generate_line_strip_indices();

  public:
    Mesh();

    bool lock(int num_vertices);
    void get_writer(const InternalName *name, GeomVertexWriter &writer);
    void unlock();

    void reset();

    bool should_render() const;

    GeomVertexData *get_vertex_data() const;

  public:
    PrimitiveType primitive_type;
    PT(GeomPrimitive) indices;
    PT(GeomVertexArrayData) index_buffer;
    // This is shared between meshes using the same vertex format.
    VertexData *vertices;
    const RenderState *state;

  private:
    bool _should_render;
    bool _locked;
    int _locked_vertices;
    int _first_vertex;
  };

  Mesh *get_mesh(const GeomVertexFormat *format, const RenderState *state,
                 PrimitiveType primitive_type);

private:
  VertexData *find_or_create_vertex_data(const GeomVertexFormat *format);

private:
  std::unordered_map<MeshEntry, PT(Mesh), MeshEntry::Hasher, MeshEntry::Compare> _meshes;
  std::unordered_map<const GeomVertexFormat *, PT(VertexData)> _vertex_datas;

};

class EXPCL_PANDABSP DynamicCullTraverser : public CullTraverser {
  DECLARE_CLASS(DynamicCullTraverser, CullTraverser);

public:
  DynamicCullTraverser(CullTraverser *trav, DynamicRender *render);

  DynamicRender *get_render() const;

private:
  PT(DynamicRender) _render;
};

INLINE bool DynamicRender::Mesh::
should_render() const {
  return _should_render;
}

INLINE GeomVertexData *DynamicRender::Mesh::
get_vertex_data() const {
  return vertices->_vertex_data;
}

INLINE DynamicCullTraverser::
DynamicCullTraverser(CullTraverser *trav, DynamicRender *render) :
  CullTraverser(*trav) {
  _render = render;
}

INLINE DynamicRender *DynamicCullTraverser::
get_render() const {
  return _render;
}

#endif // DYNAMICRENDER_H
