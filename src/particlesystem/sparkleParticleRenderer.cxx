// Filename: sparkleParticleRenderer.cxx
// Created by:  charles (27Jun00)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001 - 2004, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://etc.cmu.edu/panda3d/docs/license/ .
//
// To contact the maintainers of this program write to
// panda3d-general@lists.sourceforge.net .
//
////////////////////////////////////////////////////////////////////

#include "sparkleParticleRenderer.h"
#include "boundingSphere.h"
#include "geomNode.h"
#include "qpgeom.h"
#include "qpgeomVertexWriter.h"
#include "geomLine.h"

////////////////////////////////////////////////////////////////////
//    Function : SparkleParticleRenderer
//      Access : Public
// Description : Default Constructor
////////////////////////////////////////////////////////////////////
SparkleParticleRenderer::
SparkleParticleRenderer() :
  BaseParticleRenderer(PR_ALPHA_NONE),
  _center_color(Colorf(1.0f, 1.0f, 1.0f, 1.0f)),
  _edge_color(Colorf(1.0f, 1.0f, 1.0f, 1.0f)),
  _birth_radius(0.1f), _death_radius(0.1f)
{
  resize_pool(0);
}

////////////////////////////////////////////////////////////////////
//    Function : SparkleParticleRenderer
//      Access : Public
// Description : Constructor
////////////////////////////////////////////////////////////////////
SparkleParticleRenderer::
SparkleParticleRenderer(const Colorf& center, const Colorf& edge,
                        float birth_radius, float death_radius,
                        SparkleParticleLifeScale life_scale,
                        ParticleRendererAlphaMode alpha_mode) :
  BaseParticleRenderer(alpha_mode),
  _center_color(center), _edge_color(edge), _birth_radius(birth_radius),
  _death_radius(death_radius), _life_scale(life_scale)
{
  resize_pool(0);
}

////////////////////////////////////////////////////////////////////
//    Function : SparkleParticleRenderer
//      Access : Public
// Description : Copy Constructor
////////////////////////////////////////////////////////////////////
SparkleParticleRenderer::
SparkleParticleRenderer(const SparkleParticleRenderer& copy) :
  BaseParticleRenderer(copy) {
  _center_color = copy._center_color;
  _edge_color = copy._edge_color;
  _birth_radius = copy._birth_radius;
  _death_radius = copy._death_radius;
  _life_scale = copy._life_scale;

  resize_pool(0);
}

////////////////////////////////////////////////////////////////////
//    Function : ~SparkleParticleRenderer
//      Access : Public
// Description : Destructor
////////////////////////////////////////////////////////////////////
SparkleParticleRenderer::
~SparkleParticleRenderer() {
}

////////////////////////////////////////////////////////////////////
//    Function : make copy
//      Access : Public
// Description : child virtual for spawning systems
////////////////////////////////////////////////////////////////////
BaseParticleRenderer *SparkleParticleRenderer::
make_copy() {
  return new SparkleParticleRenderer(*this);
}

////////////////////////////////////////////////////////////////////
//    Function : birth_particle
//      Access : Private, virtual
// Description : child birth
////////////////////////////////////////////////////////////////////
void SparkleParticleRenderer::
birth_particle(int) {
}

////////////////////////////////////////////////////////////////////
//    Function : kill_particle
//      Access : Private, virtual
// Description : child kill
////////////////////////////////////////////////////////////////////
void SparkleParticleRenderer::
kill_particle(int) {
}

////////////////////////////////////////////////////////////////////
//    Function : resize_pool
//      Access : private
// Description : resizes the render pool.  Reference counting
//               makes this easy.
////////////////////////////////////////////////////////////////////
void SparkleParticleRenderer::
resize_pool(int new_size) {
  if (!use_qpgeom) {
    _vertex_array = PTA_Vertexf::empty_array(new_size * 12);
    _color_array = PTA_Colorf::empty_array(new_size * 12);
  }

  _max_pool_size = new_size;

  init_geoms();
}

