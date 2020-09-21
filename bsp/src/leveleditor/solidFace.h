#pragma once

#include "config_leveleditor.h"
#include "referenceCount.h"
#include "nodePath.h"
#include "cPlane.h"
#include "luse.h"
#include "solidVertex.h"
#include "pvector.h"
#include "cPointCloud.h"
#include "faceOrientation.h"
#include "faceMaterial.h"
#include "geomVertexData.h"
#include "planeClassification.h"
#include "bspMaterialAttrib.h"
#include "materialReference.h"
#include "renderState.h"
#include "geom.h"
#include "colorAttrib.h"
#include "cycleData.h"
#include "pipelineCycler.h"
#include "boundingSphere.h"

class CSolid;

class EXPCL_EDITOR CSolidFace : public ReferenceCount {
PUBLISHED:
  CSolidFace(const CPlane &plane = CPlane(0, 0, 1, 0), CSolid *solid = nullptr);
  CSolidFace(const pvector<LPoint3> &vertices, CMaterialReference *material, CSolid *solid);
  void cleanup();

  FaceOrientation get_orientation() const;
  void set_solid(CSolid *solid);
  CSolid *get_solid() const;

  LPoint3 get_abs_origin() const;
  CPlane get_world_plane() const;
  void set_plane(const CPlane &plane);
  const CPlane &get_plane() const;
  MAKE_PROPERTY(plane, get_plane, set_plane);

  PlaneClassification classify_against_plane(const CPlane &plane) const;

  void calc_texture_coordinates(bool minimize_shift);
  void calc_tangent_space_axes();

  void xform(const LMatrix4 &mat);

  void show_clip_vis_remove();
  void show_clip_vis_keep();
  void show_3d_lines();
  void hide_3d_lines();

  const RenderState *get_state_3d() const;
  void set_state_3d(const RenderState *state);
  MAKE_PROPERTY(state_3d, get_state_3d, set_state_3d);

  const RenderState *get_state_2d() const;
  void set_state_2d(const RenderState *state);
  MAKE_PROPERTY(state_2d, get_state_2d, set_state_2d);

  const RenderState *get_state_3d_lines() const;
  void set_state_3d_lines(const RenderState *state);
  MAKE_PROPERTY(state_3d_lines, get_state_3d_lines, set_state_3d_lines);

  void set_color(const LColor &color);
  void set_material(CMaterialReference *mat);
  void set_face_material(const CFaceMaterial &mat);

  void align_texture_to_face();
  void align_texture_to_world();

  void flip();

  void generate();

  void submit_vertices();

  CFaceMaterial &get_material();
  MAKE_PROPERTY(material, get_material);

  const LColor &get_color() const;
  MAKE_PROPERTY(color, get_color);

  void add_vertex(const LPoint3 &pos);
  void add_vertex(const CSolidVertex &vert);
  void add_vertices(const pvector<LPoint3> &verts);

  void clear_indices();
  size_t get_num_indices() const;
  int get_index(size_t n) const;
  const CSolidVertex &get_vertex(size_t n) const;
  CSolidVertex &get_vertex(size_t n);
  MAKE_SEQ(get_indices, get_num_indices, get_index);
  MAKE_SEQ_PROPERTY(indices, get_num_indices, get_index);
  MAKE_SEQ(get_vertices, get_num_indices, get_vertex);
  MAKE_SEQ_PROPERTY(vertices, get_num_indices, get_vertex);

  const Geom *get_geom_3d() const;
  const Geom *get_geom_lines() const;
  bool should_draw_3d() const;
  bool should_draw_3d_lines() const;
  bool should_draw_2d() const;

public:
  void add_for_draw(CullTraverser *trav, CullTraverserData &data, const TransformState *cam);

private:

  /**
   * This data is modified/accessed on threads other than App.
   */
  class CData : public CycleData {
  public:
    CData();
    CData(const CData &copy);
    virtual CycleData *make_copy() const;

    void cleanup();

    PT(Geom) _geom3d;
    CPT(RenderState) _state3d;
    bool _draw_3d;

    CPT(RenderState) _state_3dlines;
    bool _draw_3dlines;

    PT(Geom) _geomlines;
    CPT(RenderState) _state_2d;
    bool _draw_2d;

    CPlane _plane; // The plane that the face lies on.
  };

  PipelineCycler<CData> _cycler;
  typedef CycleDataReader<CData> CDReader;
  typedef CycleDataWriter<CData> CDWriter;
  typedef CycleDataStageReader<CData> CDStageReader;
  typedef CycleDataLockedStageReader<CData> CDLockedStageReader;
  typedef CycleDataStageWriter<CData> CDStageWriter;

  CSolid *_solid;

  CFaceMaterial _material;

  // The vertex indices that make up the polygon.
  // NOTE: the actual vertex data is stored on the solid.
  pvector<int> _indices;

  LColor _color;
};

