#include "dynamicRender.h"
#include "geomTriangles.h"
#include "geomLines.h"
#include "geomLinestrips.h"
#include "cullHandler.h"
#include "cullableObject.h"
#include "omniBoundingVolume.h"
#include "pStatCollector.h"
#include "pStatTimer.h"

static PStatCollector dynamic_batches_collector("Dynamic batches");
static PStatCollector calc_indices_collector("Cull:Dynamic batch:Calc indices");
static PStatCollector reset_collector("Cull:Dynamic batch:Reset");
static PStatCollector afd_collector("Cull:Dynamic batch:Add for draw");
static PStatCollector get_mesh_collector("Cull:Dynamic batch:Get mesh");
static PStatCollector lock_collector("Cull:Dynamic batch:Lock");

IMPLEMENT_CLASS(DynamicRender);
IMPLEMENT_CLASS(DynamicCullTraverser);

// This many bytes maximum per vertex buffer.
static constexpr int vertex_buffer_size = (1024 + 512) * 1024;

static CPT(GeomVertexArrayFormat) index_format = nullptr;
static const GeomVertexArrayFormat *get_index_format() {
  if (!index_format) {
    PT(GeomVertexArrayFormat) arr = new GeomVertexArrayFormat(InternalName::get_index(), 1, GeomEnums::NT_uint16, GeomEnums::C_index);
    index_format = GeomVertexArrayFormat::register_format(arr);
  }

  return index_format;
}

DynamicRender::
DynamicRender(const std::string &name) :
  PandaNode(name) {

  set_cull_callback();
  // Ensure we are always visited.
  set_bounds(new OmniBoundingVolume);
}

DynamicRender::VertexData *DynamicRender::
find_or_create_vertex_data(const GeomVertexFormat *format) {
  auto itr = _vertex_datas.find(format);
  if (itr != _vertex_datas.end()) {
    return itr->second;
  }

  PT(VertexData) vertex_data = new VertexData(format);
  _vertex_datas[format] = vertex_data;

  return vertex_data;
}

bool DynamicRender::
cull_callback(CullTraverser *trav, CullTraverserData &data) {

  {
    PStatTimer timer(reset_collector);

    // First, reset all of our vertex datas back to position 0
    for (auto itr = _vertex_datas.begin(); itr != _vertex_datas.end(); itr++) {
      VertexData *data = itr->second;
      data->reset();
    }
    // Then, reset our meshes
    for (auto itr = _meshes.begin(); itr != _meshes.end(); itr++) {
      Mesh *mesh = itr->second;
      mesh->reset();
    }
  }


  const TransformState *internal_transform = data.get_internal_transform(trav);

  // Use the DynamicCullTraverser so children nodes have a handle to us
  // and can write to dynamic meshes.
  DynamicCullTraverser dtrav(trav, this);
  dtrav.local_object();
  dtrav.traverse_below(data);
  dtrav.end_traverse();

  afd_collector.start();
  // Now add all touched meshes for draw
  for (auto itr = _meshes.begin(); itr != _meshes.end(); itr++) {
    Mesh *mesh = itr->second;
    if (mesh->should_render()) {
      //dynamic_batches_collector.add_level(1);
      PT(Geom) geom = new Geom(mesh->get_vertex_data());
      geom->add_primitive(mesh->indices);
      trav->get_cull_handler()->record_object(
        new CullableObject(std::move(geom), std::move(mesh->state), std::move(internal_transform)), trav);
    }
  }
  afd_collector.stop();

  return false;
}

bool DynamicRender::
safe_to_combine() const {
  return false;
}

bool DynamicRender::
safe_to_flatten() const {
  return false;
}