////////////////////////////////////////////////////////////////////
//    Function : init_geoms
//      Access : private
// Description : initializes the geomnodes
////////////////////////////////////////////////////////////////////
void SparkleParticleRenderer::
init_geoms() {
  if (use_qpgeom) {
    PT(qpGeom) qpgeom = new qpGeom; 
    _line_primitive = qpgeom;
    _vdata = new qpGeomVertexData
      ("particles", qpGeomVertexFormat::get_v3cp(),
       qpGeom::UH_dynamic);
    qpgeom->set_vertex_data(_vdata);
    _lines = new qpGeomLines(qpGeom::UH_dynamic);
    qpgeom->add_primitive(_lines);

  } else {
    _line_primitive = new GeomLine;
    _line_primitive->set_coords(_vertex_array);
    _line_primitive->set_colors(_color_array, G_PER_VERTEX);
  }

  GeomNode *render_node = get_render_node();
  render_node->remove_all_geoms();
  render_node->add_geom(_line_primitive, _render_state);
}

////////////////////////////////////////////////////////////////////
//    Function : render
//      Access : private
// Description : populates the GeomLine
////////////////////////////////////////////////////////////////////
void SparkleParticleRenderer::
render(pvector< PT(PhysicsObject) >& po_vector, int ttl_particles) {

  if (!ttl_particles)
    return;

  BaseParticle *cur_particle;

  int remaining_particles = ttl_particles;
  int i;

  Vertexf *cur_vert = &_vertex_array[0];
  Colorf *cur_color = &_color_array[0];
  qpGeomVertexWriter vertex(_vdata, InternalName::get_vertex());
  qpGeomVertexWriter color(_vdata, InternalName::get_color());
  if (use_qpgeom) {
    _lines->clear_vertices();
  }

  // init the aabb

  _aabb_min.set(99999.0f, 99999.0f, 99999.0f);
  _aabb_max.set(-99999.0f, -99999.0f, -99999.0f);

  // run through the array

  for (i = 0; i < (int)po_vector.size(); i++) {
    cur_particle = (BaseParticle *) po_vector[i].p();

    if (cur_particle->get_alive() == false)
      continue;

    LPoint3f position = cur_particle->get_position();

    // adjust the aabb

    if (position[0] > _aabb_max[0])
      _aabb_max[0] = position[0];
    else if (position[0] < _aabb_min[0])
      _aabb_min[0] = position[0];

    if (position[1] > _aabb_max[1])
      _aabb_max[1] = position[1];
    else if (position[1] < _aabb_min[1])
      _aabb_min[1] = position[1];

    if (position[2] > _aabb_max[2])
      _aabb_max[2] = position[2];
    else if (position[2] < _aabb_min[2])
      _aabb_min[2] = position[2];

    // draw the particle.

    float radius = get_radius(cur_particle);
    float neg_radius = -radius;
    float alpha;

    Colorf center_color = _center_color;
    Colorf edge_color = _edge_color;

    // handle alpha

    if (_alpha_mode != PR_ALPHA_NONE) {
      if(_alpha_mode == PR_ALPHA_USER) {
        alpha = get_user_alpha();
      } else {
        alpha = cur_particle->get_parameterized_age();
        if (_alpha_mode == PR_ALPHA_OUT)
          alpha = 1.0f - alpha;
        else if (_alpha_mode == PR_ALPHA_IN_OUT)
          alpha = 2.0f * min(alpha, 1.0f - alpha);

        alpha *= get_user_alpha();
      }

      center_color[3] = alpha;
      edge_color[3] = alpha;
    }

    // 6 lines coming from the center point.

    if (use_qpgeom) {
      vertex.add_data3f(position);
      vertex.add_data3f(position + Vertexf(radius, 0.0f, 0.0f));
      vertex.add_data3f(position);
      vertex.add_data3f(position + Vertexf(neg_radius, 0.0f, 0.0f));
      vertex.add_data3f(position);
      vertex.add_data3f(position + Vertexf(0.0f, radius, 0.0f));
      vertex.add_data3f(position);
      vertex.add_data3f(position + Vertexf(0.0f, neg_radius, 0.0f));
      vertex.add_data3f(position);
      vertex.add_data3f(position + Vertexf(0.0f, 0.0f, radius));
      vertex.add_data3f(position);
      vertex.add_data3f(position + Vertexf(0.0f, 0.0f, neg_radius));

      color.add_data4f(center_color);
      color.add_data4f(edge_color);
      color.add_data4f(center_color);
      color.add_data4f(edge_color);
      color.add_data4f(center_color);
      color.add_data4f(edge_color);
      color.add_data4f(center_color);
      color.add_data4f(edge_color);
      color.add_data4f(center_color);
      color.add_data4f(edge_color);
      color.add_data4f(center_color);
      color.add_data4f(edge_color);

      _lines->add_next_vertices(2);
      _lines->close_primitive();
      _lines->add_next_vertices(2);
      _lines->close_primitive();
      _lines->add_next_vertices(2);
      _lines->close_primitive();
      _lines->add_next_vertices(2);
      _lines->close_primitive();
      _lines->add_next_vertices(2);
      _lines->close_primitive();
      _lines->add_next_vertices(2);
      _lines->close_primitive();
    } else {
      *cur_vert++ = position;
      *cur_vert++ = position + Vertexf(radius, 0.0f, 0.0f);
      *cur_vert++ = position;
      *cur_vert++ = position + Vertexf(neg_radius, 0.0f, 0.0f);
      *cur_vert++ = position;
      *cur_vert++ = position + Vertexf(0.0f, radius, 0.0f);
      *cur_vert++ = position;
      *cur_vert++ = position + Vertexf(0.0f, neg_radius, 0.0f);
      *cur_vert++ = position;
      *cur_vert++ = position + Vertexf(0.0f, 0.0f, radius);
      *cur_vert++ = position;
      *cur_vert++ = position + Vertexf(0.0f, 0.0f, neg_radius);

      *cur_color++ = center_color;
      *cur_color++ = edge_color;
      *cur_color++ = center_color;
      *cur_color++ = edge_color;
      *cur_color++ = center_color;
      *cur_color++ = edge_color;
      *cur_color++ = center_color;
      *cur_color++ = edge_color;
      *cur_color++ = center_color;
      *cur_color++ = edge_color;
      *cur_color++ = center_color;
      *cur_color++ = edge_color;
    }

    remaining_particles--;
    if (remaining_particles == 0)
      break;
  }

  if (!use_qpgeom) {
    _line_primitive->set_num_prims(6 * ttl_particles);
  }

  // done filling geomline node, now do the bb stuff

  LPoint3f aabb_center = _aabb_min + ((_aabb_max - _aabb_min) * 0.5f);
  float radius = (aabb_center - _aabb_min).length();

  _line_primitive->set_bound(BoundingSphere(aabb_center, radius));
  get_render_node()->mark_bound_stale();
}