INLINE void CSolidFace::
add_vertices(const pvector<LPoint3> &verts) {
  for (size_t i = 0; i < verts.size(); i++) {
    add_vertex(verts[i]);
  }
}

INLINE void CSolidFace::
clear_indices() {
  _indices.clear();
}

INLINE int CSolidFace::
get_index(size_t n) const {
  return _indices[n];
}

INLINE size_t CSolidFace::
get_num_indices() const {
  return _indices.size();
}

INLINE const LColor &CSolidFace::
get_color() const {
  return _color;
}

INLINE void CSolidFace::
set_plane(const CPlane &plane) {
  CDWriter cdata(_cycler);
  cdata->_plane = plane;
}

INLINE const CPlane &CSolidFace::
get_plane() const {
  CDReader cdata(_cycler);
  return cdata->_plane;
}

INLINE CFaceMaterial &CSolidFace::
get_material() {
  return _material;
}

INLINE void CSolidFace::
align_texture_to_face() {
  _material.align_texture_to_face(this);
  calc_texture_coordinates(true);
}

INLINE void CSolidFace::
align_texture_to_world() {
  _material.align_texture_to_world(this);
  calc_texture_coordinates(true);
}

INLINE void CSolidFace::
set_face_material(const CFaceMaterial &mat) {
  _material = mat;
  set_material(_material._material);
  calc_texture_coordinates(true);
}

INLINE void CSolidFace::
set_color(const LColor &color) {
  _color = color;

  CDWriter cdata(_cycler);
  cdata->_state_2d = cdata->_state_2d->set_attrib(ColorAttrib::make_flat(color));
}

INLINE void CSolidFace::
set_material(CMaterialReference *mat) {
  _material._material = mat;

  CDWriter cdata(_cycler);
  if (mat) {
    cdata->_state3d = cdata->_state3d->set_attrib(BSPMaterialAttrib::make(mat->get_material()));
  } else {
    cdata->_state3d = cdata->_state3d->remove_attrib(BSPMaterialAttrib::get_class_slot());
  }
}

INLINE void CSolidFace::
show_clip_vis_remove() {
  CDWriter cdata(_cycler);

  cdata->_draw_3d = false;
  cdata->_state_3dlines = cdata->_state_3dlines->set_attrib(ColorAttrib::make_flat(LColor(1, 0, 0, 1)));
  cdata->_state_2d = cdata->_state_2d->set_attrib(ColorAttrib::make_flat(LColor(1, 0, 0, 1)));
}

INLINE void CSolidFace::
show_clip_vis_keep() {
  CDWriter cdata(_cycler);

  cdata->_draw_3d = true;
  cdata->_state_3dlines = cdata->_state_3dlines->set_attrib(ColorAttrib::make_flat(LColor(1, 1, 0, 1)));
  cdata->_state_2d = cdata->_state_2d->set_attrib(ColorAttrib::make_flat(LColor(1)));
}

INLINE void CSolidFace::
show_3d_lines() {
  CDWriter cdata(_cycler);
  cdata->_draw_3dlines = true;
}

INLINE void CSolidFace::
hide_3d_lines() {
  CDWriter cdata(_cycler);
  cdata->_draw_3dlines = false;
}

INLINE void CSolidFace::
set_state_3d(const RenderState *state) {
  CDWriter cdata(_cycler);
  cdata->_state3d = state;
}

INLINE const RenderState *CSolidFace::
get_state_3d() const {
  CDReader cdata(_cycler);
  return cdata->_state3d;
}

INLINE void CSolidFace::
set_state_3d_lines(const RenderState *state) {
  CDWriter cdata(_cycler);
  cdata->_state_3dlines = state;
}

INLINE const RenderState *CSolidFace::
get_state_3d_lines() const {
  CDReader cdata(_cycler);
  return cdata->_state_3dlines;
}

INLINE void CSolidFace::
set_state_2d(const RenderState *state) {
  CDWriter cdata(_cycler);
  cdata->_state_2d = state;
}

INLINE const RenderState *CSolidFace::
get_state_2d() const {
  CDReader cdata(_cycler);
  return cdata->_state_2d;
}

INLINE const Geom *CSolidFace::
get_geom_3d() const {
  CDReader cdata(_cycler);
  return cdata->_geom3d;
}

INLINE const Geom *CSolidFace::
get_geom_lines() const {
  CDReader cdata(_cycler);
  return cdata->_geomlines;
}

INLINE bool CSolidFace::
should_draw_3d() const {
  CDReader cdata(_cycler);
  return cdata->_draw_3d;
}

INLINE bool CSolidFace::
should_draw_3d_lines() const {
  CDReader cdata(_cycler);
  return cdata->_draw_3dlines;
}

INLINE bool CSolidFace::
should_draw_2d() const {
  CDReader cdata(_cycler);
  return cdata->_draw_2d;
}

INLINE CSolid *CSolidFace::
get_solid() const {
  return _solid;
}
