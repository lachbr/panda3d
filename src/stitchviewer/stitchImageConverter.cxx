// Filename: stitchImageConverter.cxx
// Created by:  drose (06Nov99)
// 
////////////////////////////////////////////////////////////////////

#include "stitchImageConverter.h"
#include "stitchImage.h"
#include "triangleMesh.h"

#include <geomTristrip.h>
#include <geomNode.h>
#include <texture.h>
#include <textureTransition.h>
#include <chancfg.h>
#include <camera.h>
#include <perspectiveProjection.h>
#include <frustum.h>

StitchImageConverter::
StitchImageConverter() {
  _output_image = NULL;
}

void StitchImageConverter::
add_output_image(StitchImage *image) {
  _output_image = image;
}

void StitchImageConverter::
override_chan_cfg(ChanCfgOverrides &override) {
  override.setField(ChanCfgOverrides::Mask,
		    ((unsigned int)(W_DOUBLE|W_DEPTH|W_MULTISAMPLE)));
  override.setField(ChanCfgOverrides::Title, "Stitch");

  LVecBase2d size = _output_image->get_size_pixels();
  
  override.setField(ChanCfgOverrides::SizeX, (int)size[0]);
  override.setField(ChanCfgOverrides::SizeY, (int)size[1]);
}

void StitchImageConverter::
setup_camera(const RenderRelation &camera_arc) {
  PT(Camera) cam = DCAST(Camera, camera_arc.get_child());

  Frustumf frust;
  frust._t = 0.5;
  frust._b = -0.5;
  frust._l = -0.5;
  frust._r = 0.5;
  frust._fnear = 0.5;
  frust._ffar = 2.0;

  PerspectiveProjection proj(frust);
  cam->set_projection(proj);
}

bool StitchImageConverter::
is_interactive() const {
  //return false;
  return true;
}

void StitchImageConverter::
create_image_geometry(Image &im) {
  assert(_output_image != NULL);

  double dist = 1.0 + (double)im._index / (double)_images.size();
#if 0
  int x_verts = _output_image->get_x_verts();
  int y_verts = _output_image->get_y_verts();
  TriangleMesh mesh(x_verts, y_verts);

  for (int xi = 0; xi < x_verts; xi++) {
    for (int yi = 0; yi < y_verts; yi++) {
      LVector2d uvd =
	im._image->project(_output_image->get_grid_vector(xi, yi));
      LVector2f uvf(uvd);

      LVector3f p = LVector3f::rfu(2 * (double)xi / (double)(x_verts - 1) - 1,
				   1.0,
				   1 - 2 * (double)yi / (double)(y_verts - 1));
      mesh._coords.push_back(p);
      mesh._texcoords.push_back(uvf);
    }
  }
#else
  int x_verts = im._image->get_x_verts();
  int y_verts = im._image->get_y_verts();
  TriangleMesh mesh(x_verts, y_verts);

  for (int xi = 0; xi < x_verts; xi++) {
    for (int yi = 0; yi < y_verts; yi++) {
      LVector2d uvd = 
	_output_image->project(im._image->get_grid_vector(xi, yi));

      LVector3f p = LVector3f::rfu(2 * uvd[0] - 1,
				   1.0,
				   2 * uvd[1] - 1);
      LPoint2f uvf((double)xi / (double)(x_verts - 1),
		   1.0 - (double)yi / (double)(y_verts - 1));
      mesh._coords.push_back(p);
      mesh._texcoords.push_back(uvf);
    }
  }
#endif

  PT(GeomTristrip) geom = mesh.build_mesh();

  PT(GeomNode) node = new GeomNode;
  node->add_geom(geom.p());

  im._arc = new RenderRelation(_render, node);

  if (im._image->_data != NULL) {
    im._tex = new Texture;
    im._tex->set_name(im._image->get_filename());
    im._tex->load(*im._image->_data);
    im._arc->set_transition(new TextureTransition(im._tex));
  }
}
