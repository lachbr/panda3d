#include "solid.h"
#include "cPolygon.h"
#include "editorUtils.h"
#include "planeClassification.h"
#include "cullTraverser.h"
#include "cullTraverserData.h"
#include "geomVertexFormat.h"
#include "geomVertexArrayFormat.h"
#include "geomEnums.h"
#include "internalName.h"
#include "boundingBox.h"
#include "omniBoundingVolume.h"

IMPLEMENT_CLASS(CSolid);

BitMask32 CSolid::_3d_mask = BitMask32::bit(0);
BitMask32 CSolid::_2d_mask = BitMask32::bit(1);

static CPT(GeomVertexFormat) face_format = nullptr;
static const GeomVertexFormat *get_face_format() {
  if (!face_format) {
    PT(GeomVertexArrayFormat) arr = new GeomVertexArrayFormat;
    // NOTE: this must be in the same order as the members of CSolidVertex
    arr->add_column(InternalName::get_vertex(), 3, GeomEnums::NT_stdfloat, GeomEnums::C_point);
    arr->add_column(InternalName::get_normal(), 3, GeomEnums::NT_stdfloat, GeomEnums::C_normal);
    arr->add_column(InternalName::get_tangent(), 3, GeomEnums::NT_stdfloat, GeomEnums::C_vector);
    arr->add_column(InternalName::get_binormal(), 3, GeomEnums::NT_stdfloat, GeomEnums::C_vector);
    arr->add_column(InternalName::get_texcoord(), 2, GeomEnums::NT_stdfloat, GeomEnums::C_texcoord);
    arr->add_column(InternalName::get_texcoord_name("lightmap"), 2, GeomEnums::NT_stdfloat, GeomEnums::C_texcoord);
    face_format = GeomVertexFormat::register_format(arr);
  }

  return face_format;
}

void CSolid::
add_face(PT(CSolidFace) face) {
  Thread *current_thread = Thread::get_current_thread();
  OPEN_ITERATE_CURRENT_AND_UPSTREAM(_cycler, current_thread) {
    CDStageWriter cdata(_cycler, pipeline_stage, current_thread);
    cdata->modify_faces()->push_back(face);
  }
  CLOSE_ITERATE_CURRENT_AND_UPSTREAM(_cycler);
}


void CSolid::
generate_faces() {
  // Create and fill in the vertex buffer containing the vertices of all faces.
  _vdata = new GeomVertexData("solid", get_face_format(), GeomEnums::UH_dynamic);
  submit_vertices();

  CPT(FaceList) faces = get_faces(Thread::get_current_thread());
  // Create the Geoms and GeomPrimitives on each face.
  size_t face_count = faces->size();
  for (size_t i = 0; i < face_count; i++) {
    (*faces)[i]->generate();
  }

  mark_internal_bounds_stale();

  _has_geometry = true;
}

/**
 * Copies all vertices from all faces into the GeomVertexData.
 */
void CSolid::
submit_vertices() {
  size_t count = _vertices.size();
  _vdata->unclean_set_num_rows(count);
  unsigned char *data = _vdata->modify_array(0)->modify_handle()->get_write_pointer();
  memcpy(data, _vertices.data(), sizeof(CSolidVertex) * count);
}

void CSolid::cleanup() {
  FOR_EACH_FACE(cleanup);
  _np.detach_node();
  _np.clear();
}

bool CSolid::
is_renderable() const {
  return get_faces(Thread::get_current_thread())->size() > 0;
}

