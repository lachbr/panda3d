#include "solidFace.h"
#include "materialReference.h"
#include "geomVertexFormat.h"
#include "internalName.h"
#include "geomEnums.h"
#include "geomNode.h"
#include "plane_culled_geom_node.h"
#include "antialiasAttrib.h"
#include "geomTriangles.h"
#include "geomLinestrips.h"
#include "geom.h"
#include "solid.h"
#include "cullTraverserData.h"
#include "cullTraverser.h"
#include "cullableObject.h"
#include "cullHandler.h"

CSolidFace::
CSolidFace(const CPlane &plane, CSolid *solid) {
  set_plane(plane);

  _solid = solid;
  _color = LColor(0.5, 0.5, 1, 1);

}

CSolidFace::
CSolidFace(const pvector<LPoint3> &verts, CMaterialReference *material, CSolid *solid) {
  _solid = solid;
  _color = LColor(0.5, 0.5, 1, 1);

  set_plane(CPlane::from_vertices(verts[0], verts[1], verts[2]));
  add_vertices(verts);
  set_material(material);
  align_texture_to_face();
}

void CSolidFace::cleanup() {
  {
    CDWriter cdata(_cycler);
    cdata->cleanup();
  }
  _solid = nullptr;
}

CSolidVertex &CSolidFace::
get_vertex(size_t n) {
  return _solid->_vertices[_indices[n]];
}

const CSolidVertex &CSolidFace::
get_vertex(size_t n) const {
  return _solid->_vertices[_indices[n]];
}

INLINE CPlane CSolidFace::
get_world_plane() const {
  LMatrix4 mat = _solid->get_np().get_mat(NodePath());
  if (mat.is_identity() || mat.is_nan()) {
    return get_plane();
  }

  CPlane plane(get_plane());
  plane.xform(mat);
  return plane;
}

void CSolidFace::
add_for_draw(CullTraverser *trav, CullTraverserData &data, const TransformState *cam) {
  const Geom *geom3d;
  const Geom *geomlines;
  const RenderState *state_3d;
  const RenderState *state_3d_lines;
  const RenderState *state_2d;
  bool draw_3d;
  bool draw_3d_lines;
  bool draw_2d;
  const CPlane *plane;
  {
    Thread *thread = trav->get_current_thread();
    CDReader cdata(_cycler, thread);
    geom3d = cdata->_geom3d;
    geomlines = cdata->_geomlines;
    state_3d = cdata->_state3d;
    state_3d_lines = cdata->_state_3dlines;
    state_2d = cdata->_state_2d;
    plane = &cdata->_plane;
    draw_3d = cdata->_draw_3d;
    draw_3d_lines = cdata->_draw_3dlines;
    draw_2d = cdata->_draw_2d;
  }

  if (!draw_2d && !draw_3d && !draw_3d_lines) {
    // Early out
    return;
  }

  const TransformState *internal_transform = data.get_internal_transform(trav);
  BitMask32 mask = trav->get_camera_mask();

  if ((mask & CSolid::_3d_mask) != 0) {
    // Don't draw 3d filled if camera is behind face plane.
    if (draw_3d && geom3d && plane->dist_to_plane(cam->get_pos()) > 0.0f) {
      const RenderState *state = data._state->compose(state_3d);
      CullableObject *object = new CullableObject(std::move(geom3d), std::move(state), std::move(internal_transform));
      trav->get_cull_handler()->record_object(object, trav);
    }
    if (draw_3d_lines && geomlines) {
      const RenderState *state = data._state->compose(state_3d_lines);
      CullableObject *object = new CullableObject(std::move(geomlines), std::move(state), std::move(internal_transform));
      trav->get_cull_handler()->record_object(object, trav);
    }

  } else if (((mask & CSolid::_2d_mask) != 0) && draw_2d && geomlines) {
    const RenderState *state = data._state->compose(state_2d);
    CullableObject *object = new CullableObject(std::move(geomlines), std::move(state), std::move(internal_transform));
    trav->get_cull_handler()->record_object(object, trav);
  }

}