DynamicRender::Mesh *DynamicRender::
get_mesh(const GeomVertexFormat *format, const RenderState *state,
         DynamicRender::PrimitiveType primitive_type) {

  PStatTimer timer(get_mesh_collector);

  MeshEntry entry(primitive_type, format, state);

  auto itr = _meshes.find(entry);
  if (itr != _meshes.end()) {
    return itr->second;
  }

  PT(Mesh) mesh = new Mesh;
  mesh->vertices = find_or_create_vertex_data(format);
  mesh->index_buffer = new GeomVertexArrayData(get_index_format(), GeomEnums::UH_dynamic);
  mesh->primitive_type = primitive_type;
  mesh->state = state;
  switch(primitive_type) {
  case PT_triangles:
    mesh->indices = new GeomTriangles(GeomEnums::UH_dynamic);
    break;
  case PT_lines:
    mesh->indices = new GeomLines(GeomEnums::UH_dynamic);
    break;
  case PT_line_strips:
    mesh->indices = new GeomLinestrips(GeomEnums::UH_dynamic);
    break;
  default:
    break;
  }

  mesh->indices->set_vertices(mesh->index_buffer);

  _meshes[entry] = mesh;
  return mesh;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DynamicRender::VertexData::
VertexData(const GeomVertexFormat *fmt) {
  _position = 0;
  _locked_vertices = -1;
  _locked = false;
  _vertex_data = new GeomVertexData("dynamic-vertices", fmt, GeomEnums::UH_dynamic);
  _vertex_data->unclean_set_num_rows(vertex_buffer_size / fmt->get_array(0)->get_stride());
  _format = fmt;
}

bool DynamicRender::VertexData::
lock(int num_vertices, int &first_vertex) {
  if (_locked) {
    return false;
  }

  if ((_position + num_vertices) > _vertex_data->get_num_rows()) {
    // We would exceed the number of vertices in the table.
    return false;
  }

  first_vertex = _position;
  _locked_vertices = num_vertices;
  _locked = true;
  return true;
}

void DynamicRender::VertexData::
unlock() {
  if (!_locked) {
    return;
  }

  _position += _locked_vertices;
  _locked = false;
  _locked_vertices = -1;
}

void DynamicRender::VertexData::
get_writer(const InternalName *name, GeomVertexWriter &writer) {
  nassertv(_locked);
  writer = GeomVertexWriter(_vertex_data, name);
  writer.set_row_unsafe(_position);
}

void DynamicRender::VertexData::
reset() {
  nassertv(!_locked);
  _position = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DynamicRender::Mesh::
Mesh() {
  primitive_type = PT_triangles;
  indices = nullptr;
  vertices = nullptr;
  state = nullptr;
  _locked = false;
  _locked_vertices = -1;
  _first_vertex = -1;
  _should_render = false;
}

bool DynamicRender::Mesh::
lock(int num_vertices) {
  nassertr(!_locked, false);

  lock_collector.start();

  if (!vertices->lock(num_vertices, _first_vertex)) {
    lock_collector.stop();
    return false;
  }

  _locked = true;
  _locked_vertices = num_vertices;
  return true;
}

void DynamicRender::Mesh::
get_writer(const InternalName *name, GeomVertexWriter &writer) {
  nassertv(_locked);
  vertices->get_writer(name, writer);
}

void DynamicRender::Mesh::
unlock() {
  nassertv(_locked);

  vertices->unlock();

  calc_indices_collector.start();
  // Generate the indices based on our primitive type.
  switch (primitive_type) {
  case PT_triangles:
    generate_triangle_indices();
    break;
  case PT_lines:
    generate_line_indices();
    break;
  case PT_line_strips:
    generate_line_strip_indices();
    break;
  default:
    break;
  }
  calc_indices_collector.stop();

  _locked = false;
  _locked_vertices = -1;
  _first_vertex = -1;
  _should_render = true;

  lock_collector.stop();
}

void DynamicRender::Mesh::
reset() {
  _should_render = false;
  indices->clear_vertices();
}

void DynamicRender::Mesh::
generate_triangle_indices() {
  for (int i = 1; i < _locked_vertices - 1; i++) {
    indices->add_vertices(_first_vertex,
                          _first_vertex + i,
                          _first_vertex + (i + 1));
    indices->close_primitive();
  }
}

void DynamicRender::Mesh::
generate_line_indices() {
  for (int i = 1; i < _locked_vertices; i++) {
    indices->add_vertices(_first_vertex + (i - 1), _first_vertex + i);
    indices->close_primitive();
  }
  // Add the last line to close the loop
  indices->add_vertices(_first_vertex, _locked_vertices - 1);
  indices->close_primitive();
}

void DynamicRender::Mesh::
generate_line_strip_indices() {
  for (int i = 0; i < _locked_vertices; i++) {
    indices->add_vertex(_first_vertex + i);
  }
  // Add the first vertex again to close the loop
  indices->add_vertex(_first_vertex);
  indices->close_primitive();
}