////////////////////////////////////////////////////////////////////
//     Function : output
//       Access : Public
//  Description : Write a string representation of this instance to
//                <out>.
////////////////////////////////////////////////////////////////////
void SparkleParticleRenderer::
output(ostream &out) const {
  #ifndef NDEBUG //[
  out<<"SparkleParticleRenderer";
  #endif //] NDEBUG
}

////////////////////////////////////////////////////////////////////
//     Function : write
//       Access : Public
//  Description : Write a string representation of this instance to
//                <out>.
////////////////////////////////////////////////////////////////////
void SparkleParticleRenderer::
write(ostream &out, int indent) const {
  #ifndef NDEBUG //[
  out.width(indent); out<<""; out<<"SparkleParticleRenderer:\n";
  out.width(indent+2); out<<""; out<<"_center_color "<<_center_color<<"\n";
  out.width(indent+2); out<<""; out<<"_edge_color "<<_edge_color<<"\n";
  out.width(indent+2); out<<""; out<<"_birth_radius "<<_birth_radius<<"\n";
  out.width(indent+2); out<<""; out<<"_death_radius "<<_death_radius<<"\n";
  out.width(indent+2); out<<""; out<<"_line_primitive "<<_line_primitive<<"\n";
  out.width(indent+2); out<<""; out<<"_vertex_array "<<_vertex_array<<"\n";
  out.width(indent+2); out<<""; out<<"_color_array "<<_color_array<<"\n";
  out.width(indent+2); out<<""; out<<"_max_pool_size "<<_max_pool_size<<"\n";
  out.width(indent+2); out<<""; out<<"_life_scale "<<_life_scale<<"\n";
  out.width(indent+2); out<<""; out<<"_aabb_min "<<_aabb_min<<"\n";
  out.width(indent+2); out<<""; out<<"_aabb_max "<<_aabb_max<<"\n";
  BaseParticleRenderer::write(out, indent+2);
  #endif //] NDEBUG
}
