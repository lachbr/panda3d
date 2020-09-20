#ifndef SOLIDNODE_H
#define SOLIDNODE_H

#include "config_bsp.h"
#include "config_editor.h"
#include "modelNode.h"
#include "luse.h"
#include "bitMask.h"
#include "copyOnWriteObject.h"
#include "cycleData.h"

class DynamicCullTraverser;
class CullTraverserData;

class EXPCL_EDITOR Vertex {
PUBLISHED:
  void set_pos(const LPoint3 &pos);
  void set_normal(const LNormal &normal);
  void set_tangent(const LVector3 &tangent);
  void set_binormal(const LVector3 &binormal);
  void set_texcoord(const LTexCoord &texcoord);
  void set_texcoord_lightmap(const LTexCoord &texcoord);

public:
  LPoint3 pos;
  LNormal normal;
  LVector3 tangent;
  LVector3 binormal;
  LTexCoord texcoord;
  LTexCoord texcoord_lightmap;
};

INLINE void Vertex::
set_pos(const LPoint3 &p) {
  pos = p;
}

INLINE void Vertex::
set_normal(const LNormal &norm) {
  normal = norm;
}

INLINE void Vertex::
set_tangent(const LVector3 &tan) {
  tangent = tan;
}

INLINE void Vertex::
set_binormal(const LVector3 &bin) {
  binormal = bin;
}

INLINE void Vertex::
set_texcoord(const LTexCoord &coord) {
  texcoord = coord;
}

INLINE void Vertex::
set_texcoord_lightmap(const LTexCoord &coord) {
  texcoord_lightmap = coord;
}

class EXPCL_EDITOR Face : public ReferenceCount {
PUBLISHED:
  Face();

  void add_vertex(const Vertex &vert);
  void remove_all_vertices();
  size_t get_num_vertices() const;
  Vertex &get_vertex(size_t n);
  MAKE_SEQ(get_vertices, get_num_vertices, get_vertex);
  MAKE_SEQ_PROPERTY(vertices, get_num_vertices, get_vertex);

  void set_state_3d(const RenderState *state);
  void set_state_3d_lines(const RenderState *state);
  void set_state_2d(const RenderState *state);
  const RenderState *get_state_3d() const;
  const RenderState *get_state_3d_lines() const;
  const RenderState *get_state_2d() const;

  MAKE_PROPERTY(state3D, get_state_3d, set_state_3d);
  MAKE_PROPERTY(state3DLines, get_state_3d_lines, set_state_3d_lines);
  MAKE_PROPERTY(state2D, get_state_2d, set_state_2d);

private:
  const RenderState *_state_3d;
  const RenderState *_state_3d_lines;
  const RenderState *_state_2d;
  pvector<Vertex> _vertices;
};

INLINE void Face::
add_vertex(const Vertex &vert) {
  _vertices.push_back(vert);
}

INLINE size_t Face::
get_num_vertices() const {
  return _vertices.size();
}

INLINE Vertex &Face::
get_vertex(size_t n) {
  return _vertices[n];
}

INLINE void Face::
remove_all_vertices() {
  _vertices.clear();
}

INLINE void Face::
set_state_3d(const RenderState *state) {
  _state_3d = state;
}

INLINE void Face::
set_state_3d_lines(const RenderState *state) {
  _state_3d_lines = state;
}

INLINE void Face::
set_state_2d(const RenderState *state) {
  _state_2d = state;
}

INLINE const RenderState *Face::
get_state_3d() const {
  return _state_3d;
}

INLINE const RenderState *Face::
get_state_3d_lines() const {
  return _state_3d_lines;
}

INLINE const RenderState *Face::
get_state_2d() const {
  return _state_2d;
}

class EXPCL_EDITOR SolidNode : public ModelNode {
  DECLARE_CLASS(SolidNode, ModelNode);

PUBLISHED:
  SolidNode();

  void set_draw_3d(bool draw);
  void set_draw_3d_lines(bool draw);
  void set_draw_2d(bool draw);

  void add_face(PT(Face) face);
  void remove_all_faces();
  Face *get_face(size_t n);
  size_t get_num_faces() const;
  MAKE_SEQ(get_faces, get_num_faces, get_face);
  MAKE_SEQ_PROPERTY(faces, get_num_faces, get_face);

  static void set_3d_mask(const BitMask32 &mask);
  static void set_2d_mask(const BitMask32 &mask);

public:
  virtual bool is_renderable() const;
  virtual void add_for_draw(CullTraverser *trav, CullTraverserData &data);

  virtual void compute_internal_bounds(CPT(BoundingVolume) &internal_bounds,
                                       int &internal_vertices,
                                       int pipeline_stage,
                                       Thread *thread) const;
  virtual CPT(TransformState) calc_tight_bounds(LPoint3 &min_point, LPoint3 &max_point, bool &found_any,
                  const TransformState *transform, Thread *current_thread) const;

private:
  void draw_3d(DynamicCullTraverser *trav, CullTraverserData &data);
  void draw_3d_lines(DynamicCullTraverser *trav, CullTraverserData &data);
  void draw_2d(DynamicCullTraverser *trav, CullTraverserData &data);

private:
  typedef CopyOnWriteObj<pvector<PT(Face)>> FaceList;
  class CData : public CycleData {
  public:
    CData();
    CData(const CData &other);
    virtual CycleData *make_copy() const;

    CPT(FaceList) get_faces() const;
    PT(FaceList) modify_faces();

  private:
    COWPT(FaceList) _faces;
  };

  PipelineCycler<CData> _cycler;
  typedef CycleDataReader<CData> CDReader;
  typedef CycleDataWriter<CData> CDWriter;
  typedef CycleDataStageWriter<CData> CDStageWriter;
  typedef CycleDataStageReader<CData> CDStageReader;

  bool _draw_3d;
  bool _draw_3d_lines;
  bool _draw_2d;

  static BitMask32 _3d_mask;
  static BitMask32 _2d_mask;
};

INLINE SolidNode::
SolidNode() :
  ModelNode("solid") {

  _draw_3d = true;
  _draw_3d_lines = false;
  _draw_2d = true;
}

INLINE void SolidNode::
set_draw_3d(bool draw) {
  _draw_3d = draw;
}

INLINE void SolidNode::
set_draw_3d_lines(bool draw) {
  _draw_3d_lines = draw;
}

INLINE void SolidNode::
set_draw_2d(bool draw) {
  _draw_2d = draw;
}

INLINE Face *SolidNode::
get_face(size_t n) {
  CDReader cdata(_cycler);
  return (*cdata->get_faces())[n];
}

INLINE size_t SolidNode::
get_num_faces() const {
  CDReader cdata(_cycler);
  return cdata->get_faces()->size();
}

INLINE SolidNode::CData::
CData() :
  CycleData() {
  _faces = new FaceList;
}

INLINE SolidNode::CData::
CData(const CData &other) :
  CycleData() {
  _faces = other._faces;
}

INLINE CPT(SolidNode::FaceList) SolidNode::CData::
get_faces() const {
  return _faces.get_read_pointer();
}

INLINE PT(SolidNode::FaceList) SolidNode::CData::
modify_faces() {
  return _faces.get_write_pointer();
}

#endif // SOLIDNODE_H
