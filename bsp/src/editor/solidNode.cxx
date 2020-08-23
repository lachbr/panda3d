#include "solidNode.h"
#include "boundingBox.h"
#include "dynamicRender.h"

static CPT(GeomVertexFormat) format_3d = nullptr;
static CPT(GeomVertexFormat) format_lines = nullptr;

static const GeomVertexFormat *get_format_3d() {
  if (!format_3d) {
    PT(GeomVertexArrayFormat) arr = new GeomVertexArrayFormat;
    // NOTE: this must be in the same order as the members of CSolidVertex
    arr->add_column(InternalName::get_vertex(), 3, GeomEnums::NT_stdfloat, GeomEnums::C_point);
    arr->add_column(InternalName::get_normal(), 3, GeomEnums::NT_stdfloat, GeomEnums::C_normal);
    arr->add_column(InternalName::get_tangent(), 3, GeomEnums::NT_stdfloat, GeomEnums::C_vector);
    arr->add_column(InternalName::get_binormal(), 3, GeomEnums::NT_stdfloat, GeomEnums::C_vector);
    arr->add_column(InternalName::get_texcoord(), 2, GeomEnums::NT_stdfloat, GeomEnums::C_texcoord);
    arr->add_column(InternalName::get_texcoord_name("lightmap"), 2, GeomEnums::NT_stdfloat, GeomEnums::C_texcoord);
    format_3d = GeomVertexFormat::register_format(arr);
  }

  return format_3d;
}

static const GeomVertexFormat *get_format_lines() {
  if (!format_lines) {
    PT(GeomVertexArrayFormat) arr = new GeomVertexArrayFormat;
    arr->add_column(InternalName::get_vertex(), 3, GeomEnums::NT_stdfloat, GeomEnums::C_point);
    format_lines = GeomVertexFormat::register_format(arr);
  }

  return format_lines;
}

Face::
Face() {
  _state_3d = RenderState::make_empty();
  _state_2d = RenderState::make_empty();
  _state_3d_lines = RenderState::make_empty();
}

IMPLEMENT_CLASS(SolidNode);

BitMask32 SolidNode::_3d_mask = BitMask32::bit(0);
BitMask32 SolidNode::_2d_mask = BitMask32::bit(1);

void SolidNode::
set_3d_mask(const BitMask32 &mask) {
  _3d_mask = mask;
}

void SolidNode::
set_2d_mask(const BitMask32 &mask) {
  _2d_mask = mask;
}

void SolidNode::
add_face(PT(Face) face) {
  Thread *current_thread = Thread::get_current_thread();
  OPEN_ITERATE_CURRENT_AND_UPSTREAM(_cycler, current_thread) {
    CDStageWriter cdata(_cycler, pipeline_stage, current_thread);
    cdata->modify_faces()->push_back(face);
  }
  CLOSE_ITERATE_CURRENT_AND_UPSTREAM(_cycler);

  mark_internal_bounds_stale();
}

void SolidNode::
remove_all_faces() {
  Thread *current_thread = Thread::get_current_thread();
  OPEN_ITERATE_CURRENT_AND_UPSTREAM(_cycler, current_thread) {
    CDStageWriter cdata(_cycler, pipeline_stage, current_thread);
    cdata->modify_faces()->clear();
  }
  CLOSE_ITERATE_CURRENT_AND_UPSTREAM(_cycler);

  mark_internal_bounds_stale();
}

void SolidNode::
compute_internal_bounds(CPT(BoundingVolume) &internal_bounds,
                        int &internal_vertices,
                        int pipeline_stage,
                        Thread *thread) const {

  CPT(FaceList) faces;
  {
    CDStageReader cdata(_cycler, pipeline_stage, thread);
    faces = cdata->get_faces();
  }

  internal_vertices = 0;

  LPoint3 mins(9999999);
  LPoint3 maxs(-9999999);

  size_t fcount = faces->size();
  for (size_t i = 0; i < fcount; i++) {
    Face *face = (*faces)[i];
    size_t vcount = face->get_num_vertices();
    internal_vertices += vcount;
    for (size_t j = 0; j < vcount; j++) {
      const Vertex &vert = face->get_vertex(j);
      if (vert.pos[0] < mins[0]) {
        mins[0] = vert.pos[0];
      }
      if (vert.pos[0] > maxs[0]) {
        maxs[0] = vert.pos[0];
      }
      if (vert.pos[1] < mins[1]) {
        mins[1] = vert.pos[1];
      }
      if (vert.pos[1] > maxs[1]) {
        maxs[1] = vert.pos[1];
      }
      if (vert.pos[2] < mins[2]) {
        mins[2] = vert.pos[2];
      }
      if (vert.pos[2] > maxs[2]) {
        maxs[2] = vert.pos[2];
      }
    }
  }

  internal_bounds = new BoundingBox(mins, maxs);
}

