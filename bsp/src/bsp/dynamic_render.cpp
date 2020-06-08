#include "dynamic_render.h"
#include "geomTriangles.h"
#include "geomLines.h"
#include "geomLinestrips.h"
#include "clockObject.h"
#include "geomVertexWriter.h"
#include "internalName.h"

NotifyCategoryDeclNoExport(dynamicrender)
NotifyCategoryDef(dynamicrender, "")

DynamicRender::
DynamicRender() {
  _geom = nullptr;
  _drawing = false;
  _modified = true;
  _gn = new GeomNode("dynamicGeometry");
  _np = NodePath(_gn);
}

void DynamicRender::
set_draw_mask(const BitMask32 &mask) {
  _np.hide(BitMask32::all_on());
  _np.show_through(mask);
}

void DynamicRender::
render_state(const RenderState *rs) {
  nassertv(!_drawing);
  _context.state = rs;
}

void DynamicRender::
primitive_type(DynamicRender::PrimitiveType type) {
  nassertv(!_drawing);
  _context.prim_type = type;
}

void DynamicRender::
vertex_format(const GeomVertexFormat *format) {
  nassertv(!_drawing);
  _context.format = format;
}

void DynamicRender::
color(const LColor &color) {
  nassertv(!_drawing);
  _context.draw_color = color;
}

void DynamicRender::
begin() {
  nassertv(!_drawing);

  _modified = true;

  _context.calc_hash();
  _geom = find_or_create_geom_entry();

  _drawing = true;
}

void DynamicRender::
draw() {
  nassertv(_drawing);
  _gn->add_geom(_geom->geom, _geom->state);
  _drawing = false;
  _context.set_default();
}

DynamicRender::DynamicGeomEntry *DynamicRender::
find_or_create_geom_entry() {
  auto itr = _geoms.find(_context.hash);
  if (itr != _geoms.end()) {
    return itr->second;
  }

  // Haven't encountered this render context yet
  PT(DynamicGeomEntry) entry = new DynamicGeomEntry;
  entry->state = _context.state;
  entry->indices = create_indices();
  entry->vertices = find_or_create_vertex_buffer();
  entry->geom = new Geom(entry->vertices->get_vdata());
  entry->geom->add_primitive(entry->indices);
  _geoms[_context.hash] = entry;
  return entry;
}

DynamicVertexBuffer *DynamicRender::
find_or_create_vertex_buffer(int max_vertices) {
  auto itr = _vertex_buffers.find(_context.format);
  if (itr != _vertex_buffers.end()) {
    return itr->second;
  }

  PT(DynamicVertexBuffer) buffer = new DynamicVertexBuffer(_context.format, max_vertices);
  _vertex_buffers[_context.format] = buffer;
  return buffer;
}

PT(GeomPrimitive) DynamicRender::
create_indices(int max_indices) {
  PT(GeomPrimitive) prim;
  switch (_context.prim_type) {
  case PT_triangles:
    prim = new GeomTriangles(GeomEnums::UH_dynamic);
    break;
  case PT_lines:
    prim = new GeomLines(GeomEnums::UH_dynamic);
    break;
  case PT_line_strip:
    prim = new GeomLinestrips(GeomEnums::UH_dynamic);
    break;
  default:
    prim = nullptr;
  }

  if (prim) {
    prim->reserve_num_vertices(max_indices);
  }

  return prim;
}

void DynamicRender::
reset() {
  if (dynamicrender_cat.is_debug())  {
    dynamicrender_cat.debug()
      << "We have " << _geoms.size() << " geoms, " << _vertex_buffers.size() << " vertex buffers\nVertex Buffers:";
    for (auto itr = _vertex_buffers.begin(); itr != _vertex_buffers.end(); itr++) {
      std::cout << "----------------------------------" << std::endl;
      std::cout << "Position: " << itr->second->get_position() << std::endl;
      std::cout << "Max verts: " << itr->second->get_max_vertices() << std::endl;
      itr->second->get_vdata()->output(std::cout);
    }
    dynamicrender_cat.debug()
      << "Geoms:\n";
    for (auto itr = _geoms.begin(); itr != _geoms.end(); itr++) {
      std::cout << "----------------------------------" << std::endl;
      itr->second->indices->output(std::cout);
      itr->second->state->output(std::cout);
    }
  }
  
  if (_modified) {
    if (dynamicrender_cat.is_debug()) {
      dynamicrender_cat.debug()
        << "Resetting... had " << _gn->get_num_geoms() << " geoms\n";
    }
    _gn->remove_all_geoms();
    for (auto itr = _vertex_buffers.begin(); itr != _vertex_buffers.end(); itr++) {
      itr->second->reset();
    }
    for (auto itr = _geoms.begin(); itr != _geoms.end(); itr++) {
      itr->second->indices->clear_vertices();
    }

    _modified = false;
  }
}

// Functions for drawing primitives

void DynamicRender::
draw_rect(const LVector3 &mins, const LVector3 &maxs) {
  primitive_type(PT_line_strip);
  vertex_format(GeomVertexFormat::get_v3c4());

  begin();

  int first_index;
  _geom->vertices->lock(4, first_index);

  GeomVertexWriter vwriter = _geom->vertices->get_writer(InternalName::get_vertex());
  GeomVertexWriter cwriter = _geom->vertices->get_writer(InternalName::get_color());
  vwriter.set_data3f(mins[0], 0, mins[2]);
  cwriter.set_data4f(_context.draw_color);
  vwriter.set_data3f(mins[0], 0, maxs[2]);
  cwriter.set_data4f(_context.draw_color);
  vwriter.set_data3f(maxs[0], 0, maxs[2]);
  cwriter.set_data4f(_context.draw_color);
  vwriter.set_data3f(maxs[0], 0, mins[2]);
  cwriter.set_data4f(_context.draw_color);

  _geom->vertices->unlock();

  _geom->indices->add_consecutive_vertices(first_index, 4);
  _geom->indices->add_vertex(first_index);
  _geom->indices->close_primitive();

  draw();
}