void CSolid::
compute_internal_bounds(CPT(BoundingVolume) &internal_bounds,
                        int &internal_vertices,
                        int pipeline_stage,
                        Thread *thread) const {
  internal_vertices = (int)_vertices.size();

  LPoint3 mins(9999999);
  LPoint3 maxs(-9999999);
  size_t count = _vertices.size();
  for (size_t i = 0; i < count; i++) {
    const CSolidVertex &vert = _vertices[i];
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

  internal_bounds = new BoundingBox(mins, maxs);
}

CPT(TransformState) CSolid::
calc_tight_bounds(LPoint3 &min_point, LPoint3 &max_point, bool &found_any,
                  const TransformState *transform, Thread *current_thread) const {
  CPT(TransformState) next_transform =
    PandaNode::calc_tight_bounds(min_point, max_point, found_any, transform, current_thread);

  const LMatrix4 &mat = next_transform->get_mat();

  size_t count = _vertices.size();
  for (size_t i = 0; i < count; i++) {
    const CSolidVertex &vert = _vertices[i];
    LPoint3 pos = mat.xform_point_general(vert.pos);

    if (i == 0) {
      max_point = pos;
      min_point = pos;
      continue;
    }

    if (pos[0] < min_point[0]) {
      min_point[0] = pos[0];
    }
    if (pos[0] > max_point[0]) {
      max_point[0] = pos[0];
    }
    if (pos[1] < min_point[1]) {
      min_point[1] = pos[1];
    }
    if (pos[1] > max_point[1]) {
      max_point[1] = pos[1];
    }
    if (pos[2] < min_point[2]) {
      min_point[2] = pos[2];
    }
    if (pos[2] > max_point[2]) {
      max_point[2] = pos[2];
    }
  }

  return next_transform;
}

void CSolid::
add_for_draw(CullTraverser *trav, CullTraverserData &data) {
  const TransformState *net_node = data.get_net_transform(trav);
  const TransformState *cam = net_node->invert_compose(trav->get_camera_transform());

  CPT(FaceList) faces = get_faces(trav->get_current_thread());
  size_t count = faces->size();
  for (size_t i = 0; i < count; i++) {
    (*faces)[i]->add_for_draw(trav, data, cam);
  }
}

bool CSolid::split(const CPlane &plane, PT(CSolid) *front, PT(CSolid) *back) {
  *front = *back = nullptr;

  CPT(FaceList) faces = get_faces(Thread::get_current_thread());
  size_t num_faces = faces->size();

  pvector<CPlane> back_planes = { plane };
  pvector<CPlane> front_planes = { plane };
  front_planes[0].flip();

  int numfront = 0, numback = 0, spanning = 0;
  for (size_t i = 0; i < num_faces; i++) {
    CSolidFace *face = (*faces)[i];
    PlaneClassification classify = face->classify_against_plane(plane);
    if (classify != PlaneClassification::Front) {
      back_planes.push_back(face->get_world_plane());
    }
    if (classify != PlaneClassification::Back) {
      front_planes.push_back(face->get_world_plane());
    }
  }

  size_t num_back = back_planes.size();
  size_t num_front = front_planes.size();
  if (num_front == 1) {
    // All the faces are behind the plane.
    *back = this;
    return false;
  } else if (num_back == 1) {
    // All the faces are in front of the plane.
    *front = this;
    return false;
  }

  *back = CSolid::create_from_intersecting_planes(back_planes, false);
  *front = CSolid::create_from_intersecting_planes(front_planes, false);

  for (size_t i = 0; i < (*back)->get_num_faces(); i++) {
    CSolidFace *face = (*back)->get_face(i);
    face->set_face_material((*faces)[0]->get_material().clone());
    face->align_texture_to_face();
  }
  for (size_t i = 0; i < (*front)->get_num_faces(); i++) {
    CSolidFace *face = (*front)->get_face(i);
    face->set_face_material((*faces)[0]->get_material().clone());
    face->align_texture_to_face();
  }

  // Restore textures (match the planes up on each face)
  for (size_t i = 0; i < num_faces; i++) {
    CSolidFace *orig = (*faces)[i];
    CPlane world_plane = orig->get_world_plane();
    CPT(FaceList) back_faces = (*back)->get_faces(Thread::get_current_thread());
    for (size_t j = 0; j < back_faces->size(); j++) {
      CSolidFace *face = (*back_faces)[j];
      PlaneClassification classify = face->classify_against_plane(world_plane);
      if (classify != PlaneClassification::OnPlane) {
        continue;
      }
      face->set_face_material(orig->get_material().clone());
      face->align_texture_to_face();
      break;
    }
    CPT(FaceList) front_faces = (*front)->get_faces(Thread::get_current_thread());
    for (size_t j = 0; j < front_faces->size(); j++) {
      CSolidFace *face = (*front_faces)[j];
      PlaneClassification classify = face->classify_against_plane(world_plane);
      if (classify != PlaneClassification::OnPlane) {
        continue;
      }
      face->set_face_material(orig->get_material().clone());
      face->align_texture_to_face();
      break;
    }
  }

  (*back)->generate_faces();
  (*front)->generate_faces();

  return true;
}

PT(CSolid) CSolid::
create_from_intersecting_planes(const pvector<CPlane> &planes, bool generate_faces) {
  PT(CSolid) solid = new CSolid;

  size_t num_planes = planes.size();
  for (size_t i = 0; i < num_planes; i++) {
    // Split the polygon by all the other planes
    PT(CPolygon) poly = new CPolygon(planes[i]);
    for (size_t j = 0; j < num_planes; j++) {
      if (i != j) {
        poly->split(planes[j]);
      }
    }

    // The final polygon is the face
    PT(CSolidFace) face = new CSolidFace(planes[i], solid);
    size_t num_verts = poly->get_num_vertices();
    for (size_t j = 0; j < num_verts; j++) {
      // Round vertices a bit for sanity
      face->add_vertex(EditorUtils::round_vector(poly->get_vertex(j), 2));
    }
    solid->add_face(face);
  }

  return solid;
}

void CSolid::
set_3d_mask(const BitMask32 &mask) {
  _3d_mask = mask;
}

void CSolid::
set_2d_mask(const BitMask32 &mask) {
  _2d_mask = mask;
}

CycleData *CSolid::CData::
make_copy() const {
  return new CData(*this);
}