CPT(TransformState) SolidNode::
calc_tight_bounds(LPoint3 &min_point, LPoint3 &max_point, bool &found_any,
                  const TransformState *transform, Thread *current_thread) const {
  CPT(TransformState) next_transform =
    PandaNode::calc_tight_bounds(min_point, max_point, found_any, transform, current_thread);

  const LMatrix4 &mat = next_transform->get_mat();

  CPT(FaceList) faces;
  {
    CDReader cdata(_cycler, current_thread);
    faces = cdata->get_faces();
  }

  size_t fcount = faces->size();
  if (fcount == 0) {
    found_any = false;
    return next_transform;
  } else {
    found_any = true;
  }

  LPoint3 mins(9999999);
  LPoint3 maxs(-9999999);

  for (size_t i = 0; i < fcount; i++) {
    Face *face = (*faces)[i];
    size_t vcount = face->get_num_vertices();
    for (size_t j = 0; j < vcount; j++) {
      const Vertex &vert = face->get_vertex(j);
      const LPoint3 pos = mat.xform_point_general(vert.pos);
      if (pos[0] < mins[0]) {
        mins[0] = pos[0];
      }
      if (pos[0] > maxs[0]) {
        maxs[0] = pos[0];
      }
      if (pos[1] < mins[1]) {
        mins[1] = pos[1];
      }
      if (pos[1] > maxs[1]) {
        maxs[1] = pos[1];
      }
      if (pos[2] < mins[2]) {
        mins[2] = pos[2];
      }
      if (pos[2] > maxs[2]) {
        maxs[2] = pos[2];
      }
    }
  }

  min_point = mins;
  max_point = maxs;

  return next_transform;
}

bool SolidNode::
is_renderable() const {
  return true;
}

void SolidNode::
add_for_draw(CullTraverser *trav, CullTraverserData &data) {
  if (!_draw_3d && !_draw_3d_lines && !_draw_2d) {
    return;
  }

  DynamicCullTraverser *dtrav = DCAST(DynamicCullTraverser, trav);
  DynamicRender *render = dtrav->get_render();

  BitMask32 mask = dtrav->get_camera_mask();

  if ((mask & _3d_mask) != 0) {
    if (_draw_3d) {
      draw_3d(dtrav, data);
    }
    if (_draw_3d_lines) {
      draw_3d_lines(dtrav, data);
    }

  } else if(((mask & _2d_mask) != 0) && _draw_2d) {
    draw_2d(dtrav, data);
  }
}

void SolidNode::
draw_3d(DynamicCullTraverser *dtrav, CullTraverserData &data) {
  DynamicRender *render = dtrav->get_render();

  CPT(FaceList) faces;
  {
    CDReader cdata(_cycler, dtrav->get_current_thread());
    faces = cdata->get_faces();
  }
  size_t face_count = faces->size();

  for (size_t i = 0; i < face_count; i++) {
    Face *face = (*faces)[i];
    size_t vert_count = face->get_num_vertices();

    DynamicRender::Mesh *mesh = render->get_mesh(get_format_3d(),
      face->get_state_3d(), DynamicRender::PT_triangles);
    if (!mesh->lock(vert_count)) {
      continue;
    }

    GeomVertexWriter vwriter;
    GeomVertexWriter nwriter;
    GeomVertexWriter twriter;
    GeomVertexWriter tanwriter;
    GeomVertexWriter biwriter;
    mesh->get_writer(InternalName::get_vertex(), vwriter);
    mesh->get_writer(InternalName::get_normal(), nwriter);
    mesh->get_writer(InternalName::get_texcoord(), twriter);
    mesh->get_writer(InternalName::get_tangent(), tanwriter);
    mesh->get_writer(InternalName::get_binormal(), biwriter);

    for (size_t j = 0; j < vert_count; j++) {
      const Vertex &vert = face->get_vertex(j);

      vwriter.set_data3f(vert.pos);
      nwriter.set_data3f(vert.normal);
      twriter.set_data2f(vert.texcoord);
      tanwriter.set_data3f(vert.tangent);
      biwriter.set_data3f(vert.binormal);
    }

    mesh->unlock();
  }
}

void SolidNode::
draw_3d_lines(DynamicCullTraverser *dtrav, CullTraverserData &data) {
  DynamicRender *render = dtrav->get_render();

  CPT(FaceList) faces;
  {
    CDReader cdata(_cycler, dtrav->get_current_thread());
    faces = cdata->get_faces();
  }
  size_t face_count = faces->size();

  for (size_t i = 0; i < face_count; i++) {
    Face *face = (*faces)[i];
    size_t vert_count = face->get_num_vertices();

    DynamicRender::Mesh *mesh = render->get_mesh(get_format_lines(),
      face->get_state_3d_lines(), DynamicRender::PT_line_strips);
    if (!mesh->lock(vert_count)) {
      continue;
    }

    GeomVertexWriter vwriter;
    mesh->get_writer(InternalName::get_vertex(), vwriter);

    for (size_t j = 0; j < vert_count; j++) {
      const Vertex &vert = face->get_vertex(j);
      vwriter.set_data3f(vert.pos);
    }

    mesh->unlock();
  }
}

void SolidNode::
draw_2d(DynamicCullTraverser *dtrav, CullTraverserData &data) {
  DynamicRender *render = dtrav->get_render();

  CPT(FaceList) faces;
  {
    CDReader cdata(_cycler, dtrav->get_current_thread());
    faces = cdata->get_faces();
  }
  size_t face_count = faces->size();

  for (size_t i = 0; i < face_count; i++) {
    Face *face = (*faces)[i];
    size_t vert_count = face->get_num_vertices();

    DynamicRender::Mesh *mesh = render->get_mesh(get_format_lines(),
      face->get_state_2d(), DynamicRender::PT_line_strips);
    if (!mesh->lock(vert_count)) {
      continue;
    }

    GeomVertexWriter vwriter;
    mesh->get_writer(InternalName::get_vertex(), vwriter);

    for (size_t j = 0; j < vert_count; j++) {
      const Vertex &vert = face->get_vertex(j);
      vwriter.set_data3f(vert.pos);
    }

    mesh->unlock();
  }
}

CycleData *SolidNode::CData::
make_copy() const {
  return new CData(*this);
}