void CSolidFace::
set_solid(CSolid *solid) {
  _solid = solid;
}

void CSolidFace::
add_vertex(const LPoint3 &pos) {
  CSolidVertex vert;
  vert.pos = pos;
  vert.normal = get_plane().get_normal();
  _indices.push_back(_solid->add_vertex(vert));
}

void CSolidFace::
add_vertex(const CSolidVertex &vert) {
  _indices.push_back(_solid->add_vertex(vert));
}

FaceOrientation CSolidFace::
get_orientation() const {
  CPlane plane = get_world_plane();

  // The normal must have nonzero length!
  if (plane[0] == 0 && plane[1] == 0 && plane[2] == 0) {
    return FaceOrientation::Invalid;
  }

  //
  // Find the axis that the surface normal has the greatest projection onto.
  //

  FaceOrientation orientation = FaceOrientation::Invalid;
  LVector3 normal = plane.get_normal();

  PN_stdfloat max_dot = 0.0;

  for (int i = 0; i < 6; i++) {
    PN_stdfloat dot = normal.dot(FaceNormals[i]);

    if (dot >= max_dot) {
      max_dot = dot;
      orientation = (FaceOrientation)i;
    }
  }

  return orientation;
}

LPoint3 CSolidFace::
get_abs_origin() const {
  LPoint3 avg(0);
  size_t count = _indices.size();
  for (size_t i = 0; i < count; i++) {
    avg += get_vertex(i).get_world_pos(_solid);
  }
  return avg / count;
}

void CSolidFace::
calc_texture_coordinates(bool minimize_shift) {
  if (minimize_shift) {
    _material.minimize_texture_shift_values();
  }

  if (_material._material == nullptr) {
    return;
  }

  if (_material._material->get_x_size() == 0 || _material._material->get_y_size() == 0) {
    return;
  }

  if (_material._scale[0] == 0 || _material._scale[1] == 0) {
    return;
  }

  PN_stdfloat udiv = _material._material->get_x_size() * _material._scale[0];
  PN_stdfloat uadd = _material._shift[0] / _material._material->get_x_size();
  PN_stdfloat vdiv = _material._material->get_y_size() * _material._scale[1];
  PN_stdfloat vadd = _material._shift[1] / _material._material->get_y_size();

  size_t count = _indices.size();
  for (size_t i = 0; i < count; i++) {
    CSolidVertex &vert = get_vertex(i);
    LPoint3 pos = vert.get_world_pos(_solid);
    vert.texcoord[0] = pos.dot(_material._uaxis) / udiv + uadd;
    vert.texcoord[1] = pos.dot(_material._vaxis) / vdiv + vadd;
  }

  calc_tangent_space_axes();

  if (_solid->has_geometry()) {
    submit_vertices();
  }
}

PlaneClassification CSolidFace::
classify_against_plane(const CPlane &plane) const {
  size_t front = 0, back = 0, onplane = 0;
  size_t count = _indices.size();

  for (size_t i = 0; i < count; i++) {
    int test = plane.on_plane(get_vertex(i).get_world_pos(_solid));
    if (test <= 0) {
      back++;
    }
    if (test >= 0) {
      front++;
    }
    if (test == 0) {
      onplane++;
    }
  }

  if (onplane == count) {
    return PlaneClassification::OnPlane;

  } else if (front == count) {
    return PlaneClassification::Front;

  } else if (back == count) {
    return PlaneClassification::Back;

  } else {
    return PlaneClassification::Spanning;
  }
}

