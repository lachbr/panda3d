// Filename: geomParticleRenderer.h
// Created by:  charles (05Jul00)
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

#ifndef GEOMPARTICLERENDERER_H
#define GEOMPARTICLERENDERER_H

#include "baseParticleRenderer.h"
#include "baseParticle.h"
#include "colorInterpolationManager.h"
#include "pandaNode.h"
#include "pointerTo.h"
#include "pointerToArray.h"
#include "pvector.h"
#include "pStatCollector.h"

class EXPCL_PANDAPHYSICS GeomParticleRenderer : public BaseParticleRenderer {
PUBLISHED:
  GeomParticleRenderer(ParticleRendererAlphaMode am = PR_ALPHA_NONE,
                       PandaNode *geom_node = (PandaNode *) NULL);
  GeomParticleRenderer(const GeomParticleRenderer& copy);
  virtual ~GeomParticleRenderer();

  INLINE void set_geom_node(PandaNode *node);
  INLINE PandaNode *get_geom_node();
  INLINE ColorInterpolationManager* get_color_interpolation_manager() const;  

  virtual BaseParticleRenderer *make_copy();

  virtual void output(ostream &out) const;
  virtual void write_linear_forces(ostream &out, int indent=0) const;
  virtual void write(ostream &out, int indent=0) const;

private:
  PT(PandaNode) _geom_node;
  PT(ColorInterpolationManager) _color_interpolation_manager;

  pvector< PT(PandaNode) > _node_vector;

  int _pool_size;

  // geomparticlerenderer takes advantage of the birth/death functions

  virtual void birth_particle(int index);
  virtual void kill_particle(int index);

  virtual void init_geoms();
  virtual void render(pvector< PT(PhysicsObject) >& po_vector,
                      int ttl_particles);

  virtual void resize_pool(int new_size);
  void kill_nodes();

  static PStatCollector _render_collector;
};

#include "geomParticleRenderer.I"

#endif // GEOMPARTICLERENDERER_H
