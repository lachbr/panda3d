#pragma once

#include "config_bsp.h"
#include "config_editor.h"
#include "modelNode.h"
#include "solidFace.h"
#include "pvector.h"
#include "geomVertexData.h"
#include "bitMask.h"
#include "copyOnWriteObject.h"
#include "cycleData.h"

#define FOR_EACH_FACE(func) \
CDReader cdata(_cycler); \
CPT(FaceList) faces = cdata->get_faces(); \
size_t count = faces->size(); \
for (size_t i = 0; i < count; i++) { \
  (*faces)[i]->func(); \
}

class CullTraverser;
class CullTraverserData;

/**
 * C++ implementation of a solid, which is a convex piece of editor geometry.
 * The solid is one GeomNode, and each face of the solid contains Geoms. The
 * Geoms of a face that are rendered is determined by the viewport that is being
 * rendered into.
 */
class EXPCL_EDITOR CSolid : public ModelNode {
  DECLARE_CLASS(CSolid, ModelNode);

PUBLISHED:
  CSolid();

  void align_textures_to_faces();
  void align_textures_to_world();
  void generate_faces();

  void show_clip_vis_remove();
  void show_clip_vis_keep();
  void show_bounding_box();
  void hide_bounding_box();

  static PT(CSolid) create_from_intersecting_planes(const pvector<CPlane> &planes,
                                                    bool generate_faces = true);

  void add_face(PT(CSolidFace) face);
  size_t get_num_faces() const;
  CSolidFace *get_face(size_t n) const;
  MAKE_SEQ_PROPERTY(faces, get_num_faces, get_face);

  NodePath get_np() const;
  MAKE_PROPERTY(np, get_np);

  void cleanup();

  EXTENSION(PyObject *split(const CPlane &plane));

  void submit_vertices();

  static void set_3d_mask(const BitMask32 &mask);
  static void set_2d_mask(const BitMask32 &mask);

  bool has_geometry() const;

public:
  bool split(const CPlane &plane, PT(CSolid) *front, PT(CSolid) *back);

  virtual bool is_renderable() const;
  virtual void add_for_draw(CullTraverser *trav, CullTraverserData &data);
  virtual void compute_internal_bounds(CPT(BoundingVolume) &internal_bounds, int &internal_vertices,
                                       int pipeline_stage, Thread *current_thread) const;
  virtual CPT(TransformState) calc_tight_bounds(LPoint3 &min_point, LPoint3 &max_point, bool &found_any,
                  const TransformState *transform, Thread *current_thread) const;

private:
  int add_vertex(const CSolidVertex &vertex);

private:

  typedef CopyOnWriteObj<pvector<PT(CSolidFace)>> FaceList;
  class CData : public CycleData {
  public:
    CData();
    CData(const CData &copy);
    virtual CycleData *make_copy() const;

    CPT(FaceList) get_faces() const;
    PT(FaceList) modify_faces();
    void set_faces(FaceList *faces);

  private:
    COWPT(FaceList) _faces;
  };

  CPT(FaceList) get_faces(Thread *thread) const;

  typedef PipelineCycler<CData> Cycler;
  typedef CycleDataReader<CData> CDReader;
  typedef CycleDataWriter<CData> CDWriter;
  typedef CycleDataStageWriter<CData> CDStageWriter;

  Cycler _cycler;

  pvector<CSolidVertex> _vertices;
  NodePath _np;
  PT(GeomVertexData) _vdata;
  bool _has_geometry;

  static BitMask32 _3d_mask;
  static BitMask32 _2d_mask;

  friend class CSolidFace;
};

INLINE CSolid::
CSolid() :
  ModelNode("solid") {

  _has_geometry = false;
  _vdata = nullptr;
  _np = NodePath(this);
}

INLINE bool CSolid::
has_geometry() const {
  return _has_geometry;
}

INLINE int CSolid::
add_vertex(const CSolidVertex &vertex) {
  _vertices.push_back(vertex);
  return (int)(_vertices.size() - 1);
}

INLINE NodePath CSolid::
get_np() const {
  return _np;
}

INLINE size_t CSolid::
get_num_faces() const {
  CDReader cdata(_cycler);
  return cdata->get_faces()->size();
}

INLINE CSolidFace *CSolid::
get_face(size_t n) const {
  CDReader cdata(_cycler);
  return (*cdata->get_faces())[n];
}

INLINE void CSolid::
align_textures_to_faces() {
  FOR_EACH_FACE(align_texture_to_face);
}

INLINE void CSolid::
align_textures_to_world() {
  FOR_EACH_FACE(align_texture_to_world);
}

INLINE void CSolid::
show_clip_vis_remove() {
  FOR_EACH_FACE(show_clip_vis_remove);
}

INLINE void CSolid::
show_clip_vis_keep() {
  FOR_EACH_FACE(show_clip_vis_keep);
}

INLINE void CSolid::
show_bounding_box() {
  FOR_EACH_FACE(show_3d_lines);
}

INLINE void CSolid::
hide_bounding_box() {
  FOR_EACH_FACE(hide_3d_lines);
}

INLINE CPT(CSolid::FaceList) CSolid::
get_faces(Thread *thread) const {
  CDReader cdata(_cycler, thread);
  return cdata->get_faces();
}

INLINE CSolid::CData::
CData() {
  _faces = new CSolid::FaceList;
}

INLINE CSolid::CData::
CData(const CSolid::CData &copy) {
  _faces = copy._faces;
}

INLINE CPT(CSolid::FaceList) CSolid::CData::
get_faces() const {
  return _faces.get_read_pointer();
}

INLINE PT(CSolid::FaceList) CSolid::CData::
modify_faces() {
  return _faces.get_write_pointer();
}

INLINE void CSolid::CData::
set_faces(FaceList *faces) {
  _faces = faces;
}