void CSolidFace::
calc_tangent_space_axes() {
  //
  // get the texture space axes
  //
  LVector3 &uvect = _material._uaxis;
  LVector3 &vvect = _material._vaxis;
  CPlane plane = get_world_plane();
  LVector3 normal = plane.get_normal();

  //
  // calculate the tangent space per polygon point
  //
  size_t count = _indices.size();
  for (size_t i = 0; i < count; i++) {
    CSolidVertex &vert = get_vertex(i);
    //
    // create the axes
    //
    vert.binormal = vvect.normalized();
    vert.tangent = normal.cross(vert.binormal).normalized();
    vert.binormal = vert.tangent.cross(normal).normalized();

    //
    // adjust tangent for "backwards" mapping if need be
    //
    LVector3 tmp = uvect.cross(vvect);
    if (normal.dot(tmp) > 0.0) {
      vert.tangent = -vert.tangent;
    }
  }
}

void CSolidFace::
flip() {
  std::reverse(_indices.begin(), _indices.end());
  set_plane(CPlane::from_vertices(get_vertex(0).pos, get_vertex(1).pos, get_vertex(2).pos));
}

void CSolidFace::
xform(const LMatrix4 &mat) {
  size_t count = _indices.size();
  for (size_t i = 0; i < count; i++) {
    get_vertex(i).xform(mat);
  }
  set_plane(CPlane::from_vertices(get_vertex(0).pos, get_vertex(1).pos, get_vertex(2).pos));
}

void CSolidFace::
generate() {
  //
  // Generate indices
  //

  int count = (int)_indices.size();
  // Triangles in 3D view
  PT(GeomTriangles) prim3d = new GeomTriangles(GeomEnums::UH_static);
  for (int i = 1; i < count - 1; i++) {
    prim3d->add_vertices(_indices[i + 1], _indices[i], _indices[0]);
    prim3d->close_primitive();
  }

  // Line loop in 2d view.. using line strips
  PT(GeomLinestrips) prim2d = new GeomLinestrips(GeomEnums::UH_static);
  prim2d->add_consecutive_vertices(_indices[0], count);
  // Close off the line strip with the first vertex.. creating a line loop
  prim2d->add_vertex(_indices[0]);
  prim2d->close_primitive();

  //
  // Generate mesh objects
  //

  PT(Geom) geom3d = new Geom(_solid->_vdata);
  geom3d->add_primitive(prim3d);

  PT(Geom) geomlines = new Geom(_solid->_vdata);
  geomlines->add_primitive(prim2d);

  CDWriter cdata(_cycler);
  cdata->_geom3d = geom3d;
  cdata->_geomlines = geomlines;
}

/**
 * Copies the vertices for this face into the solid GeomVertexData.
 */
void CSolidFace::
submit_vertices() {
  CSolidVertex *data = (CSolidVertex *)_solid->_vdata->modify_array(0)->modify_handle()->get_write_pointer();
  size_t count = _indices.size();
  for (size_t i = 0; i < count; i++) {
    int index = _indices[i];
    *(data + index) = _solid->_vertices[index];
  }
}

CSolidFace::CData::
CData() {
  _geom3d = nullptr;
  _geomlines = nullptr;
  _draw_3d = true;
  _draw_2d = true;
  _draw_3dlines = true;
  _state3d = RenderState::make_empty();
  _state_3dlines = RenderState::make(ColorAttrib::make_flat(LColor(1, 1, 0, 1)),
                                     AntialiasAttrib::make(AntialiasAttrib::M_line));
  _state_2d = RenderState::make_empty();
}

CSolidFace::CData::
CData(const CSolidFace::CData &copy) {
  _geom3d = copy._geom3d;
  _geomlines = copy._geomlines;
  _draw_3d = copy._draw_3d;
  _draw_2d = copy._draw_2d;
  _draw_3dlines = copy._draw_3dlines;
  _state3d = copy._state3d;
  _state_2d = copy._state_2d;
  _state_3dlines = copy._state_3dlines;
}

CycleData *CSolidFace::CData::
make_copy() const {
  return new CData(*this);
}

void CSolidFace::CData::
cleanup() {
  _geom3d = nullptr;
  _state3d = nullptr;
  _geomlines = nullptr;
  _state_2d = nullptr;
  _state_3dlines = nullptr;
}
