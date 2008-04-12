// Filename: glGraphicsStateGuardian_src.cxx
// Created by:  drose (02Feb99)
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

#include "config_util.h"
#include "displayRegion.h"
#include "renderBuffer.h"
#include "geom.h"
#include "geomVertexData.h"
#include "geomTriangles.h"
#include "geomTristrips.h"
#include "geomTrifans.h"
#include "geomLines.h"
#include "geomLinestrips.h"
#include "geomPoints.h"
#include "geomVertexReader.h"
#include "graphicsWindow.h"
#include "lens.h"
#include "perspectiveLens.h"
#include "directionalLight.h"
#include "pointLight.h"
#include "spotlight.h"
#include "planeNode.h"
#include "attribSlots.h"
#include "fog.h"
#include "clockObject.h"
#include "string_utils.h"
#include "nodePath.h"
#include "dcast.h"
#include "pvector.h"
#include "vector_string.h"
#include "string_utils.h"
#include "pnmImage.h"
#include "config_gobj.h"
#include "mutexHolder.h"
#include "indirectLess.h"
#include "pStatTimer.h"
#include "load_prc_file.h"

#ifdef HAVE_CG
#include "Cg/cgGL.h"
#endif

#include <algorithm>

TypeHandle CLP(GraphicsStateGuardian)::_type_handle;

PStatCollector CLP(GraphicsStateGuardian)::_load_display_list_pcollector("Draw:Transfer data:Display lists");
PStatCollector CLP(GraphicsStateGuardian)::_primitive_batches_display_list_pcollector("Primitive batches:Display lists");
PStatCollector CLP(GraphicsStateGuardian)::_vertices_display_list_pcollector("Vertices:Display lists");
PStatCollector CLP(GraphicsStateGuardian)::_vertices_immediate_pcollector("Vertices:Immediate mode");

// The following noop functions are assigned to the corresponding
// glext function pointers in the class, in case the functions are not
// defined by the GL, just so it will always be safe to call the
// extension functions.

static void APIENTRY
null_glPointParameterfv(GLenum, const GLfloat *) {
}

static void APIENTRY
null_glDrawRangeElements(GLenum mode, GLuint start, GLuint end,
                         GLsizei count, GLenum type, const GLvoid *indices) {
  // If we don't support glDrawRangeElements(), just use the original
  // glDrawElements() instead.
  GLP(DrawElements)(mode, count, type, indices);
}

static void APIENTRY
null_glActiveTexture(GLenum gl_texture_stage) {
  // If we don't support multitexture, we'd better not try to request
  // a texture beyond the first texture stage.
  nassertv(gl_texture_stage == GL_TEXTURE0);
}

static void APIENTRY
null_glBlendEquation(GLenum) {
}

static void APIENTRY
null_glBlendColor(GLclampf, GLclampf, GLclampf, GLclampf) {
}


////////////////////////////////////////////////////////////////////
//     Function: uchar_bgr_to_rgb
//  Description: Recopies the given array of pixels, converting from
//               BGR to RGB arrangement.
////////////////////////////////////////////////////////////////////
static void
uchar_bgr_to_rgb(unsigned char *dest, const unsigned char *source,
                 int num_pixels) {
  for (int i = 0; i < num_pixels; i++) {
    dest[0] = source[2];
    dest[1] = source[1];
    dest[2] = source[0];
    dest += 3;
    source += 3;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: uchar_bgra_to_rgba
//  Description: Recopies the given array of pixels, converting from
//               BGRA to RGBA arrangement.
////////////////////////////////////////////////////////////////////
static void
uchar_bgra_to_rgba(unsigned char *dest, const unsigned char *source,
                   int num_pixels) {
  for (int i = 0; i < num_pixels; i++) {
    dest[0] = source[2];
    dest[1] = source[1];
    dest[2] = source[0];
    dest[3] = source[3];
    dest += 4;
    source += 4;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: ushort_bgr_to_rgb
//  Description: Recopies the given array of pixels, converting from
//               BGR to RGB arrangement.
////////////////////////////////////////////////////////////////////
static void
ushort_bgr_to_rgb(unsigned short *dest, const unsigned short *source,
                  int num_pixels) {
  for (int i = 0; i < num_pixels; i++) {
    dest[0] = source[2];
    dest[1] = source[1];
    dest[2] = source[0];
    dest += 3;
    source += 3;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: ushort_bgra_to_rgba
//  Description: Recopies the given array of pixels, converting from
//               BGRA to RGBA arrangement.
////////////////////////////////////////////////////////////////////
static void
ushort_bgra_to_rgba(unsigned short *dest, const unsigned short *source,
                    int num_pixels) {
  for (int i = 0; i < num_pixels; i++) {
    dest[0] = source[2];
    dest[1] = source[1];
    dest[2] = source[0];
    dest[3] = source[3];
    dest += 4;
    source += 4;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: fix_component_ordering
//  Description: Reverses the order of the components within the
//               image, to convert (for instance) GL_BGR to GL_RGB.
//               Returns the byte pointer representing the converted
//               image, or the original image if it is unchanged.
//
//               new_image must be supplied; it is the PTA_uchar that
//               will be used to hold the converted image if required.
//               It will be modified only if the conversion is
//               necessary, in which case the data will be stored
//               there, and this pointer will be returned.  If the
//               conversion is not necessary, this pointer will be
//               left unchanged.
////////////////////////////////////////////////////////////////////
static const unsigned char *
fix_component_ordering(PTA_uchar &new_image,
                       const unsigned char *orig_image, size_t orig_image_size,
                       GLenum external_format, Texture *tex) {
  const unsigned char *result = orig_image;

  switch (external_format) {
  case GL_RGB:
    switch (tex->get_component_type()) {
    case Texture::T_unsigned_byte:
      new_image = PTA_uchar::empty_array(orig_image_size);
      uchar_bgr_to_rgb(new_image, orig_image, orig_image_size / 3);
      result = new_image;
      break;

    case Texture::T_unsigned_short:
      new_image = PTA_uchar::empty_array(orig_image_size);
      ushort_bgr_to_rgb((unsigned short *)new_image.p(),
                        (const unsigned short *)orig_image,
                        orig_image_size / 6);
      result = new_image;
      break;

    default:
      break;
    }
    break;

  case GL_RGBA:
    switch (tex->get_component_type()) {
    case Texture::T_unsigned_byte:
      new_image = PTA_uchar::empty_array(orig_image_size);
      uchar_bgra_to_rgba(new_image, orig_image, orig_image_size / 4);
      result = new_image;
      break;

    case Texture::T_unsigned_short:
      new_image = PTA_uchar::empty_array(orig_image_size);
      ushort_bgra_to_rgba((unsigned short *)new_image.p(),
                          (const unsigned short *)orig_image,
                          orig_image_size / 8);
      result = new_image;
      break;

    default:
      break;
    }
    break;

  default:
    break;
  }

  return result;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
CLP(GraphicsStateGuardian)::
CLP(GraphicsStateGuardian)(GraphicsPipe *pipe) :
  GraphicsStateGuardian(CS_yup_right, pipe)
{
  _error_count = 0;

  // Hack.  Turn on the flag that we turned off at a higher level,
  // since we know this works properly in OpenGL, and we want the
  // performance benefit it gives us.
  _prepared_objects->_support_released_buffer_cache = true;

#ifdef DO_PSTATS
  if (CLP(finish)) {
    GLCAT.warning()
      << "The config variable gl-finish is set True.  This may have a substantial negative impact your render performance.\n";
  }
#endif  // DO_PSTATS
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::Destructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
CLP(GraphicsStateGuardian)::
~CLP(GraphicsStateGuardian)() {
  if (GLCAT.is_debug()) {
    GLCAT.debug()
      << "GLGraphicsStateGuardian " << this << " destructing\n";
  }

  close_gsg();

  if (_stencil_render_states) {
    delete _stencil_render_states;
    _stencil_render_states = 0;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::reset
//       Access: Public, Virtual
//  Description: Resets all internal state as if the gsg were newly
//               created.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
reset() {
  free_pointers();
  GraphicsStateGuardian::reset();

  // Output the vendor and version strings.
  query_gl_version();

  // Save the extensions tokens.
  save_extensions((const char *)GLP(GetString)(GL_EXTENSIONS));
  get_extra_extensions();
  report_extensions();

  _supported_geom_rendering =
    Geom::GR_indexed_point |
    Geom::GR_point | Geom::GR_point_uniform_size |
    Geom::GR_indexed_other |
    Geom::GR_triangle_strip | Geom::GR_triangle_fan |
    Geom::GR_flat_last_vertex;

  _supports_point_parameters = false;

  if (is_at_least_version(1, 4)) {
    _supports_point_parameters = true;
#ifdef EXPECT_GL_VERSION_1_4
    GLCAT.debug()
      << "Getting compile-time PointParameter pointers\n";
    _glPointParameterfv = (PFNGLPOINTPARAMETERFVPROC)&GLP(PointParameterfv);
#else  // EXPECT_GL_VERSION_1_4
    GLCAT.debug()
      << "Getting run-time PointParameter pointers\n";
    _glPointParameterfv = (PFNGLPOINTPARAMETERFVPROC)
      get_extension_func(GLPREFIX_QUOTED, "PointParameterfv");
#endif  // EXPECT_GL_VERSION_1_4

  } else if (has_extension("GL_ARB_point_parameters")) {
    _supports_point_parameters = true;
    _glPointParameterfv = (PFNGLPOINTPARAMETERFVPROC)
      get_extension_func(GLPREFIX_QUOTED, "PointParameterfvARB");
  }
  if (_supports_point_parameters) {
    if (_glPointParameterfv == NULL) {
      GLCAT.warning()
        << "glPointParameterfv advertised as supported by OpenGL runtime, but could not get pointers to extension functions.\n";
      _supports_point_parameters = false;
    }
  }
  if (_supports_point_parameters) {
    _supported_geom_rendering |= Geom::GR_point_perspective;
  } else {
    _glPointParameterfv = null_glPointParameterfv;
  }

  _supports_point_sprite = has_extension("GL_ARB_point_sprite");
  if (_supports_point_sprite) {
    // It appears that the point_sprite extension doesn't support
    // texture transforms on the generated texture coordinates.  How
    // inconsistent.  Because of this, we don't advertise
    // GR_point_sprite_tex_matrix.
    _supported_geom_rendering |= Geom::GR_point_sprite;
  }

  _supports_vertex_blend = has_extension("GL_ARB_vertex_blend");

  if (_supports_vertex_blend) {
    _glWeightPointerARB = (PFNGLWEIGHTPOINTERARBPROC)
      get_extension_func(GLPREFIX_QUOTED, "WeightPointerARB");
    _glVertexBlendARB = (PFNGLVERTEXBLENDARBPROC)
      get_extension_func(GLPREFIX_QUOTED, "VertexBlendARB");
    _glWeightfvARB = (PFNGLWEIGHTFVARBPROC)
      get_extension_func(GLPREFIX_QUOTED, "WeightfvARB");

    if (_glWeightPointerARB == NULL || _glVertexBlendARB == NULL ||
        _glWeightfvARB == NULL) {
      GLCAT.warning()
        << "Vertex blending advertised as supported by OpenGL runtime, but could not get pointers to extension functions.\n";
      _supports_vertex_blend = false;
    }
  }

  if (_supports_vertex_blend) {
    GLP(Enable)(GL_WEIGHT_SUM_UNITY_ARB);

    GLint max_vertex_units = 0;
    GLP(GetIntegerv)(GL_MAX_VERTEX_UNITS_ARB, &max_vertex_units);
    _max_vertex_transforms = max_vertex_units;
    if (GLCAT.is_debug()) {
      GLCAT.debug()
        << "max vertex transforms = " << _max_vertex_transforms << "\n";
    }
  }

  _supports_matrix_palette = has_extension("GL_ARB_matrix_palette");

  if (_supports_matrix_palette) {
    _glCurrentPaletteMatrixARB = (PFNGLCURRENTPALETTEMATRIXARBPROC)
      get_extension_func(GLPREFIX_QUOTED, "CurrentPaletteMatrixARB");
    _glMatrixIndexPointerARB = (PFNGLMATRIXINDEXPOINTERARBPROC)
      get_extension_func(GLPREFIX_QUOTED, "MatrixIndexPointerARB");
    _glMatrixIndexuivARB = (PFNGLMATRIXINDEXUIVARBPROC)
      get_extension_func(GLPREFIX_QUOTED, "MatrixIndexuivARB");

    if (_glCurrentPaletteMatrixARB == NULL ||
        _glMatrixIndexPointerARB == NULL ||
        _glMatrixIndexuivARB == NULL) {
      GLCAT.warning()
        << "Matrix palette advertised as supported by OpenGL runtime, but could not get pointers to extension functions.\n";
      _supports_matrix_palette = false;
    }
  }

  /*
    The matrix_palette support in this module is completely untested
    (because I don't happen to have a card handy whose driver supports
    this extension), so I have this ConfigVariable set to
    unconditionally set this flag off for now, to protect the unwary.
    When we have shown that the code works, we should remove this bit.
    In the meantime, you must put both "matrix-palette 1" and
    "gl-matrix-palette 1" in your Config.prc to exercise the new
    code. */
  if (!ConfigVariableBool("gl-matrix-palette", false, PRC_DESC("Temporary hack variable protecting untested code.  See glGraphicsStateGuardian_src.cxx."))) {
    if (_supports_matrix_palette) {
      if (GLCAT.is_debug()) {
        GLCAT.debug() << "Forcing off matrix palette support.\n";
      }
    }
    _supports_matrix_palette = false;
  }

  if (_supports_matrix_palette) {
    GLint max_palette_matrices = 0;
    GLP(GetIntegerv)(GL_MAX_PALETTE_MATRICES_ARB, &max_palette_matrices);
    _max_vertex_transform_indices = max_palette_matrices;
    if (GLCAT.is_debug()) {
      GLCAT.debug()
        << "max vertex transform indices = " << _max_vertex_transform_indices << "\n";
    }
  }

  _supports_draw_range_elements = false;

  if (is_at_least_version(1, 2)) {
    _supports_draw_range_elements = true;
#ifdef EXPECT_GL_VERSION_1_2
    GLCAT.debug()
      << "Getting compile-time DrawRangeElements pointers\n";
    _glDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)&GLP(DrawRangeElements);
#else  // EXPECT_GL_VERSION_1_2
    GLCAT.debug()
      << "Getting run-time DrawRangeElements pointers\n";
    _glDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)
      get_extension_func(GLPREFIX_QUOTED, "DrawRangeElements");
#endif  // EXPECT_GL_VERSION_1_2

  } else if (has_extension("GL_EXT_draw_range_elements")) {
    _supports_draw_range_elements = true;
    _glDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)
      get_extension_func(GLPREFIX_QUOTED, "DrawRangeElementsEXT");
  }
  if (_supports_draw_range_elements) {
    if (_glDrawRangeElements == NULL) {
      GLCAT.warning()
        << "glDrawRangeElements advertised as supported by OpenGL runtime, but could not get pointers to extension functions.\n";
      _supports_draw_range_elements = false;
    }
  }
  if (!_supports_draw_range_elements) {
    _glDrawRangeElements = null_glDrawRangeElements;
  }

  _supports_depth_texture =
    has_extension("GL_ARB_depth_texture") || is_at_least_version(1, 4);

  _supports_depth_stencil = false;
  if (_supports_depth_texture) {
    _supports_depth_stencil =
      has_extension("GL_EXT_packed_depth_stencil");
  }
  
  _supports_3d_texture = false;

  if (is_at_least_version(1, 2)) {
    _supports_3d_texture = true;

#ifdef EXPECT_GL_VERSION_1_2
    GLCAT.debug()
      << "Getting compile-time 3-D textures pointers\n";
    _glTexImage3D = (PFNGLTEXIMAGE3DPROC)&GLP(TexImage3D);
    _glTexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC)&GLP(TexSubImage3D);
#else  // EXPECT_GL_VERSION_1_2
    GLCAT.debug()
      << "Getting run-time 3-D textures pointers\n";
    _glTexImage3D = (PFNGLTEXIMAGE3DPROC)
      get_extension_func(GLPREFIX_QUOTED, "TexImage3D");
    _glTexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC)
      get_extension_func(GLPREFIX_QUOTED, "TexSubImage3D");
#endif  // EXPECT_GL_VERSION_1_2

  } else if (has_extension("GL_EXT_texture3D")) {
    _supports_3d_texture = true;

    _glTexImage3D = (PFNGLTEXIMAGE3DPROC)
      get_extension_func(GLPREFIX_QUOTED, "TexImage3DEXT");
    _glTexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC)
      get_extension_func(GLPREFIX_QUOTED, "TexSubImage3DEXT");
  }

  if (_supports_3d_texture) {
    if (_glTexImage3D == NULL || _glTexSubImage3D == NULL) {
      GLCAT.warning()
        << "3-D textures advertised as supported by OpenGL runtime, but could not get pointers to extension functions.\n";
      _supports_3d_texture = false;
    }
  }

  _supports_cube_map =
    has_extension("GL_ARB_texture_cube_map") || is_at_least_version(1, 3);

  _supports_compressed_texture = false;
  if (is_at_least_version(1, 3)) {
    _supports_compressed_texture = true;

#ifdef EXPECT_GL_VERSION_1_3
    GLCAT.debug()
      << "Getting compile-time compressed textures pointers\n";
    _glCompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC)&GLP(CompressedTexImage1D);
    _glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)&GLP(CompressedTexImage2D);
    _glCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)&GLP(CompressedTexImage3D);
    _glCompressedTexSubImage1D = (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)&GLP(CompressedTexSubImage1D);
    _glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)&GLP(CompressedTexSubImage2D);
    _glCompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)&GLP(CompressedTexSubImage3D);
    _glGetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC)&GLP(GetCompressedTexImage);
#else  // EXPECT_GL_VERSION_1_3
    GLCAT.debug()
      << "Getting run-time compressed textures pointers\n";
    _glCompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC)
      get_extension_func(GLPREFIX_QUOTED, "CompressedTexImage1D");
    _glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)
      get_extension_func(GLPREFIX_QUOTED, "CompressedTexImage2D");
    _glCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)
      get_extension_func(GLPREFIX_QUOTED, "CompressedTexImage3D");
    _glCompressedTexSubImage1D = (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)
      get_extension_func(GLPREFIX_QUOTED, "CompressedTexSubImage1D");
    _glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)
      get_extension_func(GLPREFIX_QUOTED, "CompressedTexSubImage2D");
    _glCompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)
      get_extension_func(GLPREFIX_QUOTED, "CompressedTexSubImage3D");
    _glGetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC)
      get_extension_func(GLPREFIX_QUOTED, "GetCompressedTexImage");
#endif  // EXPECT_GL_VERSION_1_3

  } else if (has_extension("GL_ARB_texture_compression")) {
    _supports_compressed_texture = true;

    _glCompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC)
      get_extension_func(GLPREFIX_QUOTED, "CompressedTexImage1DARB");
    _glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)
      get_extension_func(GLPREFIX_QUOTED, "CompressedTexImage2DARB");
    _glCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)
      get_extension_func(GLPREFIX_QUOTED, "CompressedTexImage3DARB");
    _glCompressedTexSubImage1D = (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)
      get_extension_func(GLPREFIX_QUOTED, "CompressedTexSubImage1DARB");
    _glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)
      get_extension_func(GLPREFIX_QUOTED, "CompressedTexSubImage2DARB");
    _glCompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)
      get_extension_func(GLPREFIX_QUOTED, "CompressedTexSubImage3DARB");
    _glGetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC)
      get_extension_func(GLPREFIX_QUOTED, "GetCompressedTexImageARB");
  }

  if (_supports_compressed_texture) {
    if (_glCompressedTexImage1D == NULL ||
        _glCompressedTexImage2D == NULL ||
        _glCompressedTexImage3D == NULL ||
        _glCompressedTexSubImage1D == NULL ||
        _glCompressedTexSubImage2D == NULL ||
        _glCompressedTexSubImage3D == NULL ||
        _glGetCompressedTexImage == NULL) {
      GLCAT.warning()
        << "Compressed textures advertised as supported by OpenGL runtime, but could not get pointers to extension functions.\n";
      _supports_compressed_texture = false;
    }
  }

  if (_supports_compressed_texture) {
    _compressed_texture_formats.set_bit(Texture::CM_on);

    GLint num_compressed_formats = 0;
    GLP(GetIntegerv)(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &num_compressed_formats);
    GLint *formats = (GLint *)PANDA_MALLOC_ARRAY(num_compressed_formats * sizeof(GLint));
    GLP(GetIntegerv)(GL_COMPRESSED_TEXTURE_FORMATS, formats);
    for (int i = 0; i < num_compressed_formats; ++i) {
      switch (formats[i]) {
      case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        _compressed_texture_formats.set_bit(Texture::CM_dxt1);
        break;

      case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        _compressed_texture_formats.set_bit(Texture::CM_dxt3);
        break;

      case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        _compressed_texture_formats.set_bit(Texture::CM_dxt5);
        break;

      case GL_COMPRESSED_RGB_FXT1_3DFX:
      case GL_COMPRESSED_RGBA_FXT1_3DFX:
        _compressed_texture_formats.set_bit(Texture::CM_fxt1);
        break;

      default:
        break;
      }
    }
    PANDA_FREE_ARRAY(formats);
  }

  _supports_bgr =
    has_extension("GL_EXT_bgra") || is_at_least_version(1, 2);

  _supports_rescale_normal =
    CLP(support_rescale_normal) &&
    (has_extension("GL_EXT_rescale_normal") || is_at_least_version(1, 2));

  _supports_multisample =
    has_extension("GL_ARB_multisample");

  _supports_generate_mipmap =
    has_extension("GL_SGIS_generate_mipmap") || is_at_least_version(1, 4);

  _supports_multitexture = false;

  _supports_mesa_6 = false;
  if (_gl_version.find("Mesa 6.",0) != string::npos) {
    _supports_mesa_6 = true;
  }

  _supports_tex_non_pow2 =
    has_extension("GL_ARB_texture_non_power_of_two");

  if (is_at_least_version(1, 3)) {
    _supports_multitexture = true;

#ifdef EXPECT_GL_VERSION_1_3
    GLCAT.debug()
      << "Getting compile-time multitexture pointers\n";
    _glActiveTexture = (PFNGLACTIVETEXTUREPROC)&GLP(ActiveTexture);
    _glClientActiveTexture = (PFNGLACTIVETEXTUREPROC)&GLP(ClientActiveTexture);
    _glMultiTexCoord1f = (PFNGLMULTITEXCOORD1FPROC)&GLP(MultiTexCoord1f);
    _glMultiTexCoord2f = (PFNGLMULTITEXCOORD2FPROC)&GLP(MultiTexCoord2f);
    _glMultiTexCoord3f = (PFNGLMULTITEXCOORD3FPROC)&GLP(MultiTexCoord3f);
    _glMultiTexCoord4f = (PFNGLMULTITEXCOORD4FPROC)&GLP(MultiTexCoord4f);

#else  // EXPECT_GL_VERSION_1_3
    GLCAT.debug() 
      << "Getting run-time multitexture pointers\n";
    _glActiveTexture = (PFNGLACTIVETEXTUREPROC)
      get_extension_func(GLPREFIX_QUOTED, "ActiveTexture");
    _glClientActiveTexture = (PFNGLACTIVETEXTUREPROC)
      get_extension_func(GLPREFIX_QUOTED, "ClientActiveTexture");
    _glMultiTexCoord1f = (PFNGLMULTITEXCOORD1FPROC)
      get_extension_func(GLPREFIX_QUOTED, "MultiTexCoord1f");
    _glMultiTexCoord2f = (PFNGLMULTITEXCOORD2FPROC)
      get_extension_func(GLPREFIX_QUOTED, "MultiTexCoord2f");
    _glMultiTexCoord3f = (PFNGLMULTITEXCOORD3FPROC)
      get_extension_func(GLPREFIX_QUOTED, "MultiTexCoord3f");
    _glMultiTexCoord4f = (PFNGLMULTITEXCOORD4FPROC)
      get_extension_func(GLPREFIX_QUOTED, "MultiTexCoord4f");
#endif  // EXPECT_GL_VERSION_1_3

  } else if (has_extension("GL_ARB_multitexture")) {
    _supports_multitexture = true;

    _glActiveTexture = (PFNGLACTIVETEXTUREPROC)
      get_extension_func(GLPREFIX_QUOTED, "ActiveTextureARB");
    _glClientActiveTexture = (PFNGLACTIVETEXTUREPROC)
      get_extension_func(GLPREFIX_QUOTED, "ClientActiveTextureARB");
    _glMultiTexCoord1f = (PFNGLMULTITEXCOORD1FPROC)
      get_extension_func(GLPREFIX_QUOTED, "MultiTexCoord1fARB");
    _glMultiTexCoord2f = (PFNGLMULTITEXCOORD2FPROC)
      get_extension_func(GLPREFIX_QUOTED, "MultiTexCoord2fARB");
    _glMultiTexCoord3f = (PFNGLMULTITEXCOORD3FPROC)
      get_extension_func(GLPREFIX_QUOTED, "MultiTexCoord3fARB");
    _glMultiTexCoord4f = (PFNGLMULTITEXCOORD4FPROC)
      get_extension_func(GLPREFIX_QUOTED, "MultiTexCoord4fARB");
  }

  if (_supports_multitexture) {
    if (_glActiveTexture == NULL || _glClientActiveTexture == NULL ||
        _glMultiTexCoord1f == NULL || _glMultiTexCoord2f == NULL ||
        _glMultiTexCoord3f == NULL || _glMultiTexCoord4f == NULL) {
      GLCAT.warning()
        << "Multitexture advertised as supported by OpenGL runtime, but could not get pointers to extension functions.\n";
      _supports_multitexture = false;
    }
  }
  if (!_supports_multitexture) {
    _glActiveTexture = null_glActiveTexture;
    _glClientActiveTexture = null_glActiveTexture;
  }

  if (has_extension("GL_ARB_depth_texture")) {
    _supports_depth_texture = true;
  }

  if (_supports_depth_texture &&
      has_extension("GL_ARB_shadow") &&
      has_extension("GL_ARB_fragment_program_shadow")) {
    _supports_shadow_filter = true;
  }
  if (_gl_vendor.substr(0,3)=="ATI") {
    // ATI drivers have never provided correct shadow support.
    _supports_shadow_filter = false;
  }
  
  _supports_texture_combine =
    has_extension("GL_ARB_texture_env_combine") || is_at_least_version(1, 3);
  _supports_texture_saved_result =
    has_extension("GL_ARB_texture_env_crossbar") || is_at_least_version(1, 4);
  _supports_texture_dot3 =
    has_extension("GL_ARB_texture_env_dot3") || is_at_least_version(1, 3);

  _supports_buffers = false;

  if (is_at_least_version(1, 5)) {
    _supports_buffers = true;

#ifdef EXPECT_GL_VERSION_1_5
    GLCAT.debug()
      << "Getting compile-time buffer pointers\n";
    _glGenBuffers = (PFNGLGENBUFFERSPROC)&GLP(GenBuffers);
    _glBindBuffer = (PFNGLBINDBUFFERPROC)&GLP(BindBuffer);
    _glBufferData = (PFNGLBUFFERDATAPROC)&GLP(BufferData);
    _glBufferSubData = (PFNGLBUFFERSUBDATAPROC)&GLP(BufferSubData);
    _glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)&GLP(DeleteBuffers);
#else  // EXPECT_GL_VERSION_1_5
    GLCAT.debug()
      << "Getting run-time buffer pointers\n";
    _glGenBuffers = (PFNGLGENBUFFERSPROC)
      get_extension_func(GLPREFIX_QUOTED, "GenBuffers");
    _glBindBuffer = (PFNGLBINDBUFFERPROC)
      get_extension_func(GLPREFIX_QUOTED, "BindBuffer");
    _glBufferData = (PFNGLBUFFERDATAPROC)
      get_extension_func(GLPREFIX_QUOTED, "BufferData");
    _glBufferSubData = (PFNGLBUFFERSUBDATAPROC)
      get_extension_func(GLPREFIX_QUOTED, "BufferSubData");
    _glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)
      get_extension_func(GLPREFIX_QUOTED, "DeleteBuffers");
#endif  // EXPECT_GL_VERSION_1_5

  } else if (has_extension("GL_ARB_vertex_buffer_object")) {
    _supports_buffers = true;

    _glGenBuffers = (PFNGLGENBUFFERSPROC)
      get_extension_func(GLPREFIX_QUOTED, "GenBuffersARB");
    _glBindBuffer = (PFNGLBINDBUFFERPROC)
      get_extension_func(GLPREFIX_QUOTED, "BindBufferARB");
    _glBufferData = (PFNGLBUFFERDATAPROC)
      get_extension_func(GLPREFIX_QUOTED, "BufferDataARB");
    _glBufferSubData = (PFNGLBUFFERSUBDATAPROC)
      get_extension_func(GLPREFIX_QUOTED, "BufferSubDataARB");
    _glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)
      get_extension_func(GLPREFIX_QUOTED, "DeleteBuffersARB");
  }

  if (_supports_buffers) {
    if (_glGenBuffers == NULL || _glBindBuffer == NULL ||
        _glBufferData == NULL || _glBufferSubData == NULL ||
        _glDeleteBuffers == NULL) {
      GLCAT.warning()
        << "Buffers advertised as supported by OpenGL runtime, but could not get pointers to extension functions.\n";
      _supports_buffers = false;
    }
  }

#ifdef HAVE_CG
  if (cgGLIsProfileSupported(CG_PROFILE_ARBFP1) &&
      cgGLIsProfileSupported(CG_PROFILE_ARBVP1)) {
    _supports_basic_shaders = true;
    if (basic_shaders_only) {
      _shader_caps._active_vprofile = (int)CG_PROFILE_ARBVP1;
      _shader_caps._active_fprofile = (int)CG_PROFILE_ARBFP1;
    } else { 
      _shader_caps._active_vprofile = (int)cgGLGetLatestProfile(CG_GL_VERTEX);
      _shader_caps._active_fprofile = (int)cgGLGetLatestProfile(CG_GL_FRAGMENT);
    }
    _shader_caps._ultimate_vprofile = (int)CG_PROFILE_VP40;
    _shader_caps._ultimate_fprofile = (int)CG_PROFILE_FP40;
    _glBindProgram = (PFNGLBINDPROGRAMARBPROC)
      get_extension_func(GLPREFIX_QUOTED, "BindProgramARB");
    // Bug workaround for radeons.
    if (_shader_caps._active_fprofile == CG_PROFILE_ARBFP1) {
      if (has_extension("GL_ATI_draw_buffers")) {
        _shader_caps._bug_list.insert(Shader::SBUG_ati_draw_buffers);
      }
    }    
  }
#endif

  _supports_framebuffer_object = false;
  if (has_extension("GL_EXT_framebuffer_object")) {
    _supports_framebuffer_object = true;
    _glIsRenderbuffer = (PFNGLISRENDERBUFFEREXTPROC)
      get_extension_func(GLPREFIX_QUOTED, "IsRenderbufferEXT");
    _glBindRenderbuffer = (PFNGLBINDRENDERBUFFEREXTPROC)
      get_extension_func(GLPREFIX_QUOTED, "BindRenderbufferEXT");
    _glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSEXTPROC)
      get_extension_func(GLPREFIX_QUOTED, "DeleteRenderbuffersEXT");
    _glGenRenderbuffers = (PFNGLGENRENDERBUFFERSEXTPROC)
      get_extension_func(GLPREFIX_QUOTED, "GenRenderbuffersEXT");
    _glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEEXTPROC)
      get_extension_func(GLPREFIX_QUOTED, "RenderbufferStorageEXT");
    _glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC)
      get_extension_func(GLPREFIX_QUOTED, "GetRenderbufferParameterivEXT");
    _glIsFramebuffer = (PFNGLISFRAMEBUFFEREXTPROC)
      get_extension_func(GLPREFIX_QUOTED, "IsFramebufferEXT");
    _glBindFramebuffer = (PFNGLBINDFRAMEBUFFEREXTPROC)
      get_extension_func(GLPREFIX_QUOTED, "BindFramebufferEXT");
    _glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSEXTPROC)
      get_extension_func(GLPREFIX_QUOTED, "DeleteFramebuffersEXT");
    _glGenFramebuffers = (PFNGLGENFRAMEBUFFERSEXTPROC)
      get_extension_func(GLPREFIX_QUOTED, "GenFramebuffersEXT");
    _glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)
      get_extension_func(GLPREFIX_QUOTED, "CheckFramebufferStatusEXT");
    _glFramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DEXTPROC)
      get_extension_func(GLPREFIX_QUOTED, "FramebufferTexture1DEXT");
    _glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)
      get_extension_func(GLPREFIX_QUOTED, "FramebufferTexture2DEXT");
    _glFramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DEXTPROC)
      get_extension_func(GLPREFIX_QUOTED, "FramebufferTexture3DEXT");
    _glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)
      get_extension_func(GLPREFIX_QUOTED, "FramebufferRenderbufferEXT");
    _glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC)
      get_extension_func(GLPREFIX_QUOTED, "GetFramebufferAttachmentParameterivEXT");
    _glGenerateMipmap = (PFNGLGENERATEMIPMAPEXTPROC)
      get_extension_func(GLPREFIX_QUOTED, "GenerateMipmapEXT");
  }

  _glDrawBuffers = NULL;
  if (is_at_least_version(2, 0)) {
    _glDrawBuffers = (PFNGLDRAWBUFFERSPROC)
      get_extension_func(GLPREFIX_QUOTED, "DrawBuffers");
  } else if (has_extension("GL_ARB_draw_buffers")) {
    _glDrawBuffers = (PFNGLDRAWBUFFERSPROC)
      get_extension_func(GLPREFIX_QUOTED, "DrawBuffersARB");
  }
  _max_draw_buffers = 1;
  if (_glDrawBuffers != 0) {
    GLint max_draw_buffers = 0;
    GLP(GetIntegerv)(GL_MAX_DRAW_BUFFERS, &max_draw_buffers);
    _max_draw_buffers = max_draw_buffers;
  }

  _supports_occlusion_query = false;
  if (CLP(support_occlusion_query)) {
    if (is_at_least_version(1, 5)) {
      _supports_occlusion_query = true;

#ifdef EXPECT_GL_VERSION_1_5
      GLCAT.debug()
        << "Getting compile-time occlusion pointers\n";
      _glGenQueries = (PFNGLGENQUERIESPROC)&GLP(GenQueries);
      _glBeginQuery = (PFNGLBEGINQUERYPROC)&GLP(BeginQuery);
      _glEndQuery = (PFNGLENDQUERYPROC)&GLP(EndQuery);
      _glDeleteQueries = (PFNGLDELETEQUERIESPROC)&GLP(DeleteQueries);
      _glGetQueryiv = (PFNGLGETQUERYIVPROC)&GLP(GetQueryiv);
      _glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)&GLP(GetQueryObjectuiv);

#else  // EXPECT_GL_VERSION_1_5
      GLCAT.debug()
        << "Getting run-time occlusion pointers\n";
      _glGenQueries = (PFNGLGENQUERIESPROC)
        get_extension_func(GLPREFIX_QUOTED, "GenQueries");
      _glBeginQuery = (PFNGLBEGINQUERYPROC)
        get_extension_func(GLPREFIX_QUOTED, "BeginQuery");
      _glEndQuery = (PFNGLENDQUERYPROC)
        get_extension_func(GLPREFIX_QUOTED, "EndQuery");
      _glDeleteQueries = (PFNGLDELETEQUERIESPROC)
        get_extension_func(GLPREFIX_QUOTED, "DeleteQueries");
      _glGetQueryiv = (PFNGLGETQUERYIVPROC)
        get_extension_func(GLPREFIX_QUOTED, "GetQueryiv");
      _glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)
        get_extension_func(GLPREFIX_QUOTED, "GetQueryObjectuiv");
#endif  // EXPECT_GL_VERSION_1_5

    } else if (has_extension("GL_ARB_occlusion_query")) {
      _supports_occlusion_query = true;
      _glGenQueries = (PFNGLGENQUERIESPROC)
        get_extension_func(GLPREFIX_QUOTED, "GenQueriesARB");
      _glBeginQuery = (PFNGLBEGINQUERYPROC)
        get_extension_func(GLPREFIX_QUOTED, "BeginQueryARB");
      _glEndQuery = (PFNGLENDQUERYPROC)
        get_extension_func(GLPREFIX_QUOTED, "EndQueryARB");
      _glDeleteQueries = (PFNGLDELETEQUERIESPROC)
        get_extension_func(GLPREFIX_QUOTED, "DeleteQueriesARB");
      _glGetQueryiv = (PFNGLGETQUERYIVPROC)
        get_extension_func(GLPREFIX_QUOTED, "GetQueryivARB");
      _glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)
        get_extension_func(GLPREFIX_QUOTED, "GetQueryObjectuivARB");
    }
  }

  if (_supports_occlusion_query) {
    if (_glGenQueries == NULL || _glBeginQuery == NULL ||
        _glEndQuery == NULL || _glDeleteQueries == NULL ||
        _glGetQueryiv == NULL || _glGetQueryObjectuiv == NULL) {
      GLCAT.warning()
        << "Occlusion queries advertised as supported by OpenGL runtime, but could not get pointers to extension functions.\n";
      _supports_occlusion_query = false;
    } else {
      GLint num_bits;
      _glGetQueryiv(GL_SAMPLES_PASSED, GL_QUERY_COUNTER_BITS, &num_bits);
      if (num_bits == 0) {
        _supports_occlusion_query = false;
      }
      if (GLCAT.is_debug()) {
        GLCAT.debug()
          << "Occlusion query counter provides " << num_bits << " bits.\n";
      }
    }
  }

  _glBlendEquation = NULL;
  bool supports_blend_equation = false;
  if (is_at_least_version(1, 2)) {
    supports_blend_equation = true;
#ifdef EXPECT_GL_VERSION_1_2
    GLCAT.debug()
      << "Getting compile-time BlendEquation pointers\n";
    _glBlendEquation = (PFNGLBLENDEQUATIONPROC)&GLP(BlendEquation);
#else  // EXPECT_GL_VERSION_1_2
    GLCAT.debug()
      << "Getting run-time blend pointers\n";
    _glBlendEquation = (PFNGLBLENDEQUATIONPROC)
      get_extension_func(GLPREFIX_QUOTED, "BlendEquation");
#endif  // EXPECT_GL_VERSION_1_2
  } else if (has_extension("GL_EXT_blend_minmax")) {
    supports_blend_equation = true;
    _glBlendEquation = (PFNGLBLENDEQUATIONPROC)
      get_extension_func(GLPREFIX_QUOTED, "BlendEquationEXT");
  }
  if (supports_blend_equation && _glBlendEquation == NULL) {
    GLCAT.warning()
      << "BlendEquation advertised as supported by OpenGL runtime, but could not get pointers to extension function.\n";
  }
  if (_glBlendEquation == NULL) {
    _glBlendEquation = null_glBlendEquation;
  }

  _glBlendColor = NULL;
  bool supports_blend_color = false;
  if (is_at_least_version(1, 2)) {
    supports_blend_color = true;
#ifdef EXPECT_GL_VERSION_1_2
    GLCAT.debug()
      << "Getting compile-time BlendColor pointers\n";
    _glBlendColor = (PFNGLBLENDCOLORPROC)&GLP(BlendColor);
#else  // EXPECT_GL_VERSION_1_2
    GLCAT.debug()
      << "Getting run-time BlendColor pointers\n";
    _glBlendColor = (PFNGLBLENDCOLORPROC)
      get_extension_func(GLPREFIX_QUOTED, "BlendColor");
#endif  // EXPECT_GL_VERSION_1_2
  } else if (has_extension("GL_EXT_blend_color")) {
    supports_blend_color = true;
    _glBlendColor = (PFNGLBLENDCOLORPROC)
      get_extension_func(GLPREFIX_QUOTED, "BlendColorEXT");
  }
  if (supports_blend_color && _glBlendColor == NULL) {
    GLCAT.warning()
      << "BlendColor advertised as supported by OpenGL runtime, but could not get pointers to extension function.\n";
  }
  if (_glBlendColor == NULL) {
    _glBlendColor = null_glBlendColor;
  }

  _edge_clamp = GL_CLAMP;
  if (has_extension("GL_SGIS_texture_edge_clamp") ||
      is_at_least_version(1, 2)) {
    _edge_clamp = GL_CLAMP_TO_EDGE;
  }

  _border_clamp = GL_CLAMP;
  if (CLP(support_clamp_to_border) &&
      (has_extension("GL_ARB_texture_border_clamp") ||
       is_at_least_version(1, 3))) {
    _border_clamp = GL_CLAMP_TO_BORDER;
  }

  _mirror_repeat = GL_REPEAT;
  if (has_extension("GL_ARB_texture_mirrored_repeat") ||
      is_at_least_version(1, 4)) {
    _mirror_repeat = GL_MIRRORED_REPEAT;
  }

  _mirror_clamp = GL_CLAMP;
  _mirror_edge_clamp = _edge_clamp;
  _mirror_border_clamp = _border_clamp;
  if (has_extension("GL_EXT_texture_mirror_clamp")) {
    _mirror_clamp = GL_MIRROR_CLAMP_EXT;
    _mirror_edge_clamp = GL_MIRROR_CLAMP_TO_EDGE_EXT;
    _mirror_border_clamp = GL_MIRROR_CLAMP_TO_BORDER_EXT;
  }

  if (_supports_multisample) {
    GLint sample_buffers = 0;
    GLP(GetIntegerv)(GL_SAMPLE_BUFFERS, &sample_buffers);
    if (sample_buffers != 1) {
      // Even if the API supports multisample, we might have ended up
      // with a framebuffer that doesn't have any multisample bits.
      // (It's also possible the graphics card doesn't provide any
      // framebuffers with multisample.)  In this case, we don't
      // really support the multisample API's, since they won't do
      // anything.
      _supports_multisample = false;
    }
  }

  GLint max_texture_size = 0;
  GLint max_3d_texture_size = 0;
  GLint max_cube_map_size = 0;

  GLP(GetIntegerv)(GL_MAX_TEXTURE_SIZE, &max_texture_size);
  _max_texture_dimension = max_texture_size;

  if (_supports_3d_texture) {
    GLP(GetIntegerv)(GL_MAX_3D_TEXTURE_SIZE, &max_3d_texture_size);
    _max_3d_texture_dimension = max_3d_texture_size;
  } else {
    _max_3d_texture_dimension = 0;
  }

  if (_supports_cube_map) {
    GLP(GetIntegerv)(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &max_cube_map_size);
    _max_cube_map_dimension = max_cube_map_size;
  } else {
    _max_cube_map_dimension = 0;
  }

  GLint max_elements_vertices = 0, max_elements_indices = 0;
  GLP(GetIntegerv)(GL_MAX_ELEMENTS_VERTICES, &max_elements_vertices);
  GLP(GetIntegerv)(GL_MAX_ELEMENTS_INDICES, &max_elements_indices);
  if (max_elements_vertices > 0) {
    _max_vertices_per_array = max_elements_vertices;
  }
  if (max_elements_indices > 0) {
    _max_vertices_per_primitive = max_elements_indices;
  }

  if (GLCAT.is_debug()) {
    GLCAT.debug()
      << "max texture dimension = " << _max_texture_dimension
      << ", max 3d texture = " << _max_3d_texture_dimension
      << ", max cube map = " << _max_cube_map_dimension << "\n";
    GLCAT.debug()
      << "max_elements_vertices = " << max_elements_vertices
      << ", max_elements_indices = " << max_elements_indices << "\n";
    if (_supports_buffers) {
      if (vertex_buffers) {
        GLCAT.debug()
          << "vertex buffer objects are supported.\n";
      } else {
        GLCAT.debug()
          << "vertex buffer objects are supported (but not enabled).\n";
      }
    } else {
      GLCAT.debug()
        << "vertex buffer objects are NOT supported.\n";
    }

#ifdef SUPPORT_IMMEDIATE_MODE
    if (!vertex_arrays) {
      GLCAT.debug()
        << "immediate mode commands will be used instead of vertex arrays.\n";
    }
#endif

    if (!_supports_compressed_texture) {
      GLCAT.debug()
        << "Texture compression is not supported.\n";

    } else {
      GLint num_compressed_formats = 0;
      GLP(GetIntegerv)(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &num_compressed_formats);
      if (num_compressed_formats == 0) {
        GLCAT.debug()
          << "No specific compressed texture formats are supported.\n";
      } else {
        GLCAT.debug()
          << "Supported compressed texture formats:\n";
        GLint *formats = (GLint *)PANDA_MALLOC_ARRAY(num_compressed_formats * sizeof(GLint));
        GLP(GetIntegerv)(GL_COMPRESSED_TEXTURE_FORMATS, formats);
        for (int i = 0; i < num_compressed_formats; ++i) {
          switch (formats[i]) {
          case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            GLCAT.debug(false) << "  GL_COMPRESSED_RGB_S3TC_DXT1_EXT\n";
            break;

          case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            GLCAT.debug(false) << "  GL_COMPRESSED_RGBA_S3TC_DXT1_EXT\n";
            break;

          case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
            GLCAT.debug(false) << "  GL_COMPRESSED_RGBA_S3TC_DXT3_EXT\n";
            break;

          case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            GLCAT.debug(false) << "  GL_COMPRESSED_RGBA_S3TC_DXT5_EXT\n";
            break;

          case GL_COMPRESSED_RGB_FXT1_3DFX:
            GLCAT.debug(false) << "  GL_COMPRESSED_RGB_FXT1_3DFX\n";
            break;

          case GL_COMPRESSED_RGBA_FXT1_3DFX:
            GLCAT.debug(false) << "  GL_COMPRESSED_RGBA_FXT1_3DFX\n";
            break;

          default:
            GLCAT.debug(false)
              << "  Unknown compressed format 0x" << hex << formats[i]
              << dec << "\n";
          }
        }
        PANDA_FREE_ARRAY(formats);
      }
    }
  }

  _num_active_texture_stages = 0;

  report_my_gl_errors();

  _supports_stencil_wrap = has_extension("GL_EXT_stencil_wrap");
  _supports_two_sided_stencil = has_extension("GL_EXT_stencil_two_side");
  if (_supports_two_sided_stencil) {
    _glActiveStencilFaceEXT = (PFNGLACTIVESTENCILFACEEXTPROC)
      get_extension_func(GLPREFIX_QUOTED, "ActiveStencilFaceEXT");
  }
  else {
    _glActiveStencilFaceEXT = 0;
  }

  _auto_rescale_normal = false;

  // Ensure the initial state is what we say it should be (in some
  // cases, we don't want the GL default settings; in others, we have
  // to force the point with some drivers that aren't strictly
  // compliant w.r.t. initial settings).
  GLP(FrontFace)(GL_CCW);
  GLP(Disable)(GL_LINE_SMOOTH);
  GLP(Disable)(GL_POINT_SMOOTH);
  GLP(Disable)(GL_POLYGON_SMOOTH);

  if (_supports_multisample) {
    GLP(Disable)(GL_MULTISAMPLE);
  }

  // Set up all the enabled/disabled flags to GL's known initial
  // values: everything off.
  _multisample_mode = 0;
  _line_smooth_enabled = false;
  _point_smooth_enabled = false;
  _polygon_smooth_enabled = false;
  _scissor_enabled = false;
  _stencil_test_enabled = false;
  _blend_enabled = false;
  _depth_test_enabled = false;
  _fog_enabled = false;
  _alpha_test_enabled = false;
  _polygon_offset_enabled = false;
  _flat_shade_model = false;
  _decal_level = 0;
  _tex_gen_point_sprite = false;

  // Dither is on by default in GL; let's turn it off
  GLP(Disable)(GL_DITHER);
  _dithering_enabled = false;

  _current_shader = (Shader *)NULL;
  _current_shader_context = (CLP(ShaderContext) *)NULL;
  _vertex_array_shader = (Shader *)NULL;
  _vertex_array_shader_context = (CLP(ShaderContext) *)NULL;
  _texture_binding_shader = (Shader *)NULL;
  _texture_binding_shader_context = (CLP(ShaderContext) *)NULL;

  // Count the max number of lights
  GLint max_lights = 0;
  GLP(GetIntegerv)(GL_MAX_LIGHTS, &max_lights);
  _max_lights = max_lights;

  if (GLCAT.is_debug()) {
    GLCAT.debug()
      << "max lights = " << _max_lights << "\n";
  }

  // Count the max number of clipping planes
  GLint max_clip_planes = 0;
  GLP(GetIntegerv)(GL_MAX_CLIP_PLANES, &max_clip_planes);
  _max_clip_planes = max_clip_planes;

  if (GLCAT.is_debug()) {
    GLCAT.debug()
      << "max clip planes = " << _max_clip_planes << "\n";
  }

  if (_supports_multitexture) {
    GLint max_texture_stages = 0;
    GLP(GetIntegerv)(GL_MAX_TEXTURE_UNITS, &max_texture_stages);
    _max_texture_stages = max_texture_stages;

    if (GLCAT.is_debug()) {
      GLCAT.debug()
        << "max texture stages = " << _max_texture_stages << "\n";
    }
  }
  _current_vbuffer_index = 0;
  _current_ibuffer_index = 0;
  _current_fbo = 0;
  _auto_antialias_mode = false;
  _render_mode = RenderModeAttrib::M_filled;
  _point_size = 1.0f;
  _point_perspective = false;

  _vertex_blending_enabled = false;

  report_my_gl_errors();

  if (CLP(cheap_textures)) {
    GLCAT.info()
      << "Setting GLP(Hint)() for fastest textures.\n";
    GLP(Hint)(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
  }

  // use per-vertex fog if per-pixel fog requires SW renderer
  GLP(Hint)(GL_FOG_HINT, GL_DONT_CARE);

  GLint num_red_bits = 0;
  GLP(GetIntegerv)(GL_RED_BITS, &num_red_bits);
  if (num_red_bits < 8) {
    GLP(Enable)(GL_DITHER);
    _dithering_enabled = true;
    if (GLCAT.is_debug()) {
      GLCAT.debug()
        << "frame buffer depth = " << num_red_bits
        << " bits/channel, enabling dithering\n";
    }
  }

  _error_count = 0;

  report_my_gl_errors();

  void gl_set_stencil_functions (StencilRenderStates *stencil_render_states);
  gl_set_stencil_functions (_stencil_render_states);

#ifdef HAVE_CG

  typedef struct
  {
    CGprofile cg_profile;
    int shader_model;
  }
  CG_PROFILE_TO_SHADER_MODEL;

  static CG_PROFILE_TO_SHADER_MODEL cg_profile_to_shader_model_array [ ] = {
    // fp40 - OpenGL fragment profile for NV4x (GeForce 6xxx and 7xxx
    // Series, NV4x-based Quadro FX, etc.)
    CG_PROFILE_FP40,
    SM_30,
    
    // fp30 - OpenGL fragment profile for NV3x (GeForce FX, Quadro FX, etc.)
    CG_PROFILE_FP30,
    SM_2X,
    
    // This OpenGL profile corresponds to the per-fragment
    // functionality introduced by GeForce FX and other DirectX 9
    // GPUs.
    CG_PROFILE_ARBFP1,
    SM_20,
    
    // fp20 - OpenGL fragment profile for NV2x (GeForce3, GeForce4 Ti,
    // Quadro DCC, etc.)
    CG_PROFILE_FP20,
    SM_11,
    
    // no shader support
    CG_PROFILE_UNKNOWN,
    SM_00,
  };
  
  int index;
  CG_PROFILE_TO_SHADER_MODEL *cg_profile_to_shader_model;

  index = 0;
  cg_profile_to_shader_model = cg_profile_to_shader_model_array;
  while (cg_profile_to_shader_model -> shader_model != SM_00) {
    if (cgGLIsProfileSupported(cg_profile_to_shader_model -> cg_profile)) {
      _shader_model = cg_profile_to_shader_model -> shader_model;
      break;
    }
    cg_profile_to_shader_model++;
  }

  // DisplayInformation may have better shader model detection
  {
    GraphicsPipe *pipe;
    DisplayInformation *display_information;
    
    pipe = this -> get_pipe ( );
    if (pipe) {
      display_information = pipe -> get_display_information ( );
      if (display_information) {
        if (display_information -> get_shader_model ( ) > _shader_model) {
          _shader_model = display_information -> get_shader_model ( );
        }
      }
    }
  }  
  _auto_detect_shader_model = _shader_model;

  CGprofile vertex_profile;
  CGprofile pixel_profile;

  vertex_profile = cgGLGetLatestProfile (CG_GL_VERTEX);
  pixel_profile = cgGLGetLatestProfile (CG_GL_FRAGMENT);
  if (GLCAT.is_debug()) {
    // Temp ifdef: this crashes Mesa.
#ifndef OSMESA_MAJOR_VERSION
    GLCAT.debug()
      << "\nCg vertex profile = " << cgGetProfileString(vertex_profile) << "  id = " << vertex_profile
      << "\nCg pixel profile = " << cgGetProfileString(pixel_profile) << "  id = " << pixel_profile
      << "\nshader model = " << _shader_model
      << "\n";
#endif
  }

#endif

  // Now that the GSG has been initialized, make it available for
  // optimizations.
  add_gsg(this);
}


////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::clear
//       Access: Public
//  Description: Clears the framebuffer within the current
//               DisplayRegion, according to the flags indicated by
//               the given DrawableRegion object.
//
//               This does not set the DisplayRegion first.  You
//               should call prepare_display_region() to specify the
//               region you wish the clear operation to apply to.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
clear(DrawableRegion *clearable) {
  PStatTimer timer(_clear_pcollector);

  if ((!clearable->get_clear_color_active())&&
      (!clearable->get_clear_depth_active())&&
      (!clearable->get_clear_stencil_active())) {
    return;
  }
  
  set_state_and_transform(RenderState::make_empty(), _internal_transform);

  int mask = 0;

  for (int i=0; i<_current_properties->get_aux_rgba(); i++) {
    int layerid = GraphicsOutput::RTP_aux_rgba_0 + i;
    int layerbit = RenderBuffer::T_aux_rgba_0 << i;
    if (clearable->get_clear_active(layerid)) {
      Colorf v = clearable->get_clear_value(layerid);
      GLP(ClearColor)(v[0],v[1],v[2],v[3]);
      set_draw_buffer(layerbit);
      GLP(Clear)(GL_COLOR_BUFFER_BIT);
    }
  }
  for (int i=0; i<_current_properties->get_aux_hrgba(); i++) {
    int layerid = GraphicsOutput::RTP_aux_hrgba_0 + i;
    int layerbit = RenderBuffer::T_aux_hrgba_0 << i;
    if (clearable->get_clear_active(layerid)) {
      Colorf v = clearable->get_clear_value(layerid);
      GLP(ClearColor)(v[0],v[1],v[2],v[3]);
      set_draw_buffer(layerbit);
      GLP(Clear)(GL_COLOR_BUFFER_BIT);
    }
  }
  for (int i=0; i<_current_properties->get_aux_float(); i++) {
    int layerid = GraphicsOutput::RTP_aux_float_0 + i;
    int layerbit = RenderBuffer::T_aux_float_0 << i;
    if (clearable->get_clear_active(layerid)) {
      Colorf v = clearable->get_clear_value(layerid);
      GLP(ClearColor)(v[0],v[1],v[2],v[3]);
      set_draw_buffer(layerbit);
      GLP(Clear)(GL_COLOR_BUFFER_BIT);
    }
  }
  
  if (clearable->get_clear_color_active()) {
    Colorf v = clearable->get_clear_color();
    GLP(ClearColor)(v[0],v[1],v[2],v[3]);
    mask |= GL_COLOR_BUFFER_BIT;
    set_draw_buffer(clearable->get_draw_buffer_type());
  }
  
  if (clearable->get_clear_depth_active()) {
    GLP(ClearDepth)(clearable->get_clear_depth());
    mask |= GL_DEPTH_BUFFER_BIT;
  }

  if (clearable->get_clear_stencil_active()) {
    GLP(ClearStencil)(clearable->get_clear_stencil());
    mask |= GL_STENCIL_BUFFER_BIT;
  }
  
  GLP(Clear)(mask);
  
  // In the past, it was possible to set the draw buffer
  // once in prepare_display_region and then forget about it.
  // Now, with aux layers, it is necessary to occasionally
  // change the draw buffer.  In time, I think there will need
  // to be a draw buffer attrib.  Until then, this little hack
  // to put things back the way they were after
  // prepare_display_region will bdo.
  
  set_draw_buffer(_draw_buffer_type);

  if (GLCAT.is_spam()) {
    GLCAT.spam() << "glClear(";
    if (mask & GL_COLOR_BUFFER_BIT) {
      GLCAT.spam(false) << "GL_COLOR_BUFFER_BIT|";
    }
    if (mask & GL_DEPTH_BUFFER_BIT) {
      GLCAT.spam(false) << "GL_DEPTH_BUFFER_BIT|";
    }
    if (mask & GL_STENCIL_BUFFER_BIT) {
      GLCAT.spam(false) << "GL_STENCIL_BUFFER_BIT|";
    }
    if (mask & GL_ACCUM_BUFFER_BIT) {
      GLCAT.spam(false) << "GL_ACCUM_BUFFER_BIT|";
    }
    GLCAT.spam(false) << ")" << endl;
  }

  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::prepare_display_region
//       Access: Public, Virtual
//  Description: Prepare a display region for rendering (set up
//               scissor region and viewport)
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
prepare_display_region(DisplayRegionPipelineReader *dr,
                       Lens::StereoChannel stereo_channel) {
  nassertv(dr != (DisplayRegionPipelineReader *)NULL);
  GraphicsStateGuardian::prepare_display_region(dr, stereo_channel);

  int l, b, w, h;
  dr->get_region_pixels(l, b, w, h);
  _viewport_width = w;
  _viewport_height = h;
  GLint x = GLint(l);
  GLint y = GLint(b);
  GLsizei width = GLsizei(w);
  GLsizei height = GLsizei(h);

  _draw_buffer_type = dr->get_object()->get_draw_buffer_type() & _current_properties->get_buffer_mask() & _stereo_buffer_mask;
  _draw_buffer_type |= _current_properties->get_aux_mask();
  set_draw_buffer(_draw_buffer_type);
  
  enable_scissor(true);
  GLP(Scissor)(x, y, width, height);
  GLP(Viewport)(x, y, width, height);

  report_my_gl_errors();
  do_point_size();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::calc_projection_mat
//       Access: Public, Virtual
//  Description: Given a lens, calculates the appropriate projection
//               matrix for use with this gsg.  Note that the
//               projection matrix depends a lot upon the coordinate
//               system of the rendering API.
//
//               The return value is a TransformState if the lens is
//               acceptable, NULL if it is not.
////////////////////////////////////////////////////////////////////
CPT(TransformState) CLP(GraphicsStateGuardian)::
calc_projection_mat(const Lens *lens) {
  if (lens == (Lens *)NULL) {
    return NULL;
  }

  if (!lens->is_linear()) {
    return NULL;
  }

  // The projection matrix must always be right-handed Y-up, even if
  // our coordinate system of choice is otherwise, because certain GL
  // calls (specifically glTexGen(GL_SPHERE_MAP)) assume this kind of
  // a coordinate system.  Sigh.  In order to implement a Z-up (or
  // other arbitrary) coordinate system, we'll use a Y-up projection
  // matrix, and store the conversion to our coordinate system of
  // choice in the modelview matrix.

  LMatrix4f result =
    LMatrix4f::convert_mat(CS_yup_right, _current_lens->get_coordinate_system()) *
    lens->get_projection_mat(_current_stereo_channel);

  if (_scene_setup->get_inverted()) {
    // If the scene is supposed to be inverted, then invert the
    // projection matrix.
    result *= LMatrix4f::scale_mat(1.0f, -1.0f, 1.0f);
  }

  return TransformState::make_mat(result);
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::prepare_lens
//       Access: Public, Virtual
//  Description: Makes the current lens (whichever lens was most
//               recently specified with set_scene()) active, so
//               that it will transform future rendered geometry.
//               Normally this is only called from the draw process,
//               and usually it is called by set_scene().
//
//               The return value is true if the lens is acceptable,
//               false if it is not.
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
prepare_lens() {
  if (GLCAT.is_spam()) {
    GLCAT.spam()
      << "glMatrixMode(GL_PROJECTION): " << _projection_mat->get_mat() << endl;
  }
  GLP(MatrixMode)(GL_PROJECTION);
  GLP(LoadMatrixf)(_projection_mat->get_mat().get_data());
  report_my_gl_errors();

  do_point_size();

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::begin_frame
//       Access: Public, Virtual
//  Description: Called before each frame is rendered, to allow the
//               GSG a chance to do any internal cleanup before
//               beginning the frame.
//
//               The return value is true if successful (in which case
//               the frame will be drawn and end_frame() will be
//               called later), or false if unsuccessful (in which
//               case nothing will be drawn and end_frame() will not
//               be called).
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
begin_frame(Thread *current_thread) {
  if (!GraphicsStateGuardian::begin_frame(current_thread)) {
    return false;
  }

#ifdef DO_PSTATS
  _vertices_display_list_pcollector.clear_level();
  _vertices_immediate_pcollector.clear_level();
  _primitive_batches_display_list_pcollector.clear_level();
#endif

  report_my_gl_errors();
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: GraphicsStateGuardian::begin_scene
//       Access: Public, Virtual
//  Description: Called between begin_frame() and end_frame() to mark
//               the beginning of drawing commands for a "scene"
//               (usually a particular DisplayRegion) within a frame.
//               All 3-D drawing commands, except the clear operation,
//               must be enclosed within begin_scene() .. end_scene().
//
//               The return value is true if successful (in which case
//               the scene will be drawn and end_scene() will be
//               called later), or false if unsuccessful (in which
//               case nothing will be drawn and end_scene() will not
//               be called).
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
begin_scene() {
  return GraphicsStateGuardian::begin_scene();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::end_scene
//       Access: Protected, Virtual
//  Description: Called between begin_frame() and end_frame() to mark
//               the end of drawing commands for a "scene" (usually a
//               particular DisplayRegion) within a frame.  All 3-D
//               drawing commands, except the clear operation, must be
//               enclosed within begin_scene() .. end_scene().
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
end_scene() {
  GraphicsStateGuardian::end_scene();

  if (_vertex_array_shader_context != 0) {
    _vertex_array_shader_context->disable_shader_vertex_arrays(this);
    _vertex_array_shader = (Shader *)NULL;
    _vertex_array_shader_context = (CLP(ShaderContext) *)NULL;
  }
  if (_texture_binding_shader_context != 0) {
    _texture_binding_shader_context->disable_shader_texture_bindings(this);
    _texture_binding_shader = (Shader *)NULL;
    _texture_binding_shader_context = (CLP(ShaderContext) *)NULL;
  }
  if (_current_shader_context != 0) {
    _current_shader_context->unbind();
    _current_shader = (Shader *)NULL;
    _current_shader_context = (CLP(ShaderContext) *)NULL;
  }
  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::end_frame
//       Access: Public, Virtual
//  Description: Called after each frame is rendered, to allow the
//               GSG a chance to do any internal cleanup after
//               rendering the frame, and before the window flips.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
end_frame(Thread *current_thread) {
#ifdef DO_PSTATS
  // Check for textures, etc., that are no longer resident.  These
  // calls might be measurably expensive, and they don't have any
  // benefit unless we are actually viewing PStats, so don't do them
  // unless we're connected.  That will just mean that we'll count
  // everything as resident until the user connects PStats, at which
  // point it will then correct the assessment.  No harm done.
  if (PStatClient::is_connected()) {
    check_nonresident_texture(_prepared_objects->_texture_residency.get_inactive_resident());
    check_nonresident_texture(_prepared_objects->_texture_residency.get_active_resident());

    // OpenGL provides no methods for querying whether a buffer object
    // (vertex buffer) is resident.  In fact, the API appears geared
    // towards the assumption that such buffers are always resident.
    // OK.
  }
#endif

  GraphicsStateGuardian::end_frame(current_thread);

  // Flush any PCollectors specific to this kind of GSG.
  _primitive_batches_display_list_pcollector.flush_level();
  _vertices_display_list_pcollector.flush_level();
  _vertices_immediate_pcollector.flush_level();

  // Now is a good time to delete any pending display lists.
  {
    MutexHolder holder(_lock);
    if (!_deleted_display_lists.empty()) {
      DeletedDisplayLists::iterator ddli;
      for (ddli = _deleted_display_lists.begin();
           ddli != _deleted_display_lists.end();
           ++ddli) {
        if (GLCAT.is_debug()) {
          GLCAT.debug()
            << "releasing display list index " << (*ddli) << "\n";
        }
        GLP(DeleteLists)((*ddli), 1);
      }
      _deleted_display_lists.clear();
    }

    // And deleted occlusion queries, too.
    if (!_deleted_queries.empty()) {
      DeletedDisplayLists::iterator ddli;
      for (ddli = _deleted_queries.begin();
           ddli != _deleted_queries.end();
           ++ddli) {
        if (GLCAT.is_debug()) {
          GLCAT.debug()
            << "releasing query index " << (*ddli) << "\n";
        }
        _glDeleteQueries(1, &(*ddli));
      }
      _deleted_queries.clear();
    }
  }

  // Calling glFlush() at the end of the frame is particularly
  // necessary if this is a single-buffered visual, so that the frame
  // will be finished drawing before we return to the application.
  // It's not clear what effect this has on our total frame time.
  gl_flush();
  maybe_gl_finish();

  report_my_gl_errors();
}


////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::begin_draw_primitives
//       Access: Public, Virtual
//  Description: Called before a sequence of draw_primitive()
//               functions are called, this should prepare the vertex
//               data for rendering.  It returns true if the vertices
//               are ok, false to abort this group of primitives.
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
begin_draw_primitives(const GeomPipelineReader *geom_reader,
                      const GeomMunger *munger,
                      const GeomVertexDataPipelineReader *data_reader,
                      bool force) {
#ifndef NDEBUG
  if (GLCAT.is_spam()) {
    GLCAT.spam() << "begin_draw_primitives: " << *(data_reader->get_object()) << "\n";
  }
#endif  // NDEBUG

  if (!GraphicsStateGuardian::begin_draw_primitives(geom_reader, munger, data_reader, force)) {
    return false;
  }
  nassertr(_data_reader != (GeomVertexDataPipelineReader *)NULL, false);

  _geom_display_list = 0;

  if (_auto_antialias_mode) {
    switch (geom_reader->get_primitive_type()) {
    case GeomPrimitive::PT_polygons:
      setup_antialias_polygon();
      break;
    case GeomPrimitive::PT_points:
      setup_antialias_point();
      break;
    case GeomPrimitive::PT_lines:
      setup_antialias_line();
      break;
    case GeomPrimitive::PT_none:
      break;
    }
    if ((_target._transparency != _state._transparency)||
        (_target._color_write != _state._color_write)||
        (_target._color_blend != _state._color_blend)) {
      do_issue_blending();
      _state._transparency = _target._transparency;
      _state._color_write = _target._color_write;
      _state._color_blend = _target._color_blend;
    }
  }

  const GeomVertexAnimationSpec &animation =
    _data_reader->get_format()->get_animation();
  bool hardware_animation = (animation.get_animation_type() == Geom::AT_hardware);
  if (hardware_animation) {
    // Set up the transform matrices for vertex blending.
    nassertr(_supports_vertex_blend, false);
    GLP(Enable)(GL_VERTEX_BLEND_ARB);
    _glVertexBlendARB(animation.get_num_transforms());

    const TransformTable *table = _data_reader->get_transform_table();
    if (table != (TransformTable *)NULL) {
      if (animation.get_indexed_transforms()) {
        nassertr(_supports_matrix_palette, false);
        // We are loading the indexed matrix palette.  The ARB decided
        // to change this interface from that for the list of
        // nonindexed matrices, to make it easier to load an arbitrary
        // number of matrices.
        GLP(Enable)(GL_MATRIX_PALETTE_ARB);

        GLP(MatrixMode)(GL_MATRIX_PALETTE_ARB);

        for (int i = 0; i < table->get_num_transforms(); ++i) {
          LMatrix4f mat;
          table->get_transform(i)->mult_matrix(mat, _internal_transform->get_mat());
          _glCurrentPaletteMatrixARB(i);
          GLP(LoadMatrixf)(mat.get_data());
        }

        // Presumably loading the matrix palette does not step on the
        // GL_MODELVIEW matrix?

      } else {
        // We are loading the list of nonindexed matrices.  This is a
        // little clumsier.

        if (_supports_matrix_palette) {
          GLP(Disable)(GL_MATRIX_PALETTE_ARB);
        }

        // GL_MODELVIEW0 and 1 are different than the rest.
        int i = 0;
        if (i < table->get_num_transforms()) {
          LMatrix4f mat;
          table->get_transform(i)->mult_matrix(mat, _internal_transform->get_mat());
          GLP(MatrixMode)(GL_MODELVIEW0_ARB);
          GLP(LoadMatrixf)(mat.get_data());
          ++i;
        }
        if (i < table->get_num_transforms()) {
          LMatrix4f mat;
          table->get_transform(i)->mult_matrix(mat, _internal_transform->get_mat());
          GLP(MatrixMode)(GL_MODELVIEW1_ARB);
          GLP(LoadMatrixf)(mat.get_data());
          ++i;
        }
        while (i < table->get_num_transforms()) {
          LMatrix4f mat;
          table->get_transform(i)->mult_matrix(mat, _internal_transform->get_mat());
          GLP(MatrixMode)(GL_MODELVIEW2_ARB + i - 2);
          GLP(LoadMatrixf)(mat.get_data());
          ++i;
        }

        // Setting the GL_MODELVIEW0 matrix steps on the world matrix,
        // so we have to set a flag to reload the world matrix later.
        _transform_stale = true;
      }
    }
    _vertex_blending_enabled = true;

  } else {
    // We're not using vertex blending.
    if (_vertex_blending_enabled) {
      GLP(Disable)(GL_VERTEX_BLEND_ARB);
      if (_supports_matrix_palette) {
        GLP(Disable)(GL_MATRIX_PALETTE_ARB);
      }
      _vertex_blending_enabled = false;
    }

    if (_transform_stale) {
      GLP(MatrixMode)(GL_MODELVIEW);
      GLP(LoadMatrixf)(_internal_transform->get_mat().get_data());
    }
  }

  if (_data_reader->is_vertex_transformed()) {
    // If the vertex data claims to be already transformed into clip
    // coordinates, wipe out the current projection and modelview
    // matrix (so we don't attempt to transform it again).
    GLP(MatrixMode)(GL_PROJECTION);
    GLP(PushMatrix)();
    GLP(LoadIdentity)();
    GLP(MatrixMode)(GL_MODELVIEW);
    GLP(PushMatrix)();
    GLP(LoadIdentity)();
  }

  if (geom_reader->get_usage_hint() == Geom::UH_static &&
      _data_reader->get_usage_hint() == Geom::UH_static &&
      display_lists && (!hardware_animation || display_list_animation)) {
    // If the geom claims to be totally static, try to build it into
    // a display list.

    // Before we compile or call a display list, make sure the current
    // buffers are unbound, or the nVidia drivers may crash.
    if (_current_vbuffer_index != 0) {
      if (GLCAT.is_debug() && CLP(debug_buffers)) {
        GLCAT.debug()
          << "unbinding vertex buffer\n";
      }
      _glBindBuffer(GL_ARRAY_BUFFER, 0);
      _current_vbuffer_index = 0;
    }
    if (_current_ibuffer_index != 0) {
      if (GLCAT.is_spam()) {
        GLCAT.spam()
          << "unbinding index buffer\n";
      }
      _glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
      _current_ibuffer_index = 0;
    }

    GeomContext *gc = ((Geom *)geom_reader->get_object())->prepare_now(get_prepared_objects(), this);
    nassertr(gc != (GeomContext *)NULL, false);
    CLP(GeomContext) *ggc = DCAST(CLP(GeomContext), gc);
    const CLP(GeomMunger) *gmunger = DCAST(CLP(GeomMunger), _munger);
    UpdateSeq modified = max(geom_reader->get_modified(), _data_reader->get_modified());
    if (ggc->get_display_list(_geom_display_list, gmunger, modified)) {
      // If it hasn't been modified, just play the display list again.
      if (GLCAT.is_spam()) {
        GLCAT.spam()
          << "calling display list " << _geom_display_list << "\n";
      }

      GLP(CallList)(_geom_display_list);
#ifdef DO_PSTATS
      _vertices_display_list_pcollector.add_level(ggc->_num_verts);
      _primitive_batches_display_list_pcollector.add_level(1);
#endif

      // And now we don't need to do anything else for this geom.
      _geom_display_list = 0;
      end_draw_primitives();
      return false;
    }

    // Since we start this collector explicitly, we have to be sure to
    // stop it again.
    _load_display_list_pcollector.start();

    if (GLCAT.is_debug()) {
      GLCAT.debug()
        << "compiling display list " << _geom_display_list << "\n";
    }

    // If it has been modified, or this is the first time, then we
    // need to build the display list up.
    if (CLP(compile_and_execute)) {
      GLP(NewList)(_geom_display_list, GL_COMPILE_AND_EXECUTE);
    } else {
      GLP(NewList)(_geom_display_list, GL_COMPILE);
    }

#ifdef DO_PSTATS
    // Count up the number of vertices used by primitives in the Geom,
    // for PStats reporting.
    ggc->_num_verts = 0;
    for (int i = 0; i < geom_reader->get_num_primitives(); i++) {
      ggc->_num_verts += geom_reader->get_primitive(i)->get_num_vertices();
    }
#endif
  }

  // Enable the appropriate vertex arrays, and disable any
  // extra vertex arrays used by the previous rendering mode.
#ifdef SUPPORT_IMMEDIATE_MODE
  _use_sender = !vertex_arrays;
  // This is a workaround for a bug in Mesa 6 vertex array handling.
  if ((_supports_mesa_6) && (_current_properties->get_force_software())) {
    _use_sender = true;
  }
#endif
  if (_vertex_array_shader_context==0) {
    if (_current_shader_context==0) {
      if (!update_standard_vertex_arrays(force)) {
        return false;
      }
    } else {
      disable_standard_vertex_arrays();
      if (!_current_shader_context->update_shader_vertex_arrays(NULL, this, force)) {
        return false;
      }
    }
  } else {
    if (_current_shader_context==0) {
      _vertex_array_shader_context->disable_shader_vertex_arrays(this);
      if (!update_standard_vertex_arrays(force)) {
        return false;
      }
    } else {
      if (!_current_shader_context->
          update_shader_vertex_arrays(_vertex_array_shader_context, this, force)) {
        return false;
      }
    }
  }
  _vertex_array_shader = _current_shader;
  _vertex_array_shader_context = _current_shader_context;

  report_my_gl_errors();
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::update_standard_vertex_arrays
//       Access: Protected
//  Description: Disables any unneeded vertex arrays that
//               were previously enabled, and enables any vertex
//               arrays that are needed that were not previously
//               enabled (or, sets up an immediate-mode sender).
//               Called only from begin_draw_primitives.
//               Used only when the standard (non-shader) pipeline
//               is about to be used - glShaderContexts are responsible
//               for setting up their own vertex arrays.
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
update_standard_vertex_arrays(bool force) {
  const GeomVertexAnimationSpec &animation =
    _data_reader->get_format()->get_animation();
  bool hardware_animation = (animation.get_animation_type() == Geom::AT_hardware);
#ifdef SUPPORT_IMMEDIATE_MODE
  if (_use_sender) {
    // We must use immediate mode to render primitives.
    _sender.clear();

    _sender.add_column(_data_reader, InternalName::get_normal(),
                       NULL, NULL, GLP(Normal3f), NULL);
    if (!_sender.add_column(_data_reader, InternalName::get_color(),
                            NULL, NULL, GLP(Color3f), GLP(Color4f))) {
      // If we didn't have a color column, the item color is white.
      GLP(Color4f)(1.0f, 1.0f, 1.0f, 1.0f);
    }

    // Now set up each of the active texture coordinate stages--or at
    // least those for which we're not generating texture coordinates
    // automatically.
    int max_stage_index = _effective_texture->get_num_on_ff_stages();
    int stage_index = 0;
    while (stage_index < max_stage_index) {
      TextureStage *stage = _effective_texture->get_on_ff_stage(stage_index);
      if (!_effective_tex_gen->has_gen_texcoord_stage(stage)) {
        // This stage is not one of the stages that doesn't need
        // texcoords issued for it.
        const InternalName *name = stage->get_texcoord_name();
        if (stage_index == 0) {
          // Use the original functions for stage 0, in case we don't
          // support multitexture.
          _sender.add_column(_data_reader, name,
                             GLP(TexCoord1f), GLP(TexCoord2f),
                             GLP(TexCoord3f), GLP(TexCoord4f));

        } else {
          // Other stages require the multitexture functions.
          _sender.add_texcoord_column(_data_reader, name, stage_index,
                                      _glMultiTexCoord1f, _glMultiTexCoord2f,
                                      _glMultiTexCoord3f, _glMultiTexCoord4f);
        }
      }

      ++stage_index;
    }

    // Be sure also to disable any texture stages we had enabled before.
    while (stage_index < _last_max_stage_index) {
      _glClientActiveTexture(GL_TEXTURE0 + stage_index);
      GLP(DisableClientState)(GL_TEXTURE_COORD_ARRAY);
      ++stage_index;
    }
    _last_max_stage_index = max_stage_index;

    if (_supports_vertex_blend) {
      if (hardware_animation) {
        // Issue the weights and/or transform indices for vertex blending.
        _sender.add_vector_column(_data_reader, InternalName::get_transform_weight(),
                                  _glWeightfvARB);

        if (animation.get_indexed_transforms()) {
          // Issue the matrix palette indices.
          _sender.add_vector_uint_column(_data_reader, InternalName::get_transform_index(),
                                         _glMatrixIndexuivARB);
        }
      }
    }

    // We must add vertex last, because glVertex3f() is the key
    // function call that actually issues the vertex.
    _sender.add_column(_data_reader, InternalName::get_vertex(),
                       NULL, GLP(Vertex2f), GLP(Vertex3f), GLP(Vertex4f));

  } else
#endif  // SUPPORT_IMMEDIATE_MODE
  {
    // We may use vertex arrays or buffers to render primitives.
    const GeomVertexArrayDataHandle *array_reader;
    const unsigned char *client_pointer;
    int num_values;
    Geom::NumericType numeric_type;
    int start;
    int stride;
    
    if (_data_reader->get_normal_info(array_reader, numeric_type,
                                      start, stride)) {
      if (!setup_array_data(client_pointer, array_reader, force)) {
        return false;
      }
      GLP(NormalPointer)(get_numeric_type(numeric_type), stride,
                         client_pointer + start);
      GLP(EnableClientState)(GL_NORMAL_ARRAY);
    } else {
      GLP(DisableClientState)(GL_NORMAL_ARRAY);
    }
    
    if (_data_reader->get_color_info(array_reader, num_values, numeric_type,
                                     start, stride) &&
        numeric_type != Geom::NT_packed_dabc) {
      if (!setup_array_data(client_pointer, array_reader, force)) {
        return false;
      }
      GLP(ColorPointer)(num_values, get_numeric_type(numeric_type),
                        stride, client_pointer + start);
      GLP(EnableClientState)(GL_COLOR_ARRAY);
    } else {
      GLP(DisableClientState)(GL_COLOR_ARRAY);
      
      // Since we don't have per-vertex color, the implicit color is
      // white.
      GLP(Color4f)(1.0f, 1.0f, 1.0f, 1.0f);
    }
    
    // Now set up each of the active texture coordinate stages--or at
    // least those for which we're not generating texture coordinates
    // automatically.
    int max_stage_index = _effective_texture->get_num_on_ff_stages();
    int stage_index = 0;
    while (stage_index < max_stage_index) {
      _glClientActiveTexture(GL_TEXTURE0 + stage_index);
      TextureStage *stage = _effective_texture->get_on_ff_stage(stage_index);
      if (!_effective_tex_gen->has_gen_texcoord_stage(stage)) {
        // This stage is not one of the stages that doesn't need
        // texcoords issued for it.
        const InternalName *name = stage->get_texcoord_name();
        
        if (_data_reader->get_array_info(name, array_reader, num_values,
                                         numeric_type, start, stride)) {
          // The vertex data does have texcoords for this stage.
          if (!setup_array_data(client_pointer, array_reader, force)) {
            return false;
          }
          GLP(TexCoordPointer)(num_values, get_numeric_type(numeric_type),
                               stride, client_pointer + start);
          GLP(EnableClientState)(GL_TEXTURE_COORD_ARRAY);
          
        } else {
          // The vertex data doesn't have texcoords for this stage (even
          // though they're needed).
          GLP(DisableClientState)(GL_TEXTURE_COORD_ARRAY);
        }
      } else {
        // No texcoords are needed for this stage.
        GLP(DisableClientState)(GL_TEXTURE_COORD_ARRAY);
      }
      
      ++stage_index;
    }
    
    // Be sure also to disable any texture stages we had enabled before.
    while (stage_index < _last_max_stage_index) {
      _glClientActiveTexture(GL_TEXTURE0 + stage_index);
      GLP(DisableClientState)(GL_TEXTURE_COORD_ARRAY);
      ++stage_index;
    }
    _last_max_stage_index = max_stage_index;

    if (_supports_vertex_blend) {
      if (hardware_animation) {
        // Issue the weights and/or transform indices for vertex blending.
        if (_data_reader->get_array_info(InternalName::get_transform_weight(),
                                         array_reader, num_values, numeric_type,
                                         start, stride)) {
          if (!setup_array_data(client_pointer, array_reader, force)) {
            return false;
          }
          _glWeightPointerARB(num_values, get_numeric_type(numeric_type),
                              stride, client_pointer + start);
          GLP(EnableClientState)(GL_WEIGHT_ARRAY_ARB);
        } else {
          GLP(DisableClientState)(GL_WEIGHT_ARRAY_ARB);
        }

        if (animation.get_indexed_transforms()) {
          // Issue the matrix palette indices.
          if (_data_reader->get_array_info(InternalName::get_transform_index(),
                                           array_reader, num_values, numeric_type,
                                           start, stride)) {
            if (!setup_array_data(client_pointer, array_reader, force)) {
              return false;
            }
            _glMatrixIndexPointerARB(num_values, get_numeric_type(numeric_type),
                                     stride, client_pointer + start);
            GLP(EnableClientState)(GL_MATRIX_INDEX_ARRAY_ARB);
          } else {
            GLP(DisableClientState)(GL_MATRIX_INDEX_ARRAY_ARB);
          }
        }

      } else {
        GLP(DisableClientState)(GL_WEIGHT_ARRAY_ARB);
        if (_supports_matrix_palette) {
          GLP(DisableClientState)(GL_MATRIX_INDEX_ARRAY_ARB);
        }
      }
    }

    // There's no requirement that we add vertices last, but we do
    // anyway.
    if (_data_reader->get_vertex_info(array_reader, num_values, numeric_type,
                                      start, stride)) {
      if (!setup_array_data(client_pointer, array_reader, force)) {
        return false;
      }
      GLP(VertexPointer)(num_values, get_numeric_type(numeric_type),
                         stride, client_pointer + start);
      GLP(EnableClientState)(GL_VERTEX_ARRAY);
    }
  }
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::disable_standard_vertex_arrays
//       Access: Protected
//  Description: Used to disable all the standard vertex arrays that
//               are currently enabled.  glShaderContexts are
//               responsible for setting up their own vertex arrays,
//               but before they can do so, the standard vertex
//               arrays need to be disabled to get them "out of the
//               way."  Called only from begin_draw_primitives.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
disable_standard_vertex_arrays()
{
#ifdef SUPPORT_IMMEDIATE_MODE
  if (_use_sender) return;
#endif

  GLP(DisableClientState)(GL_NORMAL_ARRAY);
  GLP(DisableClientState)(GL_COLOR_ARRAY);
  GLP(Color4f)(1.0f, 1.0f, 1.0f, 1.0f);
  report_my_gl_errors();

  for (int stage_index=0; stage_index < _last_max_stage_index; stage_index++) {
    _glClientActiveTexture(GL_TEXTURE0 + stage_index);
    GLP(DisableClientState)(GL_TEXTURE_COORD_ARRAY);
  }
  _last_max_stage_index = 0;
  report_my_gl_errors();

  if (_supports_vertex_blend) {
    GLP(DisableClientState)(GL_WEIGHT_ARRAY_ARB);
    if (_supports_matrix_palette) {
      GLP(DisableClientState)(GL_MATRIX_INDEX_ARRAY_ARB);
    }
  }

  GLP(DisableClientState)(GL_VERTEX_ARRAY);
  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::draw_triangles
//       Access: Public, Virtual
//  Description: Draws a series of disconnected triangles.
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
draw_triangles(const GeomPrimitivePipelineReader *reader, bool force) {
  PStatTimer timer(_draw_primitive_pcollector, reader->get_current_thread());

#ifndef NDEBUG
  if (GLCAT.is_spam()) {
    GLCAT.spam() << "draw_triangles: " << *(reader->get_object()) << "\n";
  }
#endif  // NDEBUG

#ifdef SUPPORT_IMMEDIATE_MODE
  if (_use_sender) {
    draw_immediate_simple_primitives(reader, GL_TRIANGLES);

  } else
#endif  // SUPPORT_IMMEDIATE_MODE
  {
    int num_vertices = reader->get_num_vertices();
    _vertices_tri_pcollector.add_level(num_vertices);
    _primitive_batches_tri_pcollector.add_level(1);

    if (reader->is_indexed()) {
      const unsigned char *client_pointer;
      if (!setup_primitive(client_pointer, reader, force)) {
        return false;
      }

      _glDrawRangeElements(GL_TRIANGLES,
                           reader->get_min_vertex(),
                           reader->get_max_vertex(),
                           num_vertices,
                           get_numeric_type(reader->get_index_type()),
                           client_pointer);
    } else {
      GLP(DrawArrays)(GL_TRIANGLES,
                      reader->get_first_vertex(),
                      num_vertices);
    }
  }

  report_my_gl_errors();
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::draw_tristrips
//       Access: Public, Virtual
//  Description: Draws a series of triangle strips.
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
draw_tristrips(const GeomPrimitivePipelineReader *reader, bool force) {
  PStatTimer timer(_draw_primitive_pcollector, reader->get_current_thread());

  report_my_gl_errors();

#ifndef NDEBUG
  if (GLCAT.is_spam()) {
    GLCAT.spam() << "draw_tristrips: " << *(reader->get_object()) << "\n";
  }
#endif  // NDEBUG

#ifdef SUPPORT_IMMEDIATE_MODE
  if (_use_sender) {
    draw_immediate_composite_primitives(reader, GL_TRIANGLE_STRIP);

  } else
#endif  // SUPPORT_IMMEDIATE_MODE
  {
    if (connect_triangle_strips && _render_mode != RenderModeAttrib::M_wireframe) {
      // One long triangle strip, connected by the degenerate vertices
      // that have already been set up within the primitive.
      int num_vertices = reader->get_num_vertices();
      _vertices_tristrip_pcollector.add_level(num_vertices);
      _primitive_batches_tristrip_pcollector.add_level(1);
      if (reader->is_indexed()) {
        const unsigned char *client_pointer;
        if (!setup_primitive(client_pointer, reader, force)) {
          return false;
        }
        _glDrawRangeElements(GL_TRIANGLE_STRIP,
                             reader->get_min_vertex(),
                             reader->get_max_vertex(),
                             num_vertices,
                             get_numeric_type(reader->get_index_type()),
                             client_pointer);
      } else {
        GLP(DrawArrays)(GL_TRIANGLE_STRIP,
                        reader->get_first_vertex(),
                        num_vertices);
      }

    } else {
      // Send the individual triangle strips, stepping over the
      // degenerate vertices.
      CPTA_int ends = reader->get_ends();

      _primitive_batches_tristrip_pcollector.add_level(ends.size());
      if (reader->is_indexed()) {
        const unsigned char *client_pointer;
        if (!setup_primitive(client_pointer, reader, force)) {
          return false;
        }
        int index_stride = reader->get_index_stride();
        GeomVertexReader mins(reader->get_mins(), 0);
        GeomVertexReader maxs(reader->get_maxs(), 0);
        nassertr(reader->get_mins()->get_num_rows() == (int)ends.size() &&
                 reader->get_maxs()->get_num_rows() == (int)ends.size(), false);

        unsigned int start = 0;
        for (size_t i = 0; i < ends.size(); i++) {
          _vertices_tristrip_pcollector.add_level(ends[i] - start);
          _glDrawRangeElements(GL_TRIANGLE_STRIP,
                               mins.get_data1i(), maxs.get_data1i(),
                               ends[i] - start,
                               get_numeric_type(reader->get_index_type()),
                               client_pointer + start * index_stride);
          start = ends[i] + 2;
        }
      } else {
        unsigned int start = 0;
        int first_vertex = reader->get_first_vertex();
        for (size_t i = 0; i < ends.size(); i++) {
          _vertices_tristrip_pcollector.add_level(ends[i] - start);
          GLP(DrawArrays)(GL_TRIANGLE_STRIP, first_vertex + start,
                          ends[i] - start);
          start = ends[i] + 2;
        }
      }
    }
  }

  report_my_gl_errors();
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::draw_trifans
//       Access: Public, Virtual
//  Description: Draws a series of triangle fans.
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
draw_trifans(const GeomPrimitivePipelineReader *reader, bool force) {
  PStatTimer timer(_draw_primitive_pcollector, reader->get_current_thread());
#ifndef NDEBUG
  if (GLCAT.is_spam()) {
    GLCAT.spam() << "draw_trifans: " << *(reader->get_object()) << "\n";
  }
#endif  // NDEBUG

#ifdef SUPPORT_IMMEDIATE_MODE
  if (_use_sender) {
    draw_immediate_composite_primitives(reader, GL_TRIANGLE_FAN);
  } else
#endif  // SUPPORT_IMMEDIATE_MODE
  {
    // Send the individual triangle fans.  There's no connecting fans
    // with degenerate vertices, so no worries about that.
    CPTA_int ends = reader->get_ends();

    _primitive_batches_trifan_pcollector.add_level(ends.size());
    if (reader->is_indexed()) {
      const unsigned char *client_pointer;
      if (!setup_primitive(client_pointer, reader, force)) {
        return false;
      }
      int index_stride = reader->get_index_stride();
      GeomVertexReader mins(reader->get_mins(), 0);
      GeomVertexReader maxs(reader->get_maxs(), 0);
      nassertr(reader->get_mins()->get_num_rows() == (int)ends.size() &&
               reader->get_maxs()->get_num_rows() == (int)ends.size(), false);

      unsigned int start = 0;
      for (size_t i = 0; i < ends.size(); i++) {
        _vertices_trifan_pcollector.add_level(ends[i] - start);
        _glDrawRangeElements(GL_TRIANGLE_FAN,
                             mins.get_data1i(), maxs.get_data1i(), ends[i] - start,
                             get_numeric_type(reader->get_index_type()),
                             client_pointer + start * index_stride);
        start = ends[i];
      }
    } else {
      unsigned int start = 0;
      int first_vertex = reader->get_first_vertex();
      for (size_t i = 0; i < ends.size(); i++) {
        _vertices_trifan_pcollector.add_level(ends[i] - start);
        GLP(DrawArrays)(GL_TRIANGLE_FAN, first_vertex + start,
                        ends[i] - start);
        start = ends[i];
      }
    }
  }

  report_my_gl_errors();
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::draw_lines
//       Access: Public, Virtual
//  Description: Draws a series of disconnected line segments.
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
draw_lines(const GeomPrimitivePipelineReader *reader, bool force) {
  PStatTimer timer(_draw_primitive_pcollector, reader->get_current_thread());
#ifndef NDEBUG
  if (GLCAT.is_spam()) {
    GLCAT.spam() << "draw_lines: " << *(reader->get_object()) << "\n";
  }
#endif  // NDEBUG

#ifdef SUPPORT_IMMEDIATE_MODE
  if (_use_sender) {
    draw_immediate_simple_primitives(reader, GL_LINES);
  } else
#endif  // SUPPORT_IMMEDIATE_MODE
  {
    int num_vertices = reader->get_num_vertices();
    _vertices_other_pcollector.add_level(num_vertices);
    _primitive_batches_other_pcollector.add_level(1);

    if (reader->is_indexed()) {
      const unsigned char *client_pointer;
      if (!setup_primitive(client_pointer, reader, force)) {
        return false;
      }
      _glDrawRangeElements(GL_LINES,
                           reader->get_min_vertex(),
                           reader->get_max_vertex(),
                           num_vertices,
                           get_numeric_type(reader->get_index_type()),
                           client_pointer);
    } else {
      GLP(DrawArrays)(GL_LINES,
                      reader->get_first_vertex(),
                      num_vertices);
    }
  }

  report_my_gl_errors();
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::draw_linestrips
//       Access: Public, Virtual
//  Description: Draws a series of line strips.
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
draw_linestrips(const GeomPrimitivePipelineReader *reader, bool force) {
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::draw_points
//       Access: Public, Virtual
//  Description: Draws a series of disconnected points.
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
draw_points(const GeomPrimitivePipelineReader *reader, bool force) {
  PStatTimer timer(_draw_primitive_pcollector, reader->get_current_thread());
#ifndef NDEBUG
  if (GLCAT.is_spam()) {
    GLCAT.spam() << "draw_points: " << *(reader->get_object()) << "\n";
  }
#endif  // NDEBUG

#ifdef SUPPORT_IMMEDIATE_MODE
  if (_use_sender) {
    draw_immediate_simple_primitives(reader, GL_POINTS);
  } else
#endif  // SUPPORT_IMMEDIATE_MODE
  {
    int num_vertices = reader->get_num_vertices();
    _vertices_other_pcollector.add_level(num_vertices);
    _primitive_batches_other_pcollector.add_level(1);

    if (reader->is_indexed()) {
      const unsigned char *client_pointer;
      if (!setup_primitive(client_pointer, reader, force)) {
        return false;
      }
      _glDrawRangeElements(GL_POINTS,
                           reader->get_min_vertex(),
                           reader->get_max_vertex(),
                           num_vertices,
                           get_numeric_type(reader->get_index_type()),
                           client_pointer);
    } else {
      GLP(DrawArrays)(GL_POINTS,
                      reader->get_first_vertex(),
                      num_vertices);
    }
  }

  report_my_gl_errors();
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::end_draw_primitives()
//       Access: Public, Virtual
//  Description: Called after a sequence of draw_primitive()
//               functions are called, this should do whatever cleanup
//               is appropriate.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
end_draw_primitives() {
  if (_geom_display_list != 0) {
    // If we were building a display list, close it now.
    GLP(EndList)();
    _load_display_list_pcollector.stop();

    if (!CLP(compile_and_execute)) {
      GLP(CallList)(_geom_display_list);
    }
    _primitive_batches_display_list_pcollector.add_level(1);
  }
  _geom_display_list = 0;

  // Clean up the vertex blending state.
  if (_vertex_blending_enabled) {
    GLP(Disable)(GL_VERTEX_BLEND_ARB);
    if (_supports_matrix_palette) {
      GLP(Disable)(GL_MATRIX_PALETTE_ARB);
    }
    _vertex_blending_enabled = false;
  }

  if (_transform_stale) {
    GLP(MatrixMode)(GL_MODELVIEW);
    GLP(LoadMatrixf)(_internal_transform->get_mat().get_data());
  }

  if (_data_reader->is_vertex_transformed()) {
    // Restore the matrices that we pushed above.
    GLP(MatrixMode)(GL_PROJECTION);
    GLP(PopMatrix)();
    GLP(MatrixMode)(GL_MODELVIEW);
    GLP(PopMatrix)();
  }

  GraphicsStateGuardian::end_draw_primitives();
  maybe_gl_finish();
  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::prepare_texture
//       Access: Public, Virtual
//  Description: Creates whatever structures the GSG requires to
//               represent the texture internally, and returns a
//               newly-allocated TextureContext object with this data.
//               It is the responsibility of the calling function to
//               later call release_texture() with this same pointer
//               (which will also delete the pointer).
//
//               This function should not be called directly to
//               prepare a texture.  Instead, call Texture::prepare().
////////////////////////////////////////////////////////////////////
TextureContext *CLP(GraphicsStateGuardian)::
prepare_texture(Texture *tex) {
  // Make sure we'll support this texture when it's rendered.  Don't
  // bother to prepare it if we won't.
  switch (tex->get_texture_type()) {
  case Texture::TT_3d_texture:
    if (!_supports_3d_texture) {
      GLCAT.warning()
        << "3-D textures are not supported by this OpenGL driver.\n";
      return NULL;
    }
    break;

  case Texture::TT_cube_map:
    if (!_supports_cube_map) {
      GLCAT.warning()
        << "Cube map textures are not supported by this OpenGL driver.\n";
      return NULL;
    }

  default:
    break;
  }

  CLP(TextureContext) *gtc = new CLP(TextureContext)(_prepared_objects, tex);
  GLP(GenTextures)(1, &gtc->_index);
  report_my_gl_errors();

  apply_texture(gtc);
  return gtc;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::release_texture
//       Access: Public, Virtual
//  Description: Frees the GL resources previously allocated for the
//               texture.  This function should never be called
//               directly; instead, call Texture::release() (or simply
//               let the Texture destruct).
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
release_texture(TextureContext *tc) {
  report_my_gl_errors();
  CLP(TextureContext) *gtc = DCAST(CLP(TextureContext), tc);

  GLP(DeleteTextures)(1, &gtc->_index);
  report_my_gl_errors();

  gtc->_index = 0;
  delete gtc;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::extract_texture_data
//       Access: Public, Virtual
//  Description: This method should only be called by the
//               GraphicsEngine.  Do not call it directly; call
//               GraphicsEngine::extract_texture_data() instead.
//
//               This method will be called in the draw thread to
//               download the texture memory's image into its
//               ram_image value.  It returns true on success, false
//               otherwise.
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
extract_texture_data(Texture *tex) {
  // Make sure the error stack is cleared out before we begin.
  report_my_gl_errors();

  TextureContext *tc = tex->prepare_now(get_prepared_objects(), this);
  nassertr(tc != (TextureContext *)NULL, false);
  CLP(TextureContext) *gtc = DCAST(CLP(TextureContext), tc);
  GLenum target = get_texture_target(tex->get_texture_type());
  GLP(BindTexture)(target, gtc->_index);

  report_my_gl_errors();

  GLint wrap_u, wrap_v, wrap_w;
  GLint minfilter, magfilter;
  GLfloat border_color[4];

  GLP(GetTexParameteriv)(target, GL_TEXTURE_WRAP_S, &wrap_u);
  GLP(GetTexParameteriv)(target, GL_TEXTURE_WRAP_T, &wrap_v);
  wrap_w = GL_REPEAT;
  if (_supports_3d_texture) {
    GLP(GetTexParameteriv)(target, GL_TEXTURE_WRAP_R, &wrap_w);
  }
  GLP(GetTexParameteriv)(target, GL_TEXTURE_MIN_FILTER, &minfilter);

  // Mesa has a bug querying this property.
  magfilter = GL_LINEAR;
  //  GLP(GetTexParameteriv)(target, GL_TEXTURE_MAG_FILTER, &magfilter);

  GLP(GetTexParameterfv)(target, GL_TEXTURE_BORDER_COLOR, border_color);

  GLenum page_target = target;
  if (target == GL_TEXTURE_CUBE_MAP) {
    // We need a particular page to get the level parameter from.
    page_target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
  }

  GLint width = 1, height = 1, depth = 1;
  GLP(GetTexLevelParameteriv)(page_target, 0, GL_TEXTURE_WIDTH, &width);
  GLP(GetTexLevelParameteriv)(page_target, 0, GL_TEXTURE_HEIGHT, &height);
  if (_supports_3d_texture) {
    GLP(GetTexLevelParameteriv)(page_target, 0, GL_TEXTURE_DEPTH, &depth);
  }
  report_my_gl_errors();

  GLint internal_format;
  GLP(GetTexLevelParameteriv)(page_target, 0, GL_TEXTURE_INTERNAL_FORMAT, &internal_format);

  // Make sure we were able to query those parameters properly.
  GLenum error_code = GLP(GetError)();
  if (error_code != GL_NO_ERROR) {
    GLCAT.error()
      << "Unable to query texture parameters for " << tex->get_name()
      << " : " << get_error_string(error_code) << "\n";

    return false;
  }

  Texture::ComponentType type = Texture::T_unsigned_byte;
  Texture::Format format = Texture::F_rgb;
  Texture::CompressionMode compression = Texture::CM_off;

  switch (internal_format) {
  case GL_COLOR_INDEX:
    format = Texture::F_color_index;
    break;
  case GL_DEPTH_COMPONENT:
  case GL_DEPTH_STENCIL_EXT:
    type = Texture::T_float;
    format = Texture::F_depth_stencil;
    break;
  case GL_RGBA:
    format = Texture::F_rgba;
    break;
  case GL_RGBA4:
    format = Texture::F_rgba4;
    break;
  case GL_RGBA8:
    format = Texture::F_rgba8;
    break;
  case GL_RGBA12:
    type = Texture::T_unsigned_short;
    format = Texture::F_rgba12;
    break;

  case GL_RGB:
    format = Texture::F_rgb;
    break;
  case GL_RGB5:
    format = Texture::F_rgb5;
    break;
  case GL_RGB5_A1:
    format = Texture::F_rgba5;
    break;
  case GL_RGB8:
    format = Texture::F_rgb8;
    break;
  case GL_RGB12:
    format = Texture::F_rgb12;
    break;
  case GL_R3_G3_B2:
    format = Texture::F_rgb332;

  case GL_RED:
    format = Texture::F_red;
    break;
  case GL_GREEN:
    format = Texture::F_green;
    break;
  case GL_BLUE:
    format = Texture::F_blue;
    break;
  case GL_ALPHA:
    format = Texture::F_alpha;
    break;
  case GL_LUMINANCE:
    format = Texture::F_luminance;
    break;
  case GL_LUMINANCE_ALPHA:
    format = Texture::F_luminance_alpha;
    break;

  case GL_COMPRESSED_RGB:
    format = Texture::F_rgb;
    compression = Texture::CM_on;
    break;
  case GL_COMPRESSED_RGBA:
    format = Texture::F_rgba;
    compression = Texture::CM_on;
    break;
  case GL_COMPRESSED_ALPHA:
    format = Texture::F_alpha;
    compression = Texture::CM_on;
    break;
  case GL_COMPRESSED_LUMINANCE:
    format = Texture::F_luminance;
    compression = Texture::CM_on;
    break;
  case GL_COMPRESSED_LUMINANCE_ALPHA:
    format = Texture::F_luminance_alpha;
    compression = Texture::CM_on;
    break;

  case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
    format = Texture::F_rgb;
    compression = Texture::CM_dxt1;
    break;
  case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    format = Texture::F_rgbm;
    compression = Texture::CM_dxt1;
    break;
  case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    format = Texture::F_rgba;
    compression = Texture::CM_dxt3;
    break;
  case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
    format = Texture::F_rgba;
    compression = Texture::CM_dxt5;
    break;
  case GL_COMPRESSED_RGB_FXT1_3DFX:
    format = Texture::F_rgb;
    compression = Texture::CM_fxt1;
    break;
  case GL_COMPRESSED_RGBA_FXT1_3DFX:
    format = Texture::F_rgba;
    compression = Texture::CM_fxt1;
    break;
  }

  /*
  switch (target) {
  case GL_TEXTURE_1D:
    tex->setup_1d_texture(width, type, format);
    break;
    
  case GL_TEXTURE_2D:
    tex->setup_2d_texture(width, height, type, format);
    break;
    
  case GL_TEXTURE_3D:
    tex->setup_3d_texture(width, height, depth, type, format);
    break;
    
  case GL_TEXTURE_CUBE_MAP:
    tex->setup_cube_map(width, type, format);
    break;
  }
  */

  tex->set_wrap_u(get_panda_wrap_mode(wrap_u));
  tex->set_wrap_v(get_panda_wrap_mode(wrap_v));
  tex->set_wrap_w(get_panda_wrap_mode(wrap_w));
  tex->set_border_color(Colorf(border_color[0], border_color[1],
                               border_color[2], border_color[3]));

  tex->set_minfilter(get_panda_filter_type(minfilter));
  //  tex->set_magfilter(get_panda_filter_type(magfilter));

  PTA_uchar image;
  size_t page_size = 0;

  if (!extract_texture_image(image, page_size, tex, target, page_target,
                             type, compression, 0)) {
    return false;
  }

  tex->set_ram_image(image, compression, page_size);

  if (tex->uses_mipmaps()) {
    // Also get the mipmap levels.
    GLint num_expected_levels = tex->get_expected_num_mipmap_levels();
    GLint highest_level = num_expected_levels;
    if (is_at_least_version(1, 2)) {
      GLP(GetTexParameteriv)(target, GL_TEXTURE_MAX_LEVEL, &highest_level);
      highest_level = min(highest_level, num_expected_levels);
    }
    for (int n = 1; n <= highest_level; ++n) {
      if (!extract_texture_image(image, page_size, tex, target, page_target,
                                 type, compression, n)) {
        return false;
      }
      tex->set_ram_mipmap_image(n, image, page_size);
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::prepare_geom
//       Access: Public, Virtual
//  Description: Creates a new retained-mode representation of the
//               given geom, and returns a newly-allocated
//               GeomContext pointer to reference it.  It is the
//               responsibility of the calling function to later
//               call release_geom() with this same pointer (which
//               will also delete the pointer).
//
//               This function should not be called directly to
//               prepare a geom.  Instead, call Geom::prepare().
////////////////////////////////////////////////////////////////////
GeomContext *CLP(GraphicsStateGuardian)::
prepare_geom(Geom *geom) {
  return new CLP(GeomContext)(geom);
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::release_geom
//       Access: Public, Virtual
//  Description: Frees the GL resources previously allocated for the
//               geom.  This function should never be called
//               directly; instead, call Geom::release() (or simply
//               let the Geom destruct).
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
release_geom(GeomContext *gc) {
  CLP(GeomContext) *ggc = DCAST(CLP(GeomContext), gc);
  ggc->release_display_lists();
  report_my_gl_errors();

  delete ggc;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::prepare_shader
//       Access: Public, Virtual
//  Description: yadda.
////////////////////////////////////////////////////////////////////
ShaderContext *CLP(GraphicsStateGuardian)::
prepare_shader(Shader *se) {
  CLP(ShaderContext) *result = new CLP(ShaderContext)(se, this);
  if (result->valid()) return result;
  delete result;
  return NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::release_shader
//       Access: Public, Virtual
//  Description: yadda.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
release_shader(ShaderContext *sc) {
  CLP(ShaderContext) *gsc = DCAST(CLP(ShaderContext), sc);
  delete gsc;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::record_deleted_display_list
//       Access: Public
//  Description: This is intended to be called only from the
//               GLGeomContext destructor.  It saves the indicated
//               display list index in the list to be deleted at the
//               end of the frame.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
record_deleted_display_list(GLuint index) {
  MutexHolder holder(_lock);
  _deleted_display_lists.push_back(index);
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::prepare_vertex_buffer
//       Access: Public, Virtual
//  Description: Creates a new retained-mode representation of the
//               given data, and returns a newly-allocated
//               VertexBufferContext pointer to reference it.  It is the
//               responsibility of the calling function to later
//               call release_vertex_buffer() with this same pointer (which
//               will also delete the pointer).
//
//               This function should not be called directly to
//               prepare a buffer.  Instead, call Geom::prepare().
////////////////////////////////////////////////////////////////////
VertexBufferContext *CLP(GraphicsStateGuardian)::
prepare_vertex_buffer(GeomVertexArrayData *data) {
  if (_supports_buffers) {
    CLP(VertexBufferContext) *gvbc = new CLP(VertexBufferContext)(_prepared_objects, data);
    _glGenBuffers(1, &gvbc->_index);

    if (GLCAT.is_debug() && CLP(debug_buffers)) {
      GLCAT.debug()
        << "creating vertex buffer " << gvbc->_index << ": "
        << data->get_num_rows() << " vertices "
        << *data->get_array_format() << "\n";
    }

    report_my_gl_errors();
    return gvbc;
  }

  return NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::apply_vertex_buffer
//       Access: Public
//  Description: Makes the data the currently available data for
//               rendering.
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
apply_vertex_buffer(VertexBufferContext *vbc,
                    const GeomVertexArrayDataHandle *reader, bool force) {
  nassertr(_supports_buffers, false);
  nassertr(reader->get_modified() != UpdateSeq::initial(), false);

  CLP(VertexBufferContext) *gvbc = DCAST(CLP(VertexBufferContext), vbc);

  if (_current_vbuffer_index != gvbc->_index) {
    if (GLCAT.is_debug() && CLP(debug_buffers)) {
      GLCAT.debug()
        << "binding vertex buffer " << gvbc->_index << "\n";
    }
    _glBindBuffer(GL_ARRAY_BUFFER, gvbc->_index);
    _current_vbuffer_index = gvbc->_index;
    gvbc->set_active(true);
  }

  if (gvbc->was_modified(reader)) {
    int num_bytes = reader->get_data_size_bytes();
    if (GLCAT.is_debug() && CLP(debug_buffers)) {
      GLCAT.debug()
        << "copying " << num_bytes
        << " bytes into vertex buffer " << gvbc->_index << "\n";
    }
    if (num_bytes != 0) {
      const unsigned char *client_pointer = reader->get_read_pointer(force);
      if (client_pointer == NULL) {
        return false;
      }

      PStatTimer timer(_load_vertex_buffer_pcollector, reader->get_current_thread());
      if (gvbc->changed_size(reader) || gvbc->changed_usage_hint(reader)) {
        _glBufferData(GL_ARRAY_BUFFER, num_bytes, client_pointer,
                      get_usage(reader->get_usage_hint()));

      } else {
        _glBufferSubData(GL_ARRAY_BUFFER, 0, num_bytes, client_pointer);
      }
      _data_transferred_pcollector.add_level(num_bytes);
    }

    gvbc->mark_loaded(reader);
  }

  maybe_gl_finish();
  report_my_gl_errors();
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::release_vertex_buffer
//       Access: Public, Virtual
//  Description: Frees the GL resources previously allocated for the
//               data.  This function should never be called
//               directly; instead, call Data::release() (or simply
//               let the Data destruct).
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
release_vertex_buffer(VertexBufferContext *vbc) {
  nassertv(_supports_buffers);

  CLP(VertexBufferContext) *gvbc = DCAST(CLP(VertexBufferContext), vbc);

  if (GLCAT.is_debug() && CLP(debug_buffers)) {
    GLCAT.debug()
      << "deleting vertex buffer " << gvbc->_index << "\n";
  }

  // Make sure the buffer is unbound before we delete it.  Not
  // strictly necessary according to the OpenGL spec, but it might
  // help out a flaky driver, and we need to keep our internal state
  // consistent anyway.
  if (_current_vbuffer_index == gvbc->_index) {
    if (GLCAT.is_debug() && CLP(debug_buffers)) {
      GLCAT.debug()
        << "unbinding vertex buffer\n";
    }
    _glBindBuffer(GL_ARRAY_BUFFER, 0);
    _current_vbuffer_index = 0;
  }

  _glDeleteBuffers(1, &gvbc->_index);
  report_my_gl_errors();

  gvbc->_index = 0;

  delete gvbc;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::setup_array_data
//       Access: Public
//  Description: Internal function to bind a buffer object for the
//               indicated data array, if appropriate, or to unbind a
//               buffer object if it should be rendered from client
//               memory.
//
//               If the buffer object is bound, this function sets
//               client_pointer to NULL (representing the start of the
//               buffer object in server memory); if the buffer object
//               is not bound, this function sets client_pointer the
//               pointer to the data array in client memory, that is,
//               the data array passed in.
//
//               If force is not true, the function may return false
//               indicating the data is not currently available.
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
setup_array_data(const unsigned char *&client_pointer,
                 const GeomVertexArrayDataHandle *array_reader,
                 bool force) {
  if (!_supports_buffers) {
    // No support for buffer objects; always render from client.
    client_pointer = array_reader->get_read_pointer(force);
    return (client_pointer != NULL);
  }
  if (!vertex_buffers || _geom_display_list != 0 ||
      array_reader->get_usage_hint() < CLP(min_buffer_usage_hint)) {
    // The array specifies client rendering only, or buffer objects
    // are configured off.
    if (_current_vbuffer_index != 0) {
      if (GLCAT.is_debug() && CLP(debug_buffers)) {
        GLCAT.debug()
          << "unbinding vertex buffer\n";
      }
      _glBindBuffer(GL_ARRAY_BUFFER, 0);
      _current_vbuffer_index = 0;
    }
    client_pointer = array_reader->get_read_pointer(force);
    return (client_pointer != NULL);
  }

  // Prepare the buffer object and bind it.
  VertexBufferContext *vbc = ((GeomVertexArrayData *)array_reader->get_object())->prepare_now(get_prepared_objects(), this);
  nassertr(vbc != (VertexBufferContext *)NULL, false);
  if (!apply_vertex_buffer(vbc, array_reader, force)) {
    return false;
  }

  // NULL is the OpenGL convention for the first byte of the buffer object.
  client_pointer = NULL;
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::prepare_index_buffer
//       Access: Public, Virtual
//  Description: Creates a new retained-mode representation of the
//               given data, and returns a newly-allocated
//               IndexBufferContext pointer to reference it.  It is the
//               responsibility of the calling function to later
//               call release_index_buffer() with this same pointer (which
//               will also delete the pointer).
//
//               This function should not be called directly to
//               prepare a buffer.  Instead, call Geom::prepare().
////////////////////////////////////////////////////////////////////
IndexBufferContext *CLP(GraphicsStateGuardian)::
prepare_index_buffer(GeomPrimitive *data) {
  if (_supports_buffers) {
    CLP(IndexBufferContext) *gibc = new CLP(IndexBufferContext)(_prepared_objects, data);
    _glGenBuffers(1, &gibc->_index);

    if (GLCAT.is_debug() && CLP(debug_buffers)) {
      GLCAT.debug()
        << "creating index buffer " << gibc->_index << ": "
        << data->get_num_vertices() << " indices ("
        << data->get_vertices()->get_array_format()->get_column(0)->get_numeric_type()
        << ")\n";
    }

    report_my_gl_errors();
    return gibc;
  }

  return NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::apply_index_buffer
//       Access: Public
//  Description: Makes the data the currently available data for
//               rendering.
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
apply_index_buffer(IndexBufferContext *ibc,
                   const GeomPrimitivePipelineReader *reader,
                   bool force) {
  nassertr(_supports_buffers, false);
  nassertr(reader->get_modified() != UpdateSeq::initial(), false);

  CLP(IndexBufferContext) *gibc = DCAST(CLP(IndexBufferContext), ibc);

  if (_current_ibuffer_index != gibc->_index) {
    if (GLCAT.is_spam()) {
      GLCAT.spam()
        << "binding index buffer " << gibc->_index << "\n";
    }
    _glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gibc->_index);
    _current_ibuffer_index = gibc->_index;
    gibc->set_active(true);
  }

  if (gibc->was_modified(reader)) {
    int num_bytes = reader->get_data_size_bytes();
    if (GLCAT.is_spam()) {
      GLCAT.spam()
        << "copying " << num_bytes
        << " bytes into index buffer " << gibc->_index << "\n";
    }
    if (num_bytes != 0) {
      const unsigned char *client_pointer = reader->get_read_pointer(force);
      if (client_pointer == NULL) {
        return false;
      }

      PStatTimer timer(_load_index_buffer_pcollector, reader->get_current_thread());
      if (gibc->changed_size(reader) || gibc->changed_usage_hint(reader)) {
        _glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_bytes, client_pointer,
                      get_usage(reader->get_usage_hint()));

      } else {
        _glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, num_bytes,
                         client_pointer);
      }
      _data_transferred_pcollector.add_level(num_bytes);
    }
    gibc->mark_loaded(reader);
  }

  maybe_gl_finish();
  report_my_gl_errors();
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::release_index_buffer
//       Access: Public, Virtual
//  Description: Frees the GL resources previously allocated for the
//               data.  This function should never be called
//               directly; instead, call Data::release() (or simply
//               let the Data destruct).
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
release_index_buffer(IndexBufferContext *ibc) {
  nassertv(_supports_buffers);

  CLP(IndexBufferContext) *gibc = DCAST(CLP(IndexBufferContext), ibc);

  if (GLCAT.is_debug() && CLP(debug_buffers)) {
    GLCAT.debug()
      << "deleting index buffer " << gibc->_index << "\n";
  }

  // Make sure the buffer is unbound before we delete it.  Not
  // strictly necessary according to the OpenGL spec, but it might
  // help out a flaky driver, and we need to keep our internal state
  // consistent anyway.
  if (_current_ibuffer_index == gibc->_index) {
    if (GLCAT.is_spam()) {
      GLCAT.spam()
        << "unbinding index buffer\n";
    }
    _glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    _current_ibuffer_index = 0;
  }

  _glDeleteBuffers(1, &gibc->_index);
  report_my_gl_errors();

  gibc->_index = 0;

  delete gibc;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::setup_primitive
//       Access: Public
//  Description: Internal function to bind a buffer object for the
//               indicated primitive's index list, if appropriate, or
//               to unbind a buffer object if it should be rendered
//               from client memory.
//
//               If the buffer object is bound, this function sets
//               client_pointer to NULL (representing the start of the
//               buffer object in server memory); if the buffer object
//               is not bound, this function sets client_pointer to to
//               the data array in client memory, that is, the data
//               array passed in.
//
//               If force is not true, the function may return false
//               indicating the data is not currently available.
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
setup_primitive(const unsigned char *&client_pointer,
                const GeomPrimitivePipelineReader *reader,
                bool force) {
  if (!_supports_buffers) {
    // No support for buffer objects; always render from client.
    client_pointer = reader->get_read_pointer(force);
    return (client_pointer != NULL);
  }
  if (!vertex_buffers || _geom_display_list != 0 ||
      reader->get_usage_hint() == Geom::UH_client) {
    // The array specifies client rendering only, or buffer objects
    // are configured off.
    if (_current_ibuffer_index != 0) {
      if (GLCAT.is_spam()) {
        GLCAT.spam()
          << "unbinding index buffer\n";
      }
      _glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
      _current_ibuffer_index = 0;
    }
    client_pointer = reader->get_read_pointer(force);
    return (client_pointer != NULL);
  }

  // Prepare the buffer object and bind it.
  IndexBufferContext *ibc = ((GeomPrimitive *)reader->get_object())->prepare_now(get_prepared_objects(), this);
  nassertr(ibc != (IndexBufferContext *)NULL, false);
  if (!apply_index_buffer(ibc, reader, force)) {
    return false;
  }

  // NULL is the OpenGL convention for the first byte of the buffer object.
  client_pointer = NULL;
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::begin_occlusion_query
//       Access: Public, Virtual
//  Description: Begins a new occlusion query.  After this call, you
//               may call begin_draw_primitives() and
//               draw_triangles()/draw_whatever() repeatedly.
//               Eventually, you should call end_occlusion_query()
//               before the end of the frame; that will return a new
//               OcclusionQueryContext object that will tell you how
//               many pixels represented by the bracketed geometry
//               passed the depth test.
//
//               It is not valid to call begin_occlusion_query()
//               between another begin_occlusion_query()
//               .. end_occlusion_query() sequence.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
begin_occlusion_query() {
  nassertv(_supports_occlusion_query);
  nassertv(_current_occlusion_query == (OcclusionQueryContext *)NULL);
  PT(CLP(OcclusionQueryContext)) query = new CLP(OcclusionQueryContext)(this);

  _glGenQueries(1, &query->_index);

  if (GLCAT.is_debug()) {
    GLCAT.debug()
      << "beginning occlusion query index " << query->_index << "\n";
  }

  _glBeginQuery(GL_SAMPLES_PASSED, query->_index);
  _current_occlusion_query = query;
  
  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::end_occlusion_query
//       Access: Public, Virtual
//  Description: Ends a previous call to begin_occlusion_query().
//               This call returns the OcclusionQueryContext object
//               that will (eventually) report the number of pixels
//               that passed the depth test between the call to
//               begin_occlusion_query() and end_occlusion_query().
////////////////////////////////////////////////////////////////////
PT(OcclusionQueryContext) CLP(GraphicsStateGuardian)::
end_occlusion_query() {
  nassertr(_current_occlusion_query != (OcclusionQueryContext *)NULL, NULL);
  PT(OcclusionQueryContext) result = _current_occlusion_query;

  GLuint index = DCAST(CLP(OcclusionQueryContext), result)->_index;
    
  if (GLCAT.is_debug()) {
    GLCAT.debug()
      << "ending occlusion query index " << index << "\n";
  }

  _current_occlusion_query = NULL;
  _glEndQuery(GL_SAMPLES_PASSED);

  // Temporary hack to try working around an apparent driver bug on
  // iMacs.  Occlusion queries sometimes incorrectly report 0 samples,
  // unless we stall the pipe to keep fewer than a certain maximum
  // number of queries pending at once.
  static ConfigVariableInt limit_occlusion_queries("limit-occlusion-queries", 0);
  if (limit_occlusion_queries > 0) {
    if (index > (unsigned int)limit_occlusion_queries) {
      PStatTimer timer(_wait_occlusion_pcollector);
      GLuint result;
      _glGetQueryObjectuiv(index - (unsigned int)limit_occlusion_queries, 
                           GL_QUERY_RESULT, &result);
    }
  }

  report_my_gl_errors();

  return result;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::make_geom_munger
//       Access: Public, Virtual
//  Description: Creates a new GeomMunger object to munge vertices
//               appropriate to this GSG for the indicated state.
////////////////////////////////////////////////////////////////////
PT(GeomMunger) CLP(GraphicsStateGuardian)::
make_geom_munger(const RenderState *state, Thread *current_thread) {
  PT(CLP(GeomMunger)) munger = new CLP(GeomMunger)(this, state);
  return GeomMunger::register_munger(munger, current_thread);
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::framebuffer_copy_to_texture
//       Access: Public, Virtual
//  Description: Copy the pixels within the indicated display
//               region from the framebuffer into texture memory.
//
//               If z > -1, it is the cube map index into which to
//               copy.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
framebuffer_copy_to_texture(Texture *tex, int z, const DisplayRegion *dr,
                            const RenderBuffer &rb) {
  nassertv(tex != NULL && dr != NULL);
  set_read_buffer(rb._buffer_type);
  
  int xo, yo, w, h;
  dr->get_region_pixels(xo, yo, w, h);
  tex->set_size_padded(w, h);

  if (tex->get_compression() == Texture::CM_default) {
    // Unless the user explicitly turned on texture compression, turn
    // it off for the copy-to-texture case.
    tex->set_compression(Texture::CM_off);
  }

  // Sanity check everything.
  if (z >= 0) {
    if (!_supports_cube_map) {
      return;
    }
    nassertv(z < 6);
    nassertv(tex->get_texture_type() == Texture::TT_cube_map);
    if ((w != tex->get_x_size()) ||
        (h != tex->get_y_size()) ||
        (w != h)) {
      return;
    }
  } else {
    nassertv(tex->get_texture_type() == Texture::TT_2d_texture);
  }

  // Match framebuffer format if necessary.
  if (tex->get_match_framebuffer_format()) {

    switch (tex->get_format()) {
    case Texture::F_depth_stencil:
      // If the texture is one of these special formats, we don't want
      // to adapt it to the framebuffer's color format.
      break;

    default:
      if (_current_properties->get_alpha_bits()) {
        tex->set_format(Texture::F_rgba);
      } else {
        tex->set_format(Texture::F_rgb);
      }
    }
  }

  TextureContext *tc = tex->prepare_now(get_prepared_objects(), this);
  nassertv(tc != (TextureContext *)NULL);
  apply_texture(tc);

  if (z >= 0) {
    // Copy to a cube map face.  This doesn't seem to work too well
    // with CopyTexSubImage2D, so we always use CopyTexImage2D.
    GLP(CopyTexImage2D)(GL_TEXTURE_CUBE_MAP_POSITIVE_X + z, 0,
                        get_internal_image_format(tex),
                        xo, yo, w, h, 0);
  } else {
    GLP(CopyTexSubImage2D)(GL_TEXTURE_2D, 0, 0, 0, xo, yo, w, h);
  }

  report_my_gl_errors();

  // Force reload of texture state, since we've just monkeyed with it.
  _state_rs = 0;
  _state._texture = 0;
}


////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::framebuffer_copy_to_ram
//       Access: Public, Virtual
//  Description: Copy the pixels within the indicated display region
//               from the framebuffer into system memory, not texture
//               memory.  Returns true on success, false on failure.
//
//               This completely redefines the ram image of the
//               indicated texture.
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
framebuffer_copy_to_ram(Texture *tex, int z, const DisplayRegion *dr,
                        const RenderBuffer &rb) {
  nassertr(tex != NULL && dr != NULL, false);
  set_read_buffer(rb._buffer_type);
  GLP(PixelStorei)(GL_PACK_ALIGNMENT, 1);

  // Bug fix for RE, RE2, and VTX - need to disable texturing in order
  // for GLP(ReadPixels)() to work
  // NOTE: reading the depth buffer is *much* slower than reading the
  // color buffer
  set_state_and_transform(RenderState::make_empty(), _internal_transform);

  int xo, yo, w, h;
  dr->get_region_pixels(xo, yo, w, h);

  Texture::ComponentType component_type = tex->get_component_type();
  bool color_mode = false;

  Texture::Format format = tex->get_format();
  switch (format) {
  case Texture::F_depth_stencil:
    if (_current_properties->get_depth_bits() <= 8) {
      component_type = Texture::T_unsigned_byte;
    } else {
      component_type = Texture::T_unsigned_short;
    }
    break;

  default:
    color_mode = true;
    if (_current_properties->get_alpha_bits()) {
      format = Texture::F_rgba;
    } else {
      format = Texture::F_rgb;
    }
    if (_current_properties->get_color_bits() <= 24) {
      component_type = Texture::T_unsigned_byte;
    } else {
      component_type = Texture::T_unsigned_short;
    }
  }

  Texture::TextureType texture_type;
  if (z >= 0) {
    texture_type = Texture::TT_cube_map;
  } else {
    texture_type = Texture::TT_2d_texture;
  }

  if (tex->get_x_size() != w || tex->get_y_size() != h ||
      tex->get_component_type() != component_type ||
      tex->get_format() != format ||
      tex->get_texture_type() != texture_type) {
    // Re-setup the texture; its properties have changed.
    tex->setup_texture(texture_type, w, h, tex->get_z_size(),
                       component_type, format);
  }

  GLenum external_format = get_external_image_format(tex);

  if (GLCAT.is_spam()) {
    GLCAT.spam()
      << "glReadPixels(" << xo << ", " << yo << ", " << w << ", " << h << ", ";
    switch (external_format) {
    case GL_DEPTH_COMPONENT:
      GLCAT.spam(false) << "GL_DEPTH_COMPONENT, ";
      break;
    case GL_DEPTH_STENCIL_EXT:
      GLCAT.spam(false) << "GL_DEPTH_STENCIL, ";
      break;
    case GL_RGB:
      GLCAT.spam(false) << "GL_RGB, ";
      break;
    case GL_RGBA:
      GLCAT.spam(false) << "GL_RGBA, ";
      break;
    case GL_BGR:
      GLCAT.spam(false) << "GL_BGR, ";
      break;
    case GL_BGRA:
      GLCAT.spam(false) << "GL_BGRA, ";
      break;
    default:
      GLCAT.spam(false) << "unknown, ";
      break;
    }
    switch (get_component_type(component_type)) {
    case GL_UNSIGNED_BYTE:
      GLCAT.spam(false) << "GL_UNSIGNED_BYTE";
      break;
    case GL_UNSIGNED_SHORT:
      GLCAT.spam(false) << "GL_UNSIGNED_SHORT";
      break;
    case GL_FLOAT:
      GLCAT.spam(false) << "GL_FLOAT";
      break;
    default:
      GLCAT.spam(false) << "unknown";
      break;
    }
    GLCAT.spam(false)
      << ")" << endl;
  }

  size_t image_size = tex->get_ram_image_size();
  unsigned char *image_ptr = tex->modify_ram_image();
  if (z >= 0) {
    nassertr(z < tex->get_z_size(), false);
    image_size = tex->get_expected_ram_page_size();
    image_ptr += z * image_size;
  }

  GLP(ReadPixels)(xo, yo, w, h,
                  external_format, get_component_type(component_type),
                  image_ptr);

  // We may have to reverse the byte ordering of the image if GL
  // didn't do it for us.
  if (color_mode && !_supports_bgr) {
    PTA_uchar new_image;
    const unsigned char *result =
      fix_component_ordering(new_image, image_ptr, image_size,
                             external_format, tex);
    if (result != image_ptr) {
      memcpy(image_ptr, result, image_size);
    }
  }

  report_my_gl_errors();
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::apply_fog
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
apply_fog(Fog *fog) {
  Fog::Mode fmode = fog->get_mode();
  GLP(Fogi)(GL_FOG_MODE, get_fog_mode_type(fmode));

  if (fmode == Fog::M_linear) {
    float onset, opaque;
    fog->get_linear_range(onset, opaque);
    GLP(Fogf)(GL_FOG_START, onset);
    GLP(Fogf)(GL_FOG_END, opaque);

  } else {
    // Exponential fog is always camera-relative.
    GLP(Fogf)(GL_FOG_DENSITY, fog->get_exp_density());
  }

  GLP(Fogfv)(GL_FOG_COLOR, fog->get_color().get_data());
  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::do_issue_transform
//       Access: Protected
//  Description: Sends the indicated transform matrix to the graphics
//               API to be applied to future vertices.
//
//               This transform is the internal_transform, already
//               converted into the GSG's internal coordinate system.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
do_issue_transform() {
  const TransformState *transform = _internal_transform;
  if (GLCAT.is_spam()) {
    GLCAT.spam()
      << "glLoadMatrix(GL_MODELVIEW): " << transform->get_mat() << endl;
  }

  DO_PSTATS_STUFF(_transform_state_pcollector.add_level(1));
  GLP(MatrixMode)(GL_MODELVIEW);
  GLP(LoadMatrixf)(transform->get_mat().get_data());
  _transform_stale = false;

  if (_auto_rescale_normal) {
    do_auto_rescale_normal();
  }

  if (_current_shader_context) {
    _current_shader_context->issue_parameters(this, Shader::SSD_transform);
  }

  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::do_issue_shade_model
//       Access: Protected
//  Description:
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
do_issue_shade_model() {
  const ShadeModelAttrib *attrib = _target._shade_model;
  switch (attrib->get_mode()) {
  case ShadeModelAttrib::M_smooth:
    GLP(ShadeModel)(GL_SMOOTH);
    _flat_shade_model = false;
    break;

  case ShadeModelAttrib::M_flat:
    GLP(ShadeModel)(GL_FLAT);
    _flat_shade_model = true;
    break;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::do_issue_shader
//       Access: Protected
//  Description:
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
do_issue_shader() {
  CLP(ShaderContext) *context = 0;
  Shader *shader = 0;
  if (_target._shader) {
    shader = (Shader *)(_target._shader->get_shader());
  }
  if (shader) {
    context = (CLP(ShaderContext) *)(shader->prepare_now(get_prepared_objects(), this));
  }
  
  if (context == 0 || (context -> valid ( ) == false)) {
    if (_current_shader_context != 0) {
      _current_shader_context->unbind();
      _current_shader = 0;
      _current_shader_context = 0;
    }
  } else {
    if (context != _current_shader_context) {
      // Use a completely different shader than before.
      // Unbind old shader, bind the new one.
      if (_current_shader_context != 0) {
        _current_shader_context->unbind();
      }
      context->bind(this);
      _current_shader = shader;
      _current_shader_context = context;
    } else {
      // Use the same shader as before, but with new input arguments.
      context->issue_parameters(this, Shader::SSD_shaderinputs);
    }
  }

  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::do_issue_render_mode
//       Access: Protected
//  Description:
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
do_issue_render_mode() {
  const RenderModeAttrib *attrib = _target._render_mode;
  _render_mode = attrib->get_mode();
  _point_size = attrib->get_thickness();
  _point_perspective = attrib->get_perspective();

  switch (_render_mode) {
  case RenderModeAttrib::M_unchanged:
  case RenderModeAttrib::M_filled:
    GLP(PolygonMode)(GL_FRONT_AND_BACK, GL_FILL);
    break;

  case RenderModeAttrib::M_wireframe:
    GLP(PolygonMode)(GL_FRONT_AND_BACK, GL_LINE);
    break;

  case RenderModeAttrib::M_point:
    GLP(PolygonMode)(GL_FRONT_AND_BACK, GL_POINT);
    break;

  default:
    GLCAT.error()
      << "Unknown render mode " << (int)_render_mode << endl;
  }

  // The thickness affects both the line width and the point size.
  GLP(LineWidth)(_point_size);
  GLP(PointSize)(_point_size);
  report_my_gl_errors();

  do_point_size();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::do_issue_antialias
//       Access: Protected
//  Description:
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
do_issue_antialias() {
  const AntialiasAttrib *attrib = _target._antialias;
  if (attrib->get_mode_type() == AntialiasAttrib::M_auto) {
    // In this special mode, we must enable antialiasing on a
    // case-by-case basis, because we enable it differently for
    // polygons and for points and lines.
    _auto_antialias_mode = true;

  } else {
    // Otherwise, explicitly enable or disable according to the bits
    // that are set.  But if multisample is requested and supported,
    // don't use the other bits at all (they will be ignored by GL
    // anyway).
    _auto_antialias_mode = false;
    unsigned short mode = attrib->get_mode();

    if (_supports_multisample &&
        (mode & AntialiasAttrib::M_multisample) != 0) {
      enable_multisample_antialias(true);

    } else {
      enable_multisample_antialias(false);
      enable_line_smooth((mode & AntialiasAttrib::M_line) != 0);
      enable_point_smooth((mode & AntialiasAttrib::M_point) != 0);
      enable_polygon_smooth((mode & AntialiasAttrib::M_polygon) != 0);
    }
  }

  switch (attrib->get_mode_quality()) {
  case AntialiasAttrib::M_faster:
    GLP(Hint)(GL_LINE_SMOOTH_HINT, GL_FASTEST);
    GLP(Hint)(GL_POINT_SMOOTH_HINT, GL_FASTEST);
    GLP(Hint)(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
    break;

  case AntialiasAttrib::M_better:
    GLP(Hint)(GL_LINE_SMOOTH_HINT, GL_NICEST);
    GLP(Hint)(GL_POINT_SMOOTH_HINT, GL_NICEST);
    GLP(Hint)(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    break;

  default:
    GLP(Hint)(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
    GLP(Hint)(GL_POINT_SMOOTH_HINT, GL_DONT_CARE);
    GLP(Hint)(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);
    break;
  }

  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::do_issue_rescale_normal
//       Access: Protected
//  Description:
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
do_issue_rescale_normal() {
  const RescaleNormalAttrib *attrib = _target._rescale_normal;
  RescaleNormalAttrib::Mode mode = attrib->get_mode();

  _auto_rescale_normal = false;

  switch (mode) {
  case RescaleNormalAttrib::M_none:
    GLP(Disable)(GL_NORMALIZE);
    if (_supports_rescale_normal && support_rescale_normal) {
      GLP(Disable)(GL_RESCALE_NORMAL);
    }
    break;

  case RescaleNormalAttrib::M_rescale:
    if (_supports_rescale_normal && support_rescale_normal) {
      GLP(Enable)(GL_RESCALE_NORMAL);
      GLP(Disable)(GL_NORMALIZE);
    } else {
      GLP(Enable)(GL_NORMALIZE);
    }
    break;

  case RescaleNormalAttrib::M_normalize:
    GLP(Enable)(GL_NORMALIZE);
    if (_supports_rescale_normal && support_rescale_normal) {
      GLP(Disable)(GL_RESCALE_NORMAL);
    }
    break;

  case RescaleNormalAttrib::M_auto:
    _auto_rescale_normal = true;
    do_auto_rescale_normal();
    break;

  default:
    GLCAT.error()
      << "Unknown rescale_normal mode " << (int)mode << endl;
  }
  report_my_gl_errors();
}

// PandaCompareFunc - 1 + 0x200 === GL_NEVER, etc.  order is sequential
#define PANDA_TO_GL_COMPAREFUNC(PANDACMPFUNC) (PANDACMPFUNC-1 +0x200)

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::do_issue_depth_test
//       Access: Protected
//  Description:
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
do_issue_depth_test() {
  const DepthTestAttrib *attrib = _target._depth_test;
  DepthTestAttrib::PandaCompareFunc mode = attrib->get_mode();
  if (mode == DepthTestAttrib::M_none) {
    enable_depth_test(false);
  } else {
    enable_depth_test(true);
    GLP(DepthFunc)(PANDA_TO_GL_COMPAREFUNC(mode));
  }
  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::do_issue_alpha_test
//       Access: Protected
//  Description:
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
do_issue_alpha_test() {
  const AlphaTestAttrib *attrib = _target._alpha_test;
  AlphaTestAttrib::PandaCompareFunc mode = attrib->get_mode();
  if (mode == AlphaTestAttrib::M_none) {
    enable_alpha_test(false);
  } else {
    assert(GL_NEVER==(AlphaTestAttrib::M_never-1+0x200));
    GLP(AlphaFunc)(PANDA_TO_GL_COMPAREFUNC(mode), attrib->get_reference_alpha());
    enable_alpha_test(true);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::do_issue_depth_write
//       Access: Protected
//  Description:
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
do_issue_depth_write() {
  const DepthWriteAttrib *attrib = _target._depth_write;
  DepthWriteAttrib::Mode mode = attrib->get_mode();
  if (mode == DepthWriteAttrib::M_off) {
    GLP(DepthMask)(GL_FALSE);
  } else {
    GLP(DepthMask)(GL_TRUE);
  }
  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::do_issue_cull_face
//       Access: Protected
//  Description:
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
do_issue_cull_face() {
  const CullFaceAttrib *attrib = _target._cull_face;
  CullFaceAttrib::Mode mode = attrib->get_effective_mode();

  switch (mode) {
  case CullFaceAttrib::M_cull_none:
    GLP(Disable)(GL_CULL_FACE);
    break;
  case CullFaceAttrib::M_cull_clockwise:
    GLP(Enable)(GL_CULL_FACE);
    GLP(CullFace)(GL_BACK);
    break;
  case CullFaceAttrib::M_cull_counter_clockwise:
    GLP(Enable)(GL_CULL_FACE);
    GLP(CullFace)(GL_FRONT);
    break;
  default:
    GLCAT.error()
      << "invalid cull face mode " << (int)mode << endl;
    break;
  }
  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::do_issue_fog
//       Access: Protected
//  Description:
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
do_issue_fog() {
  const FogAttrib *attrib = _target._fog;
  if (!attrib->is_off()) {
    enable_fog(true);
    Fog *fog = attrib->get_fog();
    nassertv(fog != (Fog *)NULL);
    apply_fog(fog);
  } else {
    enable_fog(false);
  }
  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::do_issue_depth_offset
//       Access: Protected
//  Description:
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
do_issue_depth_offset() {
  const DepthOffsetAttrib *attrib = _target._depth_offset;
  int offset = attrib->get_offset();

  if (offset != 0) {
    // The relationship between these two parameters is a little
    // unclear and poorly explained in the GL man pages.
    GLP(PolygonOffset)((GLfloat) -offset, (GLfloat) -offset);
    enable_polygon_offset(true);

  } else {
    enable_polygon_offset(false);
  }

  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::do_issue_material
//       Access: Protected
//  Description:
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
do_issue_material() {
  static Material empty;
  const Material *material;
  if (_target._material == (MaterialAttrib *)NULL ||
      _target._material->is_off()) {
    material = &empty;
  } else {
    material = _target._material->get_material();
  }

  GLenum face = material->get_twoside() ? GL_FRONT_AND_BACK : GL_FRONT;

  GLP(Materialfv)(face, GL_SPECULAR, material->get_specular().get_data());
  GLP(Materialfv)(face, GL_EMISSION, material->get_emission().get_data());
  GLP(Materialf)(face, GL_SHININESS, material->get_shininess());

  if (material->has_ambient() && material->has_diffuse()) {
    // The material has both an ambient and diffuse specified.  This
    // means we do not need glMaterialColor().
    GLP(Disable)(GL_COLOR_MATERIAL);
    GLP(Materialfv)(face, GL_AMBIENT, material->get_ambient().get_data());
    GLP(Materialfv)(face, GL_DIFFUSE, material->get_diffuse().get_data());

  } else if (material->has_ambient()) {
    // The material specifies an ambient, but not a diffuse component.
    // The diffuse component comes from the object's color.
    GLP(Materialfv)(face, GL_AMBIENT, material->get_ambient().get_data());
    if (_has_material_force_color) {
      GLP(Disable)(GL_COLOR_MATERIAL);
      GLP(Materialfv)(face, GL_DIFFUSE, _material_force_color.get_data());
    } else {
      GLP(ColorMaterial)(face, GL_DIFFUSE);
      GLP(Enable)(GL_COLOR_MATERIAL);
    }

  } else if (material->has_diffuse()) {
    // The material specifies a diffuse, but not an ambient component.
    // The ambient component comes from the object's color.
    GLP(Materialfv)(face, GL_DIFFUSE, material->get_diffuse().get_data());
    if (_has_material_force_color) {
      GLP(Disable)(GL_COLOR_MATERIAL);
      GLP(Materialfv)(face, GL_AMBIENT, _material_force_color.get_data());
    } else {
      GLP(ColorMaterial)(face, GL_AMBIENT);
      GLP(Enable)(GL_COLOR_MATERIAL);
    }

  } else {
    // The material specifies neither a diffuse nor an ambient
    // component.  Both components come from the object's color.
    if (_has_material_force_color) {
      GLP(Disable)(GL_COLOR_MATERIAL);
      GLP(Materialfv)(face, GL_AMBIENT, _material_force_color.get_data());
      GLP(Materialfv)(face, GL_DIFFUSE, _material_force_color.get_data());
    } else {
      GLP(ColorMaterial)(face, GL_AMBIENT_AND_DIFFUSE);
      GLP(Enable)(GL_COLOR_MATERIAL);
    }
  }

  GLP(LightModeli)(GL_LIGHT_MODEL_LOCAL_VIEWER, material->get_local());
  GLP(LightModeli)(GL_LIGHT_MODEL_TWO_SIDE, material->get_twoside());
  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::do_issue_blending
//       Access: Protected
//  Description:
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
do_issue_blending() {

  // Handle the color_write attrib.  If color_write is off, then
  // all the other blending-related stuff doesn't matter.  If the
  // device doesn't support color-write, we use blending tricks
  // to effectively disable color write.
  unsigned int color_channels =
    _target._color_write->get_channels() & _color_write_mask;
  if (color_channels == ColorWriteAttrib::C_off) {
    if (_target._color_write != _state._color_write) {
      enable_multisample_alpha_one(false);
      enable_multisample_alpha_mask(false);
      if (CLP(color_mask)) {
        enable_blend(false);
        GLP(ColorMask)(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
      } else {
        enable_blend(true);
        _glBlendEquation(GL_FUNC_ADD);
        GLP(BlendFunc)(GL_ZERO, GL_ONE);
      }
    }
    return;
  } else {
    if (_target._color_write != _state._color_write) {
      if (CLP(color_mask)) {
        GLP(ColorMask)((color_channels & ColorWriteAttrib::C_red) != 0,
                       (color_channels & ColorWriteAttrib::C_green) != 0,
                       (color_channels & ColorWriteAttrib::C_blue) != 0,
                       (color_channels & ColorWriteAttrib::C_alpha) != 0);
      }
    }
  }

  CPT(ColorBlendAttrib) color_blend = _target._color_blend;
  ColorBlendAttrib::Mode color_blend_mode = _target._color_blend->get_mode();
  TransparencyAttrib::Mode transparency_mode = _target._transparency->get_mode();

  _color_blend_involves_color_scale = color_blend->involves_color_scale();

  // Is there a color blend set?
  if (color_blend_mode != ColorBlendAttrib::M_none) {
    enable_multisample_alpha_one(false);
    enable_multisample_alpha_mask(false);
    enable_blend(true);
    _glBlendEquation(get_blend_equation_type(color_blend_mode));
    GLP(BlendFunc)(get_blend_func(color_blend->get_operand_a()),
                   get_blend_func(color_blend->get_operand_b()));

    if (_color_blend_involves_color_scale) {
      // Apply the current color scale to the blend mode.
      _glBlendColor(_current_color_scale[0], _current_color_scale[1],
                    _current_color_scale[2], _current_color_scale[3]);

    } else {
      Colorf c = color_blend->get_color();
      _glBlendColor(c[0], c[1], c[2], c[3]);
    }
    return;
  }

  // No color blend; is there a transparency set?
  switch (transparency_mode) {
  case TransparencyAttrib::M_none:
  case TransparencyAttrib::M_binary:
    break;

  case TransparencyAttrib::M_alpha:
  case TransparencyAttrib::M_dual:
    enable_multisample_alpha_one(false);
    enable_multisample_alpha_mask(false);
    enable_blend(true);
    _glBlendEquation(GL_FUNC_ADD);
    GLP(BlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return;

  case TransparencyAttrib::M_multisample:
    // We need to enable *both* of these in M_multisample case.
    enable_multisample_alpha_one(true);
    enable_multisample_alpha_mask(true);
    enable_blend(false);
    return;

  case TransparencyAttrib::M_multisample_mask:
    enable_multisample_alpha_one(false);
    enable_multisample_alpha_mask(true);
    enable_blend(false);
    return;

  default:
    GLCAT.error()
      << "invalid transparency mode " << (int)transparency_mode << endl;
    break;
  }

  if (_line_smooth_enabled || _point_smooth_enabled) {
    // If we have either of these turned on, we also need to have
    // blend mode enabled in order to see it.
    enable_multisample_alpha_one(false);
    enable_multisample_alpha_mask(false);
    enable_blend(true);
    _glBlendEquation(GL_FUNC_ADD);
    GLP(BlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return;
  }

  // For best polygon smoothing, we need:
  // (1) a frame buffer that supports alpha
  // (2) sort polygons front-to-back
  // (3) glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_ONE);
  //
  // Since these modes have other implications for the application, we
  // don't attempt to do this by default.  If you really want good
  // polygon smoothing (and you don't have multisample support), do
  // all this yourself.

  // Nothing's set, so disable blending.
  enable_multisample_alpha_one(false);
  enable_multisample_alpha_mask(false);
  enable_blend(false);
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::bind_light
//       Access: Public, Virtual
//  Description: Called the first time a particular light has been
//               bound to a given id within a frame, this should set
//               up the associated hardware light with the light's
//               properties.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
bind_light(PointLight *light_obj, const NodePath &light, int light_id) {
  static PStatCollector _draw_set_state_light_bind_point_pcollector("Draw:Set State:Light:Bind:Point");
  PStatTimer timer(_draw_set_state_light_bind_point_pcollector);
  
  float light_color[4];
  GLenum id = get_light_id(light_id);
  static const Colorf black(0.0f, 0.0f, 0.0f, 1.0f);
  GLP(Lightfv)(id, GL_AMBIENT, black.get_data());
  GLP(Lightfv)(id, GL_DIFFUSE, get_light_color(light_color, light_obj));
  GLP(Lightfv)(id, GL_SPECULAR, light_obj->get_specular_color().get_data());

  // Position needs to specify x, y, z, and w
  // w == 1 implies non-infinite position
  CPT(TransformState) transform = light.get_transform(_scene_setup->get_scene_root().get_parent());
  LPoint3f pos = light_obj->get_point() * transform->get_mat();

  LPoint4f fpos(pos[0], pos[1], pos[2], 1.0f);
  GLP(Lightfv)(id, GL_POSITION, fpos.get_data());

  // GL_SPOT_DIRECTION is not significant when cutoff == 180

  // Exponent == 0 implies uniform light distribution
  GLP(Lightf)(id, GL_SPOT_EXPONENT, 0.0f);

  // Cutoff == 180 means uniform point light source
  GLP(Lightf)(id, GL_SPOT_CUTOFF, 180.0f);

  const LVecBase3f &att = light_obj->get_attenuation();
  GLP(Lightf)(id, GL_CONSTANT_ATTENUATION, att[0]);
  GLP(Lightf)(id, GL_LINEAR_ATTENUATION, att[1]);
  GLP(Lightf)(id, GL_QUADRATIC_ATTENUATION, att[2]);

  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::bind_light
//       Access: Public, Virtual
//  Description: Called the first time a particular light has been
//               bound to a given id within a frame, this should set
//               up the associated hardware light with the light's
//               properties.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
bind_light(DirectionalLight *light_obj, const NodePath &light, int light_id) {
  static PStatCollector _draw_set_state_light_bind_directional_pcollector("Draw:Set State:Light:Bind:Directional");
  PStatTimer timer(_draw_set_state_light_bind_directional_pcollector);

  float light_color[4];
  GLenum id = get_light_id( light_id );
  static const Colorf black(0.0f, 0.0f, 0.0f, 1.0f);
  GLP(Lightfv)(id, GL_AMBIENT, black.get_data());
  GLP(Lightfv)(id, GL_DIFFUSE, get_light_color(light_color, light_obj));
  GLP(Lightfv)(id, GL_SPECULAR, light_obj->get_specular_color().get_data());

  // Position needs to specify x, y, z, and w.
  // w == 0 implies light is at infinity
  CPT(TransformState) transform = light.get_transform(_scene_setup->get_scene_root().get_parent());
  LVector3f dir = light_obj->get_direction() * transform->get_mat();
  LPoint4f fdir(-dir[0], -dir[1], -dir[2], 0);
  GLP(Lightfv)(id, GL_POSITION, fdir.get_data());

  // GL_SPOT_DIRECTION is not significant when cutoff == 180
  // In this case, position x, y, z specifies direction

  // Exponent == 0 implies uniform light distribution
  GLP(Lightf)(id, GL_SPOT_EXPONENT, 0.0f);

  // Cutoff == 180 means uniform point light source
  GLP(Lightf)(id, GL_SPOT_CUTOFF, 180.0f);

  // Default attenuation values (only spotlight and point light can
  // modify these)
  GLP(Lightf)(id, GL_CONSTANT_ATTENUATION, 1.0f);
  GLP(Lightf)(id, GL_LINEAR_ATTENUATION, 0.0f);
  GLP(Lightf)(id, GL_QUADRATIC_ATTENUATION, 0.0f);

  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::bind_light
//       Access: Public, Virtual
//  Description: Called the first time a particular light has been
//               bound to a given id within a frame, this should set
//               up the associated hardware light with the light's
//               properties.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
bind_light(Spotlight *light_obj, const NodePath &light, int light_id) {
  static PStatCollector _draw_set_state_light_bind_spotlight_pcollector("Draw:Set State:Light:Bind:Spotlight");
  PStatTimer timer(_draw_set_state_light_bind_spotlight_pcollector);
  
  Lens *lens = light_obj->get_lens();
  nassertv(lens != (Lens *)NULL);

  float light_color[4];
  GLenum id = get_light_id(light_id);
  static const Colorf black(0.0f, 0.0f, 0.0f, 1.0f);
  GLP(Lightfv)(id, GL_AMBIENT, black.get_data());
  GLP(Lightfv)(id, GL_DIFFUSE, get_light_color(light_color, light_obj));
  GLP(Lightfv)(id, GL_SPECULAR, light_obj->get_specular_color().get_data());

  // Position needs to specify x, y, z, and w
  // w == 1 implies non-infinite position
  CPT(TransformState) transform = light.get_transform(_scene_setup->get_scene_root().get_parent());
  const LMatrix4f &light_mat = transform->get_mat();
  LPoint3f pos = lens->get_nodal_point() * light_mat;
  LVector3f dir = lens->get_view_vector() * light_mat;

  LPoint4f fpos(pos[0], pos[1], pos[2], 1.0f);
  GLP(Lightfv)(id, GL_POSITION, fpos.get_data());
  GLP(Lightfv)(id, GL_SPOT_DIRECTION, dir.get_data());

  GLP(Lightf)(id, GL_SPOT_EXPONENT, light_obj->get_exponent());
  GLP(Lightf)(id, GL_SPOT_CUTOFF, lens->get_hfov() * 0.5f);

  const LVecBase3f &att = light_obj->get_attenuation();
  GLP(Lightf)(id, GL_CONSTANT_ATTENUATION, att[0]);
  GLP(Lightf)(id, GL_LINEAR_ATTENUATION, att[1]);
  GLP(Lightf)(id, GL_QUADRATIC_ATTENUATION, att[2]);

  report_my_gl_errors();
}

#ifdef SUPPORT_IMMEDIATE_MODE
////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::draw_immediate_simple_primitives
//       Access: Protected
//  Description: Uses the ImmediateModeSender to draw a series of
//               primitives of the indicated type.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
draw_immediate_simple_primitives(const GeomPrimitivePipelineReader *reader, GLenum mode) {
  int num_vertices = reader->get_num_vertices();
  _vertices_immediate_pcollector.add_level(num_vertices);
  GLP(Begin)(mode);

  if (reader->is_indexed()) {
    for (int v = 0; v < num_vertices; ++v) {
      _sender.set_vertex(reader->get_vertex(v));
      _sender.issue_vertex();
    }

  } else {
    _sender.set_vertex(reader->get_first_vertex());
    for (int v = 0; v < num_vertices; ++v) {
      _sender.issue_vertex();
    }
  }

  GLP(End)();
}
#endif  // SUPPORT_IMMEDIATE_MODE

#ifdef SUPPORT_IMMEDIATE_MODE
////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::draw_immediate_composite_primitives
//       Access: Protected
//  Description: Uses the ImmediateModeSender to draw a series of
//               primitives of the indicated type.  This form is for
//               primitive types like tristrips which must involve
//               several begin/end groups.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
draw_immediate_composite_primitives(const GeomPrimitivePipelineReader *reader, GLenum mode) {
  int num_vertices = reader->get_num_vertices();
  _vertices_immediate_pcollector.add_level(num_vertices);
  CPTA_int ends = reader->get_ends();
  int num_unused_vertices_per_primitive = reader->get_object()->get_num_unused_vertices_per_primitive();

  if (reader->is_indexed()) {
    int begin = 0;
    CPTA_int::const_iterator ei;
    for (ei = ends.begin(); ei != ends.end(); ++ei) {
      int end = (*ei);

      GLP(Begin)(mode);
      for (int v = begin; v < end; ++v) {
        _sender.set_vertex(reader->get_vertex(v));
        _sender.issue_vertex();
      }
      GLP(End)();

      begin = end + num_unused_vertices_per_primitive;
    }

  } else {
    _sender.set_vertex(reader->get_first_vertex());
    int begin = 0;
    CPTA_int::const_iterator ei;
    for (ei = ends.begin(); ei != ends.end(); ++ei) {
      int end = (*ei);

      GLP(Begin)(mode);
      for (int v = begin; v < end; ++v) {
        _sender.issue_vertex();
      }
      GLP(End)();

      begin = end + num_unused_vertices_per_primitive;
    }
  }
}
#endif  // SUPPORT_IMMEDIATE_MODE

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::gl_flush
//       Access: Protected, Virtual
//  Description: Calls glFlush().
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
gl_flush() const {
  PStatTimer timer(_flush_pcollector);
  GLP(Flush)();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::gl_get_error
//       Access: Protected, Virtual
//  Description: Returns the result of glGetError().
////////////////////////////////////////////////////////////////////
GLenum CLP(GraphicsStateGuardian)::
gl_get_error() const {
  return GLP(GetError)();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::report_errors_loop
//       Access: Protected, Static
//  Description: The internal implementation of report_errors().
//               Don't call this function; use report_errors()
//               instead.  The return value is true if everything is
//               ok, or false if we should shut down.
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
report_errors_loop(int line, const char *source_file, GLenum error_code,
                   int &error_count) {
#ifndef NDEBUG
  while ((CLP(max_errors) < 0 || error_count < CLP(max_errors)) &&
         (error_code != GL_NO_ERROR)) {
    GLCAT.error()
      << "at " << line << " of " << source_file << " : "
      << get_error_string(error_code) << "\n";

    error_code = GLP(GetError)();
    error_count++;
  }

#endif
  return (error_code == GL_NO_ERROR);
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::get_error_string
//       Access: Protected, Static
//  Description: Returns gluGetErrorString(), if GLU is available;
//               otherwise, returns some default error message.
////////////////////////////////////////////////////////////////////
string CLP(GraphicsStateGuardian)::
get_error_string(GLenum error_code) {
#ifdef HAVE_GLU
  const GLubyte *error_string = GLUP(ErrorString)(error_code);
  if (error_string != (const GLubyte *)NULL) {
    return string((const char *)error_string);
  }
#endif  // HAVE_GLU

  ostringstream strm;
  strm << "GL error " << (int)error_code;

  return strm.str();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::show_gl_string
//       Access: Protected
//  Description: Outputs the result of glGetString() on the indicated
//               tag.  The output string is returned.
////////////////////////////////////////////////////////////////////
string CLP(GraphicsStateGuardian)::
show_gl_string(const string &name, GLenum id) {
  string result;

  const GLubyte *text = GLP(GetString)(id);

  if (text == (const GLubyte *)NULL) {
    GLCAT.warning()
      << "Unable to query " << name << "\n";
  } else {
    result = (const char *)text;
    if (GLCAT.is_debug()) {
      GLCAT.debug()
        << name << " = " << result << "\n";
    }
  }

  return result;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::query_gl_version
//       Access: Protected, Virtual
//  Description: Queries the runtime version of OpenGL in use.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
query_gl_version() {
  _gl_vendor = show_gl_string("GL_VENDOR", GL_VENDOR);
  _gl_renderer = show_gl_string("GL_RENDERER", GL_RENDERER);
  _gl_version = show_gl_string("GL_VERSION", GL_VERSION);

  _gl_version_major = 0;
  _gl_version_minor = 0;
  _gl_version_release = 0;

  const GLubyte *text = GLP(GetString)(GL_VERSION);
  if (text == (const GLubyte *)NULL) {
    GLCAT.debug()
      << "Unable to query GL_VERSION\n";
  } else {
    string version((const char *)text);

    string input = version;
    size_t space = input.find(' ');
    if (space != string::npos) {
      input = input.substr(0, space);
    }

    vector_string components;
    tokenize(input, components, ".");
    if (components.size() >= 1) {
      string_to_int(components[0], _gl_version_major);
    }
    if (components.size() >= 2) {
      string_to_int(components[1], _gl_version_minor);
    }
    if (components.size() >= 3) {
      string_to_int(components[2], _gl_version_release);
    }

    if (GLCAT.is_debug()) {
      GLCAT.debug()
        << "GL_VERSION = " << version << ", decoded to "
        << _gl_version_major << "." << _gl_version_minor
        << "." << _gl_version_release << "\n";
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::save_extensions
//       Access: Protected
//  Description: Separates the string returned by GL_EXTENSIONS (or
//               glx or wgl extensions) into its individual tokens
//               and saves them in the _extensions member.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
save_extensions(const char *extensions) {
  if (extensions != (const char *)NULL) {
    vector_string tokens;
    extract_words(extensions, tokens);

    vector_string::iterator ti;
    for (ti = tokens.begin(); ti != tokens.end(); ++ti) {
      _extensions.insert(*ti);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::get_extra_extensions
//       Access: Protected, Virtual
//  Description: This may be redefined by a derived class (e.g. glx or
//               wgl) to get whatever further extensions strings may
//               be appropriate to that interface, in addition to the
//               GL extension strings return by glGetString().
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
get_extra_extensions() {
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::report_extensions
//       Access: Protected
//  Description: Outputs the list of GL extensions to notify, if debug
//               mode is enabled.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
report_extensions() const {
  if (GLCAT.is_debug()) {
    GLCAT.debug()
      << "GL Extensions:\n";
    pset<string>::const_iterator ei;
    for (ei = _extensions.begin(); ei != _extensions.end(); ++ei) {
      GLCAT.debug() << (*ei) << "\n";
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::has_extension
//       Access: Protected
//  Description: Returns true if the indicated extension is reported
//               by the GL system, false otherwise.  The extension
//               name is case-sensitive.
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
has_extension(const string &extension) const {

  bool state;

  state = _extensions.find(extension) != _extensions.end();
  if (GLCAT.is_debug()) {
    GLCAT.debug()
      << "HAS EXT " << extension << " " << state << "\n";
  }

  return state;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::is_at_least_version
//       Access: Public
//  Description: Returns true if the runtime GL version number is at
//               least the indicated value, false otherwise.
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
is_at_least_version(int major_version, int minor_version,
                    int release_version) const {
  if (_gl_version_major < major_version) {
    return false;
  } else if (_gl_version_major == major_version) {
    if (_gl_version_minor < minor_version) {
      return false;
    } else if (_gl_version_minor == minor_version) {
      if (_gl_version_release < release_version) {
        return false;
      }
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::get_extension_func
//       Access: Public, Virtual
//  Description: Returns the pointer to the GL extension function with
//               the indicated name.  It is the responsibility of the
//               caller to ensure that the required extension is
//               defined in the OpenGL runtime prior to calling this;
//               it is an error to call this for a function that is
//               not defined.
////////////////////////////////////////////////////////////////////
void *CLP(GraphicsStateGuardian)::
get_extension_func(const char *, const char *) {
  return NULL;
}


////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::set_draw_buffer
//       Access: Protected
//  Description: Sets up the GLP(DrawBuffer) to render into the buffer
//               indicated by the RenderBuffer object.  This only sets
//               up the color and aux bits; it does not affect the depth,
//               stencil, accum layers.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
set_draw_buffer(int rbtype) {

  if (_current_fbo) {

    GLuint buffers[16];
    int nbuffers=0;
    if (rbtype & RenderBuffer::T_color) {
      buffers[nbuffers++] = GL_COLOR_ATTACHMENT0_EXT;
    }
    int index = 1;
    for (int i=0; i<_current_properties->get_aux_rgba(); i++) {
      if (rbtype & (RenderBuffer::T_aux_rgba_0 << i)) {
        buffers[nbuffers++] = GL_COLOR_ATTACHMENT0_EXT + index;
      }
      index += 1;
    }
    for (int i=0; i<_current_properties->get_aux_hrgba(); i++) {
      if (rbtype & (RenderBuffer::T_aux_hrgba_0 << i)) {
        buffers[nbuffers++] = GL_COLOR_ATTACHMENT0_EXT + index;
      }
      index += 1;
    }
    for (int i=0; i<_current_properties->get_aux_float(); i++) {
      if (rbtype & (RenderBuffer::T_aux_float_0 << i)) {
        buffers[nbuffers++] = GL_COLOR_ATTACHMENT0_EXT + index;
      }
      index += 1;
    }
    _glDrawBuffers(nbuffers, buffers);

  } else {

    switch (rbtype & RenderBuffer::T_color) {
    case RenderBuffer::T_front:
      GLP(DrawBuffer)(GL_FRONT);
      break;

    case RenderBuffer::T_back:
      GLP(DrawBuffer)(GL_BACK);
      break;

    case RenderBuffer::T_right:
      GLP(DrawBuffer)(GL_RIGHT);
      break;

    case RenderBuffer::T_left:
      GLP(DrawBuffer)(GL_LEFT);
      break;

    case RenderBuffer::T_front_right:
      nassertv(_current_properties->is_stereo());
      GLP(DrawBuffer)(GL_FRONT_RIGHT);
      break;

    case RenderBuffer::T_front_left:
      nassertv(_current_properties->is_stereo());
      GLP(DrawBuffer)(GL_FRONT_LEFT);
      break;

    case RenderBuffer::T_back_right:
      nassertv(_current_properties->is_stereo());
      GLP(DrawBuffer)(GL_BACK_RIGHT);
      break;

    case RenderBuffer::T_back_left:
      nassertv(_current_properties->is_stereo());
      GLP(DrawBuffer)(GL_BACK_LEFT);
      break;

    default:
      break;
    }
  }
  
  // Also ensure that any global color channels are masked out.
  if (CLP(color_mask)) {
    GLP(ColorMask)((_color_write_mask & ColorWriteAttrib::C_red) != 0,
                   (_color_write_mask & ColorWriteAttrib::C_green) != 0,
                   (_color_write_mask & ColorWriteAttrib::C_blue) != 0,
                   (_color_write_mask & ColorWriteAttrib::C_alpha) != 0);
  }
  
  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::set_read_buffer
//       Access: Protected
//  Description: Sets up the GLP(ReadBuffer) to render into the buffer
//               indicated by the RenderBuffer object.  This only sets
//               up the color bits; it does not affect the depth,
//               stencil, accum layers.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
set_read_buffer(int rbtype) {

  if (rbtype & (RenderBuffer::T_depth | RenderBuffer::T_stencil)) {
    // Special case: don't have to call ReadBuffer for these.
    return;
  }

  if (_current_fbo) {

    GLuint buffer = GL_COLOR_ATTACHMENT0_EXT;
    int index = 1;
    for (int i=0; i<_current_properties->get_aux_rgba(); i++) {
      if (rbtype & (RenderBuffer::T_aux_rgba_0 << i)) {
        buffer = GL_COLOR_ATTACHMENT0_EXT + index;
      }
      index += 1;
    }
    for (int i=0; i<_current_properties->get_aux_hrgba(); i++) {
      if (rbtype & (RenderBuffer::T_aux_hrgba_0 << i)) {
        buffer = GL_COLOR_ATTACHMENT0_EXT + index;
      }
      index += 1;
    }
    for (int i=0; i<_current_properties->get_aux_float(); i++) {
      if (rbtype & (RenderBuffer::T_aux_float_0 << i)) {
        buffer = GL_COLOR_ATTACHMENT0_EXT + index;
      }
      index += 1;
    }
    GLP(ReadBuffer)(buffer);

  } else {

    switch (rbtype & RenderBuffer::T_color) {
    case RenderBuffer::T_front:
      GLP(ReadBuffer)(GL_FRONT);
      break;

    case RenderBuffer::T_back:
      GLP(ReadBuffer)(GL_BACK);
      break;

    case RenderBuffer::T_right:
      GLP(ReadBuffer)(GL_RIGHT);
      break;

    case RenderBuffer::T_left:
      GLP(ReadBuffer)(GL_LEFT);
      break;

    case RenderBuffer::T_front_right:
      GLP(ReadBuffer)(GL_FRONT_RIGHT);
      break;

    case RenderBuffer::T_front_left:
      GLP(ReadBuffer)(GL_FRONT_LEFT);
      break;

    case RenderBuffer::T_back_right:
      GLP(ReadBuffer)(GL_BACK_RIGHT);
      break;

    case RenderBuffer::T_back_left:
      GLP(ReadBuffer)(GL_BACK_LEFT);
      break;

    default:
      GLP(ReadBuffer)(GL_FRONT_AND_BACK);
    }
  }

  report_my_gl_errors();
}



////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::get_numeric_type
//       Access: Protected, Static
//  Description: Maps from the Geom's internal numeric type symbols
//               to GL's.
////////////////////////////////////////////////////////////////////
GLenum CLP(GraphicsStateGuardian)::
get_numeric_type(Geom::NumericType numeric_type) {
  switch (numeric_type) {
  case Geom::NT_uint16:
    return GL_UNSIGNED_SHORT;

  case Geom::NT_uint32:
    return GL_UNSIGNED_INT;

  case Geom::NT_uint8:
  case Geom::NT_packed_dcba:
  case Geom::NT_packed_dabc:
    return GL_UNSIGNED_BYTE;

  case Geom::NT_float32:
    return GL_FLOAT;
  }

  GLCAT.error()
    << "Invalid NumericType value (" << (int)numeric_type << ")\n";
  return GL_UNSIGNED_BYTE;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::get_texture_target
//       Access: Protected
//  Description: Maps from the Texture's texture type symbols to
//               GL's.
////////////////////////////////////////////////////////////////////
GLenum CLP(GraphicsStateGuardian)::
get_texture_target(Texture::TextureType texture_type) const {
  switch (texture_type) {
  case Texture::TT_1d_texture:
    return GL_TEXTURE_1D;

  case Texture::TT_2d_texture:
    return GL_TEXTURE_2D;

  case Texture::TT_3d_texture:
    if (_supports_3d_texture) {
      return GL_TEXTURE_3D;
    } else {
      return GL_NONE;
    }

  case Texture::TT_cube_map:
    if (_supports_cube_map) {
      return GL_TEXTURE_CUBE_MAP;
    } else {
      return GL_NONE;
    }
  }

  GLCAT.error() << "Invalid Texture::TextureType value!\n";
  return GL_TEXTURE_2D;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::get_texture_wrap_mode
//       Access: Protected
//  Description: Maps from the Texture's internal wrap mode symbols to
//               GL's.
////////////////////////////////////////////////////////////////////
GLenum CLP(GraphicsStateGuardian)::
get_texture_wrap_mode(Texture::WrapMode wm) const {
  if (CLP(ignore_clamp)) {
    return GL_REPEAT;
  }
  switch (wm) {
  case Texture::WM_clamp:
    return _edge_clamp;

  case Texture::WM_repeat:
    return GL_REPEAT;

  case Texture::WM_mirror:
    return _mirror_repeat;

  case Texture::WM_mirror_once:
    return _mirror_border_clamp;

  case Texture::WM_border_color:
    return _border_clamp;

  case Texture::WM_invalid:
    break;
  }
  GLCAT.error() << "Invalid Texture::WrapMode value!\n";
  return _edge_clamp;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::get_panda_wrap_mode
//       Access: Protected, Static
//  Description: Maps from the GL's internal wrap mode symbols to
//               Panda's.
////////////////////////////////////////////////////////////////////
Texture::WrapMode CLP(GraphicsStateGuardian)::
get_panda_wrap_mode(GLenum wm) {
  switch (wm) {
  case GL_CLAMP:
  case GL_CLAMP_TO_EDGE:
    return Texture::WM_clamp;

  case GL_CLAMP_TO_BORDER:
    return Texture::WM_border_color;

  case GL_REPEAT:
    return Texture::WM_repeat;

  case GL_MIRROR_CLAMP_EXT:
  case GL_MIRROR_CLAMP_TO_EDGE_EXT:
    return Texture::WM_mirror;

  case GL_MIRROR_CLAMP_TO_BORDER_EXT:
    return Texture::WM_mirror_once;
  }
  GLCAT.error() << "Unexpected GL wrap mode " << (int)wm << "\n";
  return Texture::WM_clamp;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::get_texture_filter_type
//       Access: Protected, Static
//  Description: Maps from the Texture's internal filter type symbols
//               to GL's.
////////////////////////////////////////////////////////////////////
GLenum CLP(GraphicsStateGuardian)::
get_texture_filter_type(Texture::FilterType ft, Texture::Format fmt,
                        bool ignore_mipmaps) {
  if (CLP(ignore_filters)) {
    return GL_NEAREST;

  } else if (ignore_mipmaps) {
    switch (ft) {
    case Texture::FT_nearest_mipmap_nearest:
    case Texture::FT_nearest:
      return GL_NEAREST;
    case Texture::FT_linear:
    case Texture::FT_linear_mipmap_nearest:
    case Texture::FT_nearest_mipmap_linear:
    case Texture::FT_linear_mipmap_linear:
      return GL_LINEAR;
    case Texture::FT_shadow:
      return GL_LINEAR;
    case Texture::FT_default:
      if ((fmt == Texture::F_depth_stencil)) {
        return GL_NEAREST;
      } else {
        return GL_LINEAR;
      }
    case Texture::FT_invalid:
      break;
    }

  } else {
    switch (ft) {
    case Texture::FT_nearest:
      return GL_NEAREST;
    case Texture::FT_linear:
      return GL_LINEAR;
    case Texture::FT_nearest_mipmap_nearest:
      return GL_NEAREST_MIPMAP_NEAREST;
    case Texture::FT_linear_mipmap_nearest:
      return GL_LINEAR_MIPMAP_NEAREST;
    case Texture::FT_nearest_mipmap_linear:
      return GL_NEAREST_MIPMAP_LINEAR;
    case Texture::FT_linear_mipmap_linear:
      return GL_LINEAR_MIPMAP_LINEAR;
    case Texture::FT_shadow:
      return GL_LINEAR;
    case Texture::FT_default:
      if ((fmt == Texture::F_depth_stencil)) {
        return GL_NEAREST;
      } else {
        return GL_LINEAR;
      }
    case Texture::FT_invalid:
      break;
    }
  }
  GLCAT.error() << "Invalid Texture::FilterType value!\n";
  return GL_NEAREST;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::get_panda_filter_type
//       Access: Protected, Static
//  Description: Maps from the GL's internal filter type symbols
//               to Panda's.
////////////////////////////////////////////////////////////////////
Texture::FilterType CLP(GraphicsStateGuardian)::
get_panda_filter_type(GLenum ft) {
  switch (ft) {
  case GL_NEAREST:
    return Texture::FT_nearest;
  case GL_LINEAR:
    return Texture::FT_linear;
  case GL_NEAREST_MIPMAP_NEAREST:
    return Texture::FT_nearest_mipmap_nearest;
  case GL_LINEAR_MIPMAP_NEAREST:
    return Texture::FT_linear_mipmap_nearest;
  case GL_NEAREST_MIPMAP_LINEAR:
    return Texture::FT_nearest_mipmap_linear;
  case GL_LINEAR_MIPMAP_LINEAR:
    return Texture::FT_linear_mipmap_linear;
  }
  GLCAT.error() << "Unexpected GL filter type " << (int)ft << "\n";
  return Texture::FT_linear;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::get_component_type
//       Access: Protected, Static
//  Description: Maps from the Texture's internal ComponentType symbols
//               to GL's.
////////////////////////////////////////////////////////////////////
GLenum CLP(GraphicsStateGuardian)::
get_component_type(Texture::ComponentType component_type) {
  switch (component_type) {
  case Texture::T_unsigned_byte:
    return GL_UNSIGNED_BYTE;
  case Texture::T_unsigned_short:
    return GL_UNSIGNED_SHORT;
  case Texture::T_float:
    return GL_FLOAT;

  default:
    GLCAT.error() << "Invalid Texture::Type value!\n";
    return GL_UNSIGNED_BYTE;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::get_external_image_format
//       Access: Protected
//  Description: Maps from the Texture's Format symbols
//               to GL's.
////////////////////////////////////////////////////////////////////
GLint CLP(GraphicsStateGuardian)::
get_external_image_format(Texture *tex) const {
  Texture::CompressionMode compression = tex->get_ram_image_compression();
  if (compression != Texture::CM_off &&
      get_supports_compressed_texture_format(compression)) {
    switch (compression) {
    case Texture::CM_on:
      switch (tex->get_format()) {
      case Texture::F_color_index:
      case Texture::F_depth_stencil:
        // This shouldn't be possible.
        nassertr(false, GL_RGB);
        break;
        
      case Texture::F_rgba:
      case Texture::F_rgbm:
      case Texture::F_rgba4:
      case Texture::F_rgba8:
      case Texture::F_rgba12:
        return GL_COMPRESSED_RGBA;
        
      case Texture::F_rgb:
      case Texture::F_rgb5:
      case Texture::F_rgba5:
      case Texture::F_rgb8:
      case Texture::F_rgb12:
      case Texture::F_rgb332:
        return GL_COMPRESSED_RGB;
        
      case Texture::F_alpha:
        return GL_COMPRESSED_ALPHA;
        
      case Texture::F_red:
      case Texture::F_green:
      case Texture::F_blue:
      case Texture::F_luminance:
        return GL_COMPRESSED_LUMINANCE;
        
      case Texture::F_luminance_alpha:
      case Texture::F_luminance_alphamask:
        return GL_COMPRESSED_LUMINANCE_ALPHA;
      }
      break;
      
    case Texture::CM_dxt1:
      if (Texture::has_alpha(tex->get_format())) {
        return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
      } else {
        return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
      }

    case Texture::CM_dxt3:
      return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;

    case Texture::CM_dxt5:
      return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;

    case Texture::CM_fxt1:
      if (Texture::has_alpha(tex->get_format())) {
        return GL_COMPRESSED_RGBA_FXT1_3DFX;
      } else {
        return GL_COMPRESSED_RGB_FXT1_3DFX;
      }

    case Texture::CM_default:
    case Texture::CM_off:
    case Texture::CM_dxt2:
    case Texture::CM_dxt4:
      // This shouldn't happen.
      nassertr(false, GL_RGB);
      break;
    }
  }

  switch (tex->get_format()) {
  case Texture::F_color_index:
    return GL_COLOR_INDEX;
  case Texture::F_depth_stencil:
    if (_supports_depth_stencil) {
      return GL_DEPTH_STENCIL_EXT;
    } else {
      return GL_DEPTH_COMPONENT;
    }
  case Texture::F_red:
    return GL_RED;
  case Texture::F_green:
    return GL_GREEN;
  case Texture::F_blue:
    return GL_BLUE;
  case Texture::F_alpha:
    return GL_ALPHA;
  case Texture::F_rgb:
  case Texture::F_rgb5:
  case Texture::F_rgb8:
  case Texture::F_rgb12:
  case Texture::F_rgb332:
    return _supports_bgr ? GL_BGR : GL_RGB;
  case Texture::F_rgba:
  case Texture::F_rgbm:
  case Texture::F_rgba4:
  case Texture::F_rgba5:
  case Texture::F_rgba8:
  case Texture::F_rgba12:
  case Texture::F_rgba16:
  case Texture::F_rgba32:
    return _supports_bgr ? GL_BGRA : GL_RGBA;
  case Texture::F_luminance:
    return GL_LUMINANCE;
  case Texture::F_luminance_alphamask:
  case Texture::F_luminance_alpha:
    return GL_LUMINANCE_ALPHA;
  }
  GLCAT.error()
    << "Invalid Texture::Format value in get_external_image_format(): "
    << (int)tex->get_format() << "\n";
  return GL_RGB;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::get_internal_image_format
//       Access: Protected
//  Description: Maps from the Texture's Format symbols to a
//               suitable internal format for GL textures.
////////////////////////////////////////////////////////////////////
GLint CLP(GraphicsStateGuardian)::
get_internal_image_format(Texture *tex) const {
  Texture::CompressionMode compression = tex->get_compression();
  if (compression == Texture::CM_default) {
    compression = (compressed_textures) ? Texture::CM_on : Texture::CM_off;
  }
  if (tex->get_render_to_texture()) {
    // no compression for render targets
    compression = Texture::CM_off;
  }
  bool is_3d = (tex->get_texture_type() == Texture::TT_3d_texture);

  if (get_supports_compressed_texture_format(compression)) {
    switch (compression) {
    case Texture::CM_on:
      // The user asked for just generic compression.  OpenGL supports
      // requesting just generic compression, but we'd like to go ahead
      // and request a specific type (if we can figure out an
      // appropriate choice), since that makes saving the result as a
      // pre-compressed texture more dependable--this way, we will know
      // which compression algorithm was applied.
      switch (tex->get_format()) {
      case Texture::F_color_index:
      case Texture::F_depth_stencil:
        // Unsupported; fall through to below.
        break;
        
      case Texture::F_rgbm:
        if (get_supports_compressed_texture_format(Texture::CM_dxt1) && !is_3d) {
          return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        } else if (get_supports_compressed_texture_format(Texture::CM_fxt1) && !is_3d) {
          return GL_COMPRESSED_RGBA_FXT1_3DFX;
        } else {
          return GL_COMPRESSED_RGBA;
        }
        
      case Texture::F_rgba4:
        if (get_supports_compressed_texture_format(Texture::CM_dxt3) && !is_3d) {
          return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        } else if (get_supports_compressed_texture_format(Texture::CM_fxt1) && !is_3d) {
          return GL_COMPRESSED_RGBA_FXT1_3DFX;
        } else {
          return GL_COMPRESSED_RGBA;
        }
        
      case Texture::F_rgba:
      case Texture::F_rgba8:
      case Texture::F_rgba12:
        if (get_supports_compressed_texture_format(Texture::CM_dxt5) && !is_3d) {
          return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        } else if (get_supports_compressed_texture_format(Texture::CM_fxt1) && !is_3d) {
          return GL_COMPRESSED_RGBA_FXT1_3DFX;
        } else {
          return GL_COMPRESSED_RGBA;
        }
        
      case Texture::F_rgb:
      case Texture::F_rgb5:
      case Texture::F_rgba5:
      case Texture::F_rgb8:
      case Texture::F_rgb12:
      case Texture::F_rgb332:
        if (get_supports_compressed_texture_format(Texture::CM_dxt1) && !is_3d) {
          return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        } else if (get_supports_compressed_texture_format(Texture::CM_fxt1) && !is_3d) {
          return GL_COMPRESSED_RGB_FXT1_3DFX;
        } else {
          return GL_COMPRESSED_RGB;
        }
        
      case Texture::F_alpha:
        if (get_supports_compressed_texture_format(Texture::CM_dxt5) && !is_3d) {
          return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        } else if (get_supports_compressed_texture_format(Texture::CM_fxt1) && !is_3d) {
          return GL_COMPRESSED_RGBA_FXT1_3DFX;
        } else {
          return GL_COMPRESSED_ALPHA;
        }
        
      case Texture::F_red:
      case Texture::F_green:
      case Texture::F_blue:
      case Texture::F_luminance:
        if (get_supports_compressed_texture_format(Texture::CM_dxt1) && !is_3d) {
          return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        } else if (get_supports_compressed_texture_format(Texture::CM_fxt1) && !is_3d) {
          return GL_COMPRESSED_RGB_FXT1_3DFX;
        } else {
          return GL_COMPRESSED_LUMINANCE;
        }
        
      case Texture::F_luminance_alpha:
      case Texture::F_luminance_alphamask:
        if (get_supports_compressed_texture_format(Texture::CM_dxt5) && !is_3d) {
          return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        } else if (get_supports_compressed_texture_format(Texture::CM_fxt1) && !is_3d) {
          return GL_COMPRESSED_RGBA_FXT1_3DFX;
        } else {
          return GL_COMPRESSED_LUMINANCE_ALPHA;
        }
      }
      break;
      
    case Texture::CM_dxt1:
      if (Texture::has_alpha(tex->get_format())) {
        return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
      } else {
        return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
      }

    case Texture::CM_dxt3:
      return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;

    case Texture::CM_dxt5:
      return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;

    case Texture::CM_fxt1:
      if (Texture::has_alpha(tex->get_format())) {
        return GL_COMPRESSED_RGBA_FXT1_3DFX;
      } else {
        return GL_COMPRESSED_RGB_FXT1_3DFX;
      }

    case Texture::CM_default:
    case Texture::CM_off:
    case Texture::CM_dxt2:
    case Texture::CM_dxt4:
      // No compression: fall through to below.
      break;
    }
  }

  switch (tex->get_format()) {
  case Texture::F_color_index:
    return GL_COLOR_INDEX;
  case Texture::F_depth_stencil:
    if (_supports_depth_stencil) {
      return GL_DEPTH_STENCIL_EXT;
    } else {
      return GL_DEPTH_COMPONENT;
    }
  case Texture::F_rgba:
  case Texture::F_rgbm:
    return GL_RGBA;
  case Texture::F_rgba4:
    return GL_RGBA4;
  case Texture::F_rgba8:
    return GL_RGBA8;
  case Texture::F_rgba12:
    return GL_RGBA12;

  case Texture::F_rgba16:
    return GL_RGBA16F_ARB;
  case Texture::F_rgba32:
    return GL_RGBA32F_ARB;

  case Texture::F_rgb:
    return GL_RGB;
  case Texture::F_rgb5:
    return GL_RGB5;
  case Texture::F_rgba5:
    return GL_RGB5_A1;
  case Texture::F_rgb8:
    return GL_RGB8;
  case Texture::F_rgb12:
    return GL_RGB12;
  case Texture::F_rgb332:
    return GL_R3_G3_B2;

  case Texture::F_alpha:
    return GL_ALPHA;

  case Texture::F_red:
  case Texture::F_green:
  case Texture::F_blue:
  case Texture::F_luminance:
    return GL_LUMINANCE;
  case Texture::F_luminance_alpha:
  case Texture::F_luminance_alphamask:
    return GL_LUMINANCE_ALPHA;

  default:
    GLCAT.error()
      << "Invalid image format in get_internal_image_format(): "
      << (int)tex->get_format() << "\n";
    return GL_RGB;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::is_mipmap_filter
//       Access: Protected, Static
//  Description: Returns true if the indicated GL minfilter type
//               represents a mipmap format, false otherwise.
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
is_mipmap_filter(GLenum min_filter) {
  switch (min_filter) {
  case GL_NEAREST_MIPMAP_NEAREST:
  case GL_LINEAR_MIPMAP_NEAREST:
  case GL_NEAREST_MIPMAP_LINEAR:
  case GL_LINEAR_MIPMAP_LINEAR:
    return true;

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::is_compressed_format
//       Access: Protected, Static
//  Description: Returns true if the indicated GL internal format
//               represents a compressed texture format, false
//               otherwise.
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
is_compressed_format(GLenum format) {
  switch (format) {
  case GL_COMPRESSED_RGB:
  case GL_COMPRESSED_RGBA:
  case GL_COMPRESSED_ALPHA:
  case GL_COMPRESSED_LUMINANCE:
  case GL_COMPRESSED_LUMINANCE_ALPHA:
  case GL_COMPRESSED_RGB_FXT1_3DFX:
  case GL_COMPRESSED_RGBA_FXT1_3DFX:
  case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
  case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
  case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
  case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
    return true;

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::get_texture_apply_mode_type
//       Access: Protected, Static
//  Description: Maps from the texture stage's mode types
//               to the corresponding OpenGL ids
////////////////////////////////////////////////////////////////////
GLint CLP(GraphicsStateGuardian)::
get_texture_apply_mode_type(TextureStage::Mode am) {
  switch (am) {
  case TextureStage::M_modulate: return GL_MODULATE;
  case TextureStage::M_decal: return GL_DECAL;
  case TextureStage::M_blend: return GL_BLEND;
  case TextureStage::M_replace: return GL_REPLACE;
  case TextureStage::M_add: return GL_ADD;
  case TextureStage::M_combine: return GL_COMBINE;
  case TextureStage::M_blend_color_scale: return GL_BLEND;
  case TextureStage::M_modulate_glow: return GL_MODULATE;
  case TextureStage::M_modulate_gloss: return GL_MODULATE;
  }

  GLCAT.error()
    << "Invalid TextureStage::Mode value" << endl;
  return GL_MODULATE;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::get_texture_combine_type
//       Access: Protected, Static
//  Description: Maps from the texture stage's CombineMode types
//               to the corresponding OpenGL ids
////////////////////////////////////////////////////////////////////
GLint CLP(GraphicsStateGuardian)::
get_texture_combine_type(TextureStage::CombineMode cm) {
  switch (cm) {
  case TextureStage::CM_undefined: // fall through
  case TextureStage::CM_replace: return GL_REPLACE;
  case TextureStage::CM_modulate: return GL_MODULATE;
  case TextureStage::CM_add: return GL_ADD;
  case TextureStage::CM_add_signed: return GL_ADD_SIGNED;
  case TextureStage::CM_interpolate: return GL_INTERPOLATE;
  case TextureStage::CM_subtract: return GL_SUBTRACT;
  case TextureStage::CM_dot3_rgb: return GL_DOT3_RGB;
  case TextureStage::CM_dot3_rgba: return GL_DOT3_RGBA;
  }
  GLCAT.error()
    << "Invalid TextureStage::CombineMode value" << endl;
  return GL_REPLACE;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::get_texture_src_type
//       Access: Protected
//  Description: Maps from the texture stage's CombineSource types
//               to the corresponding OpenGL ids
////////////////////////////////////////////////////////////////////
GLint CLP(GraphicsStateGuardian)::
get_texture_src_type(TextureStage::CombineSource cs,
                     int last_stage, int last_saved_result,
                     int this_stage) const {
  switch (cs) {
  case TextureStage::CS_undefined: // fall through
  case TextureStage::CS_texture: return GL_TEXTURE;
  case TextureStage::CS_constant: return GL_CONSTANT;
  case TextureStage::CS_primary_color: return GL_PRIMARY_COLOR;
  case TextureStage::CS_constant_color_scale: return GL_CONSTANT;

  case TextureStage::CS_previous:
    if (last_stage == this_stage - 1) {
      return GL_PREVIOUS;
    } else if (last_stage == -1) {
      return GL_PRIMARY_COLOR;
    } else if (_supports_texture_saved_result) {
      return GL_TEXTURE0 + last_stage;
    } else {
      GLCAT.warning()
        << "Current OpenGL driver does not support texture crossbar blending.\n";
      return GL_PRIMARY_COLOR;
    }

  case TextureStage::CS_last_saved_result:
    if (last_saved_result == this_stage - 1) {
      return GL_PREVIOUS;
    } else if (last_saved_result == -1) {
      return GL_PRIMARY_COLOR;
    } else if (_supports_texture_saved_result) {
      return GL_TEXTURE0 + last_saved_result;
    } else {
      GLCAT.warning()
        << "Current OpenGL driver does not support texture crossbar blending.\n";
      return GL_PRIMARY_COLOR;
    }
  }

  GLCAT.error()
    << "Invalid TextureStage::CombineSource value" << endl;
  return GL_TEXTURE;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::get_texture_operand_type
//       Access: Protected, Static
//  Description: Maps from the texture stage's CombineOperand types
//               to the corresponding OpenGL ids
////////////////////////////////////////////////////////////////////
GLint CLP(GraphicsStateGuardian)::
get_texture_operand_type(TextureStage::CombineOperand co) {
  switch (co) {
  case TextureStage::CO_undefined: // fall through
  case TextureStage::CO_src_alpha: return GL_SRC_ALPHA;
  case TextureStage::CO_one_minus_src_alpha: return GL_ONE_MINUS_SRC_ALPHA;
  case TextureStage::CO_src_color: return GL_SRC_COLOR;
  case TextureStage::CO_one_minus_src_color: return GL_ONE_MINUS_SRC_COLOR;
  }

  GLCAT.error()
    << "Invalid TextureStage::CombineOperand value" << endl;
  return GL_SRC_COLOR;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::get_fog_mode_type
//       Access: Protected, Static
//  Description: Maps from the fog types to gl version
////////////////////////////////////////////////////////////////////
GLenum CLP(GraphicsStateGuardian)::
get_fog_mode_type(Fog::Mode m) {
  switch(m) {
  case Fog::M_linear: return GL_LINEAR;
  case Fog::M_exponential: return GL_EXP;
  case Fog::M_exponential_squared: return GL_EXP2;
    /*
      case Fog::M_spline: return GL_FOG_FUNC_SGIS;
    */

  default:
    GLCAT.error() << "Invalid Fog::Mode value" << endl;
    return GL_EXP;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::get_blend_equation_type
//       Access: Protected, Static
//  Description: Maps from ColorBlendAttrib::Mode to glBlendEquation
//               value.
////////////////////////////////////////////////////////////////////
GLenum CLP(GraphicsStateGuardian)::
get_blend_equation_type(ColorBlendAttrib::Mode mode) {
  switch (mode) {
  case ColorBlendAttrib::M_none:
  case ColorBlendAttrib::M_add:
    return GL_FUNC_ADD;

  case ColorBlendAttrib::M_subtract:
    return GL_FUNC_SUBTRACT;

  case ColorBlendAttrib::M_inv_subtract:
    return GL_FUNC_REVERSE_SUBTRACT;

  case ColorBlendAttrib::M_min:
    return GL_MIN;

  case ColorBlendAttrib::M_max:
    return GL_MAX;
  }

  GLCAT.error()
    << "Unknown color blend mode " << (int)mode << endl;
  return GL_FUNC_ADD;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::get_blend_func
//       Access: Protected, Static
//  Description: Maps from ColorBlendAttrib::Operand to glBlendFunc
//               value.
////////////////////////////////////////////////////////////////////
GLenum CLP(GraphicsStateGuardian)::
get_blend_func(ColorBlendAttrib::Operand operand) {
  switch (operand) {
  case ColorBlendAttrib::O_zero:
    return GL_ZERO;

  case ColorBlendAttrib::O_one:
    return GL_ONE;

  case ColorBlendAttrib::O_incoming_color:
    return GL_SRC_COLOR;

  case ColorBlendAttrib::O_one_minus_incoming_color:
    return GL_ONE_MINUS_SRC_COLOR;

  case ColorBlendAttrib::O_fbuffer_color:
    return GL_DST_COLOR;

  case ColorBlendAttrib::O_one_minus_fbuffer_color:
    return GL_ONE_MINUS_DST_COLOR;

  case ColorBlendAttrib::O_incoming_alpha:
    return GL_SRC_ALPHA;

  case ColorBlendAttrib::O_one_minus_incoming_alpha:
    return GL_ONE_MINUS_SRC_ALPHA;

  case ColorBlendAttrib::O_fbuffer_alpha:
    return GL_DST_ALPHA;

  case ColorBlendAttrib::O_one_minus_fbuffer_alpha:
    return GL_ONE_MINUS_DST_ALPHA;

  case ColorBlendAttrib::O_constant_color:
  case ColorBlendAttrib::O_color_scale:
    return GL_CONSTANT_COLOR;

  case ColorBlendAttrib::O_one_minus_constant_color:
  case ColorBlendAttrib::O_one_minus_color_scale:
    return GL_ONE_MINUS_CONSTANT_COLOR;

  case ColorBlendAttrib::O_constant_alpha:
  case ColorBlendAttrib::O_alpha_scale:
    return GL_CONSTANT_ALPHA;

  case ColorBlendAttrib::O_one_minus_constant_alpha:
  case ColorBlendAttrib::O_one_minus_alpha_scale:
    return GL_ONE_MINUS_CONSTANT_ALPHA;

  case ColorBlendAttrib::O_incoming_color_saturate:
    return GL_SRC_ALPHA_SATURATE;
  }

  GLCAT.error()
    << "Unknown color blend operand " << (int)operand << endl;
  return GL_ZERO;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::get_usage
//       Access: Public, Static
//  Description: Maps from UsageHint to the GL symbol.
////////////////////////////////////////////////////////////////////
GLenum CLP(GraphicsStateGuardian)::
get_usage(Geom::UsageHint usage_hint) {
  switch (usage_hint) {
  case Geom::UH_stream:
    return GL_STREAM_DRAW;

  case Geom::UH_static:
  case Geom::UH_unspecified:
    return GL_STATIC_DRAW;

  case Geom::UH_dynamic:
    return GL_DYNAMIC_DRAW;

  case Geom::UH_client:
    break;
  }

  GLCAT.error()
    << "Unexpected usage_hint " << (int)usage_hint << endl;
  return GL_STATIC_DRAW;
}


////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::print_gfx_visual
//       Access: Public
//  Description: Prints a description of the current visual selected.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
print_gfx_visual() {
  GLint i;
  GLboolean j;
  cout << "Graphics Visual Info (# bits of each):" << endl;

  cout << "RGBA: ";
  GLP(GetIntegerv)( GL_RED_BITS, &i ); cout << i << " ";
  GLP(GetIntegerv)( GL_GREEN_BITS, &i ); cout << i << " ";
  GLP(GetIntegerv)( GL_BLUE_BITS, &i ); cout << i << " ";
  GLP(GetIntegerv)( GL_ALPHA_BITS, &i ); cout << i << endl;

  cout << "Accum RGBA: ";
  GLP(GetIntegerv)( GL_ACCUM_RED_BITS, &i ); cout << i << " ";
  GLP(GetIntegerv)( GL_ACCUM_GREEN_BITS, &i ); cout << i << " ";
  GLP(GetIntegerv)( GL_ACCUM_BLUE_BITS, &i ); cout << i << " ";
  GLP(GetIntegerv)( GL_ACCUM_ALPHA_BITS, &i ); cout << i << endl;

  GLP(GetIntegerv)( GL_INDEX_BITS, &i ); cout << "Color Index: " << i << endl;

  GLP(GetIntegerv)( GL_DEPTH_BITS, &i ); cout << "Depth: " << i << endl;
  GLP(GetIntegerv)( GL_ALPHA_BITS, &i ); cout << "Alpha: " << i << endl;
  GLP(GetIntegerv)( GL_STENCIL_BITS, &i ); cout << "Stencil: " << i << endl;

  GLP(GetBooleanv)( GL_DOUBLEBUFFER, &j ); cout << "DoubleBuffer? "
                                                << (int)j << endl;

  GLP(GetBooleanv)( GL_STEREO, &j ); cout << "Stereo? " << (int)j << endl;

  if (_supports_multisample) {
    GLP(GetBooleanv)( GL_MULTISAMPLE, &j ); cout << "Multisample? " << (int)j << endl;
    GLP(GetIntegerv)( GL_SAMPLES, &i ); cout << "Samples: " << i << endl;
  }

  GLP(GetBooleanv)( GL_BLEND, &j ); cout << "Blend? " << (int)j << endl;
  GLP(GetBooleanv)( GL_POINT_SMOOTH, &j ); cout << "Point Smooth? "
                                                << (int)j << endl;
  GLP(GetBooleanv)( GL_LINE_SMOOTH, &j ); cout << "Line Smooth? "
                                               << (int)j << endl;

  GLP(GetIntegerv)( GL_AUX_BUFFERS, &i ); cout << "Aux Buffers: " << i << endl;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::get_light_color
//       Access: Public
//  Description: Fills the array of four floats with the values that
//               that should be issued as the light's color, as scaled
//               by the current value of _light_color_scale, in the
//               case of color_scale_via_lighting.  Returns
//               light_color.
////////////////////////////////////////////////////////////////////
float *CLP(GraphicsStateGuardian)::
get_light_color(float light_color[4], Light *light) const {
  const Colorf &c = light->get_color();

  light_color[0] = c[0] * _light_color_scale[0];
  light_color[1] = c[1] * _light_color_scale[1];
  light_color[2] = c[2] * _light_color_scale[2];
  light_color[3] = c[3] * _light_color_scale[3];

  return light_color;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::enable_lighting
//       Access: Protected, Virtual
//  Description: Intended to be overridden by a derived class to
//               enable or disable the use of lighting overall.  This
//               is called by do_issue_light() according to whether any
//               lights are in use or not.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
enable_lighting(bool enable) {
  static PStatCollector _draw_set_state_light_enable_lighting_pcollector("Draw:Set State:Light:Enable lighting");
  PStatTimer timer(_draw_set_state_light_enable_lighting_pcollector);
  
  if (enable) {
    GLP(Enable)(GL_LIGHTING);
  } else {
    GLP(Disable)(GL_LIGHTING);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::set_ambient_light
//       Access: Protected, Virtual
//  Description: Intended to be overridden by a derived class to
//               indicate the color of the ambient light that should
//               be in effect.  This is called by do_issue_light() after
//               all other lights have been enabled or disabled.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
set_ambient_light(const Colorf &color) {
  static PStatCollector _draw_set_state_light_ambient_pcollector("Draw:Set State:Light:Ambient");
  PStatTimer timer(_draw_set_state_light_ambient_pcollector);
  
  Colorf c = color;
  c.set(c[0] * _light_color_scale[0],
        c[1] * _light_color_scale[1],
        c[2] * _light_color_scale[2],
        c[3] * _light_color_scale[3]);
  GLP(LightModelfv)(GL_LIGHT_MODEL_AMBIENT, c.get_data());
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::enable_light
//       Access: Protected, Virtual
//  Description: Intended to be overridden by a derived class to
//               enable the indicated light id.  A specific Light will
//               already have been bound to this id via bind_light().
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
enable_light(int light_id, bool enable) {
  static PStatCollector _draw_set_state_light_enable_light_pcollector("Draw:Set State:Light:Enable light");
  PStatTimer timer(_draw_set_state_light_enable_light_pcollector);
  
  if (enable) {
    GLP(Enable)(get_light_id(light_id));
  } else {
    GLP(Disable)(get_light_id(light_id));
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::begin_bind_lights
//       Access: Protected, Virtual
//  Description: Called immediately before bind_light() is called,
//               this is intended to provide the derived class a hook
//               in which to set up some state (like transform) that
//               might apply to several lights.
//
//               The sequence is: begin_bind_lights() will be called,
//               then one or more bind_light() calls, then
//               end_bind_lights().
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
begin_bind_lights() {
  static PStatCollector _draw_set_state_light_begin_bind_pcollector("Draw:Set State:Light:Begin bind");
  PStatTimer timer(_draw_set_state_light_begin_bind_pcollector);
  
  // We need to temporarily load a new matrix so we can define the
  // light in a known coordinate system.  We pick the transform of the
  // root.  (Alternatively, we could leave the current transform where
  // it is and compute the light position relative to that transform
  // instead of relative to the root, by composing with the matrix
  // computed by _internal_transform->invert_compose(render_transform).
  // But I think loading a completely new matrix is simpler.)
  CPT(TransformState) render_transform =
    _cs_transform->compose(_scene_setup->get_world_transform());

  GLP(MatrixMode)(GL_MODELVIEW);
  GLP(PushMatrix)();
  GLP(LoadMatrixf)(render_transform->get_mat().get_data());
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::end_bind_lights
//       Access: Protected, Virtual
//  Description: Called after before bind_light() has been called one
//               or more times (but before any geometry is issued or
//               additional state is changed), this is intended to
//               clean up any temporary changes to the state that may
//               have been made by begin_bind_lights().
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
end_bind_lights() {
  static PStatCollector _draw_set_state_light_end_bind_pcollector("Draw:Set State:Light:End bind");
  PStatTimer timer(_draw_set_state_light_end_bind_pcollector);
  
  GLP(MatrixMode)(GL_MODELVIEW);
  GLP(PopMatrix)();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::enable_clip_plane
//       Access: Protected, Virtual
//  Description: Intended to be overridden by a derived class to
//               enable the indicated clip_plane id.  A specific
//               PlaneNode will already have been bound to this id via
//               bind_clip_plane().
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
enable_clip_plane(int plane_id, bool enable) {
  if (enable) {
    GLP(Enable)(get_clip_plane_id(plane_id));
  } else {
    GLP(Disable)(get_clip_plane_id(plane_id));
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::begin_bind_clip_planes
//       Access: Protected, Virtual
//  Description: Called immediately before bind_clip_plane() is called,
//               this is intended to provide the derived class a hook
//               in which to set up some state (like transform) that
//               might apply to several clip_planes.
//
//               The sequence is: begin_bind_clip_planes() will be called,
//               then one or more bind_clip_plane() calls, then
//               end_bind_clip_planes().
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
begin_bind_clip_planes() {
  // We need to temporarily load a new matrix so we can define the
  // clip_plane in a known coordinate system.  We pick the transform of the
  // root.  (Alternatively, we could leave the current transform where
  // it is and compute the clip_plane position relative to that transform
  // instead of relative to the root, by composing with the matrix
  // computed by _internal_transform->invert_compose(render_transform).
  // But I think loading a completely new matrix is simpler.)
  CPT(TransformState) render_transform =
    _cs_transform->compose(_scene_setup->get_world_transform());

  GLP(MatrixMode)(GL_MODELVIEW);
  GLP(PushMatrix)();
  GLP(LoadMatrixf)(render_transform->get_mat().get_data());
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::bind_clip_plane
//       Access: Protected, Virtual
//  Description: Called the first time a particular clip_plane has been
//               bound to a given id within a frame, this should set
//               up the associated hardware clip_plane with the clip_plane's
//               properties.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
bind_clip_plane(const NodePath &plane, int plane_id) {
  GLenum id = get_clip_plane_id(plane_id);

  CPT(TransformState) transform = plane.get_transform(_scene_setup->get_scene_root().get_parent());
  const PlaneNode *plane_node;
  DCAST_INTO_V(plane_node, plane.node());
  Planef xformed_plane = plane_node->get_plane() * transform->get_mat();

  Planed double_plane(LCAST(double, xformed_plane));
  GLP(ClipPlane)(id, double_plane.get_data());

  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::end_bind_clip_planes
//       Access: Protected, Virtual
//  Description: Called after before bind_clip_plane() has been called one
//               or more times (but before any geometry is issued or
//               additional state is changed), this is intended to
//               clean up any temporary changes to the state that may
//               have been made by begin_bind_clip_planes().
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
end_bind_clip_planes() {
  GLP(MatrixMode)(GL_MODELVIEW);
  GLP(PopMatrix)();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::set_state_and_transform
//       Access: Public, Virtual
//  Description: Simultaneously resets the render state and the
//               transform state.
//
//               This transform specified is the "internal" net
//               transform, already converted into the GSG's internal
//               coordinate space by composing it to
//               get_cs_transform().  (Previously, this used to be the
//               "external" net transform, with the assumption that
//               that GSG would convert it internally, but that is no
//               longer the case.)
//
//               Special case: if (state==NULL), then the target
//               state is already stored in _target.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
set_state_and_transform(const RenderState *target,
                        const TransformState *transform) {
#ifndef NDEBUG
  if (gsg_cat.is_spam()) {
    gsg_cat.spam() << "Setting GSG state to " << (void *)target << ":\n";
    target->write(gsg_cat.spam(false), 2);
  }
#endif

  _state_pcollector.add_level(1);
  PStatTimer timer1(_draw_set_state_pcollector);

  if (transform != _internal_transform) {
    PStatTimer timer(_draw_set_state_transform_pcollector);
    _state_pcollector.add_level(1);
    _internal_transform = transform;
    do_issue_transform();
  }

  if (target == _state_rs) {
    return;
  }
  _target_rs = target;
  _target.clear_to_defaults();
  target->store_into_slots(&_target);
  _state_rs = 0;

  if (_target._shader && (_target._shader->auto_shader())) {
    _target._shader = _target_rs->get_generated_shader();
  }
  
  if (_target._alpha_test != _state._alpha_test) {
    PStatTimer timer(_draw_set_state_alpha_test_pcollector);
    do_issue_alpha_test();
    _state._alpha_test = _target._alpha_test;
  }

  if (_target._antialias != _state._antialias) {
    PStatTimer timer(_draw_set_state_antialias_pcollector);
    do_issue_antialias();
    _state._antialias = _target._antialias;
  }

  if (_target._clip_plane != _state._clip_plane) {
    PStatTimer timer(_draw_set_state_clip_plane_pcollector);
    do_issue_clip_plane();
    _state._clip_plane = _target._clip_plane;
  }

  if (_target._color != _state._color ||
      _target._color_scale != _state._color_scale) {
    PStatTimer timer(_draw_set_state_color_pcollector);
    do_issue_color();
    do_issue_color_scale();
    _state._color = _target._color;
    _state._color_scale = _target._color_scale;
    if (_current_shader_context) {
      _current_shader_context->issue_parameters(this, Shader::SSD_color);
    }
  }

  if (_target._cull_face != _state._cull_face) {
    PStatTimer timer(_draw_set_state_cull_face_pcollector);
    do_issue_cull_face();
    _state._cull_face = _target._cull_face;
  }

  if (_target._depth_offset != _state._depth_offset) {
    PStatTimer timer(_draw_set_state_depth_offset_pcollector);
    do_issue_depth_offset();
    _state._depth_offset = _target._depth_offset;
  }

  if (_target._depth_test != _state._depth_test) {
    PStatTimer timer(_draw_set_state_depth_test_pcollector);
    do_issue_depth_test();
    _state._depth_test = _target._depth_test;
  }

  if (_target._depth_write != _state._depth_write) {
    PStatTimer timer(_draw_set_state_depth_write_pcollector);
    do_issue_depth_write();
    _state._depth_write = _target._depth_write;
  }

  if (_target._render_mode != _state._render_mode) {
    PStatTimer timer(_draw_set_state_render_mode_pcollector);
    do_issue_render_mode();
    _state._render_mode = _target._render_mode;
  }
  
  if (_target._rescale_normal != _state._rescale_normal) {
    PStatTimer timer(_draw_set_state_rescale_normal_pcollector);
    do_issue_rescale_normal();
    _state._rescale_normal = _target._rescale_normal;
  }

  if (_target._shade_model != _state._shade_model) {
    PStatTimer timer(_draw_set_state_shade_model_pcollector);
    do_issue_shade_model();
    _state._shade_model = _target._shade_model;
  }

  if ((_target._transparency != _state._transparency)||
      (_target._color_write != _state._color_write)||
      (_target._color_blend != _state._color_blend)) {
    PStatTimer timer(_draw_set_state_blending_pcollector);
    do_issue_blending();
    _state._transparency = _target._transparency;
    _state._color_write = _target._color_write;
    _state._color_blend = _target._color_blend;
  }

  if (_target._shader != _state._shader) {
    PStatTimer timer(_draw_set_state_shader_pcollector);
    do_issue_shader();
    _state._shader = _target._shader;
    _state._texture = 0;
  }

  if (_target._texture != _state._texture) {
    PStatTimer timer(_draw_set_state_texture_pcollector);
    determine_effective_texture();
    int prev_active = _num_active_texture_stages;
    do_issue_texture();

    // Since the TexGen and TexMatrix states depend partly on the
    // particular set of textures in use, we should force both of
    // those to be reissued every time we change the texture state.
    _state._tex_gen = 0;
    _state._tex_matrix = 0;

    _state._texture = _target._texture;
  }
  
  // If one of the previously-loaded TexGen modes modified the texture
  // matrix, then if either state changed, we have to change both of
  // them now.
  if (_tex_gen_modifies_mat) {
    if ((_target._tex_gen != _state._tex_gen) ||
        (_target._tex_matrix != _state._tex_matrix)) {
      _state._tex_matrix = 0;
      _state._tex_gen = 0;
    }
  }
  
  if (_target._tex_matrix != _state._tex_matrix) {
    PStatTimer timer(_draw_set_state_tex_matrix_pcollector);
    do_issue_tex_matrix();
    _state._tex_matrix = _target._tex_matrix;
  }

  if (_effective_tex_gen != _state._tex_gen) {
    PStatTimer timer(_draw_set_state_tex_gen_pcollector);
    do_issue_tex_gen();
    _state._tex_gen = _effective_tex_gen;
  }
  
  if (_target._material != _state._material) {
    PStatTimer timer(_draw_set_state_material_pcollector);
    do_issue_material();
    _state._material = _target._material;
    if (_current_shader_context) {
      _current_shader_context->issue_parameters(this, Shader::SSD_material);
    }
  }

  if (_target._light != _state._light) {
    PStatTimer timer(_draw_set_state_light_pcollector);
    do_issue_light();
    _state._light = _target._light;
  }

  if (_target._stencil != _state._stencil) {
    PStatTimer timer(_draw_set_state_stencil_pcollector);
    do_issue_stencil();
    _state._stencil = _target._stencil;
  }
     
  if (_current_shader_context == 0) {
    if (_target._fog != _state._fog) {
      PStatTimer timer(_draw_set_state_fog_pcollector);
      do_issue_fog();
      _state._fog = _target._fog;
    }
  }

  _state_rs = _target_rs;
  maybe_gl_finish();
  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::free_pointers
//       Access: Protected, Virtual
//  Description: Frees some memory that was explicitly allocated
//               within the glgsg.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
free_pointers() {
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::do_auto_rescale_normal
//       Access: Protected
//  Description: Issues the appropriate GL commands to either rescale
//               or normalize the normals according to the current
//               transform.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
do_auto_rescale_normal() {
  if (_internal_transform->has_identity_scale()) {
    // If there's no scale at all, don't do anything.
    GLP(Disable)(GL_NORMALIZE);
    if (GLCAT.is_spam()) {
      GLCAT.spam() << "glDisable(GL_NORMALIZE)\n";
    }
    if (_supports_rescale_normal && support_rescale_normal) {
      GLP(Disable)(GL_RESCALE_NORMAL);
      if (GLCAT.is_spam()) {
        GLCAT.spam() << "glDisable(GL_RESCALE_NORMAL)\n";
      }
    }

  } else if (_internal_transform->has_uniform_scale()) {
    // There's a uniform scale; use the rescale feature if available.
    if (_supports_rescale_normal && support_rescale_normal) {
      GLP(Enable)(GL_RESCALE_NORMAL);
      GLP(Disable)(GL_NORMALIZE);
      if (GLCAT.is_spam()) {
        GLCAT.spam() << "glEnable(GL_RESCALE_NORMAL)\n";
        GLCAT.spam() << "glDisable(GL_NORMALIZE)\n";
      }
    } else {
      GLP(Enable)(GL_NORMALIZE);
      if (GLCAT.is_spam()) {
        GLCAT.spam() << "glEnable(GL_NORMALIZE)\n";
      }
    }

  } else {
    // If there's a non-uniform scale, normalize everything.
    GLP(Enable)(GL_NORMALIZE);
    if (GLCAT.is_spam()) {
      GLCAT.spam() << "glEnable(GL_NORMALIZE)\n";
    }
    if (_supports_rescale_normal && support_rescale_normal) {
      GLP(Disable)(GL_RESCALE_NORMAL);
      if (GLCAT.is_spam()) {
        GLCAT.spam() << "glDisable(GL_RESCALE_NORMAL)\n";
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::do_issue_texture
//       Access: Protected, Virtual
//  Description: This is called by set_state_and_transform() when
//               the texture state has changed.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
do_issue_texture() {
  DO_PSTATS_STUFF(_texture_state_pcollector.add_level(1));

  if (_texture_binding_shader_context==0) {
    if (_current_shader_context==0) {
      update_standard_texture_bindings();
    } else {
      disable_standard_texture_bindings();
      _current_shader_context->update_shader_texture_bindings(NULL,this);
    }
  } else {
    if (_current_shader_context==0) {
      _texture_binding_shader_context->disable_shader_texture_bindings(this);
      update_standard_texture_bindings();
    } else {
      _current_shader_context->
        update_shader_texture_bindings(_texture_binding_shader_context,this);
    }
  }
  _texture_binding_shader = _current_shader;
  _texture_binding_shader_context = _current_shader_context;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::update_standard_texture_bindings
//       Access: Private
//  Description:
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
update_standard_texture_bindings() {
  int num_stages = _effective_texture->get_num_on_ff_stages();

  nassertv(num_stages <= _max_texture_stages &&
           _num_active_texture_stages <= _max_texture_stages);

  _texture_involves_color_scale = false;

  int last_saved_result = -1;
  int last_stage = -1;
  int i;
  for (i = 0; i < num_stages; i++) {
    TextureStage *stage = _effective_texture->get_on_ff_stage(i);
    Texture *texture = _effective_texture->get_on_texture(stage);
    nassertv(texture != (Texture *)NULL);

    // Issue the texture on stage i.
    _glActiveTexture(GL_TEXTURE0 + i);

    // First, turn off the previous texture mode.
    GLP(Disable)(GL_TEXTURE_1D);
    GLP(Disable)(GL_TEXTURE_2D);
    if (_supports_3d_texture) {
      GLP(Disable)(GL_TEXTURE_3D);
    }
    if (_supports_cube_map) {
      GLP(Disable)(GL_TEXTURE_CUBE_MAP);
    }
    
    TextureContext *tc = texture->prepare_now(_prepared_objects, this);
    if (tc == (TextureContext *)NULL) {
      // Something wrong with this texture; skip it.
      break;
    }
    
    // Then, turn on the current texture mode.
    GLenum target = get_texture_target(texture->get_texture_type());
    if (target == GL_NONE) {
      // Unsupported texture mode.
      break;
    }
    GLP(Enable)(target);
    
    apply_texture(tc);
    
    if (stage->involves_color_scale() && _color_scale_enabled) {
      Colorf color = stage->get_color();
      color.set(color[0] * _current_color_scale[0],
                color[1] * _current_color_scale[1],
                color[2] * _current_color_scale[2],
                color[3] * _current_color_scale[3]);
      _texture_involves_color_scale = true;
      GLP(TexEnvfv)(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color.get_data());
    } else {
      GLP(TexEnvfv)(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, stage->get_color().get_data());
    }
    
    if (stage->get_mode() == TextureStage::M_decal) {
      if (texture->get_num_components() < 3 && _supports_texture_combine) {
        // Make a special case for 1- and 2-channel decal textures.
        // OpenGL does not define their use with GL_DECAL for some
        // reason, so implement them using the combiner instead.
        GLP(TexEnvi)(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
        GLP(TexEnvi)(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
        GLP(TexEnvi)(GL_TEXTURE_ENV, GL_RGB_SCALE, 1);
        GLP(TexEnvi)(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1);
        GLP(TexEnvi)(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
        GLP(TexEnvi)(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
        GLP(TexEnvi)(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PREVIOUS);
        GLP(TexEnvi)(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
        GLP(TexEnvi)(GL_TEXTURE_ENV, GL_SRC2_RGB, GL_TEXTURE);
        GLP(TexEnvi)(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);
        
      } else {
        // Normal 3- and 4-channel decal textures.
        GLP(TexEnvi)(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
      }
      
    } else if (stage->get_mode() == TextureStage::M_combine) {
      if (!_supports_texture_combine) {
        GLCAT.warning()
          << "TextureStage::M_combine mode is not supported.\n";
        GLP(TexEnvi)(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      } else {
        GLP(TexEnvi)(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
        GLP(TexEnvi)(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
        GLP(TexEnvi)(GL_TEXTURE_ENV, GL_RGB_SCALE, stage->get_rgb_scale());
        GLP(TexEnvi)(GL_TEXTURE_ENV, GL_ALPHA_SCALE, stage->get_alpha_scale());
        GLP(TexEnvi)(GL_TEXTURE_ENV, GL_COMBINE_RGB,
                     get_texture_combine_type(stage->get_combine_rgb_mode()));
        
        switch (stage->get_num_combine_rgb_operands()) {
        case 3:
          GLP(TexEnvi)(GL_TEXTURE_ENV, GL_SRC2_RGB,
                       get_texture_src_type(stage->get_combine_rgb_source2(),
                                            last_stage, last_saved_result, i));
          GLP(TexEnvi)(GL_TEXTURE_ENV, GL_OPERAND2_RGB,
                       get_texture_operand_type(stage->get_combine_rgb_operand2()));
          // fall through
          
        case 2:
          GLP(TexEnvi)(GL_TEXTURE_ENV, GL_SRC1_RGB,
                       get_texture_src_type(stage->get_combine_rgb_source1(),
                                            last_stage, last_saved_result, i));
          GLP(TexEnvi)(GL_TEXTURE_ENV, GL_OPERAND1_RGB,
                       get_texture_operand_type(stage->get_combine_rgb_operand1()));
          // fall through
          
        case 1:
          GLP(TexEnvi)(GL_TEXTURE_ENV, GL_SRC0_RGB,
                       get_texture_src_type(stage->get_combine_rgb_source0(),
                                            last_stage, last_saved_result, i));
          GLP(TexEnvi)(GL_TEXTURE_ENV, GL_OPERAND0_RGB,
                       get_texture_operand_type(stage->get_combine_rgb_operand0()));
          // fall through
          
        default:
          break;
        }
        GLP(TexEnvi)(GL_TEXTURE_ENV, GL_COMBINE_ALPHA,
                     get_texture_combine_type(stage->get_combine_alpha_mode()));
        
        switch (stage->get_num_combine_alpha_operands()) {
        case 3:
          GLP(TexEnvi)(GL_TEXTURE_ENV, GL_SRC2_ALPHA,
                       get_texture_src_type(stage->get_combine_alpha_source2(),
                                            last_stage, last_saved_result, i));
          GLP(TexEnvi)(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA,
                       get_texture_operand_type(stage->get_combine_alpha_operand2()));
          // fall through

        case 2:
          GLP(TexEnvi)(GL_TEXTURE_ENV, GL_SRC1_ALPHA,
                       get_texture_src_type(stage->get_combine_alpha_source1(),
                                            last_stage, last_saved_result, i));
          GLP(TexEnvi)(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA,
                       get_texture_operand_type(stage->get_combine_alpha_operand1()));
          // fall through

        case 1:
          GLP(TexEnvi)(GL_TEXTURE_ENV, GL_SRC0_ALPHA,
                       get_texture_src_type(stage->get_combine_alpha_source0(),
                                            last_stage, last_saved_result, i));
          GLP(TexEnvi)(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA,
                       get_texture_operand_type(stage->get_combine_alpha_operand0()));
          // fall through

        default:
          break;
        }
      }
    } else {
      GLint glmode = get_texture_apply_mode_type(stage->get_mode());
      GLP(TexEnvi)(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, glmode);
    }

    if (stage->get_saved_result()) {
      // This texture's result will be "saved" for a future stage's
      // input.
      last_saved_result = i;
    } else {
      // This is a regular texture stage; it will be the "previous"
      // input for the next stage.
      last_stage = i;
    }
  }

  // Disable the texture stages that are no longer used.
  for (i = num_stages; i < _num_active_texture_stages; i++) {
    _glActiveTexture(GL_TEXTURE0 + i);
    GLP(Disable)(GL_TEXTURE_1D);
    GLP(Disable)(GL_TEXTURE_2D);
    if (_supports_3d_texture) {
      GLP(Disable)(GL_TEXTURE_3D);
    }
    if (_supports_cube_map) {
      GLP(Disable)(GL_TEXTURE_CUBE_MAP);
    }
  }
  
  // Save the count of texture stages for next time.
  _num_active_texture_stages = num_stages;
  
  report_my_gl_errors();
}


////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::disable_standard_texture_bindings
//       Access: Private
//  Description:
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
disable_standard_texture_bindings() {
  // Disable the texture stages that are no longer used.
  for (int i = 0; i < _num_active_texture_stages; i++) {
    _glActiveTexture(GL_TEXTURE0 + i);
    GLP(Disable)(GL_TEXTURE_1D);
    GLP(Disable)(GL_TEXTURE_2D);
    if (_supports_3d_texture) {
      GLP(Disable)(GL_TEXTURE_3D);
    }
    if (_supports_cube_map) {
      GLP(Disable)(GL_TEXTURE_CUBE_MAP);
    }
  }
  
  _num_active_texture_stages = 0;
  
  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::do_issue_tex_matrix
//       Access: Protected
//  Description:
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
do_issue_tex_matrix() {
  nassertv(_num_active_texture_stages <= _max_texture_stages);

  for (int i = 0; i < _num_active_texture_stages; i++) {
    TextureStage *stage = _effective_texture->get_on_ff_stage(i);
    _glActiveTexture(GL_TEXTURE0 + i);

    GLP(MatrixMode)(GL_TEXTURE);
    if (_target._tex_matrix->has_stage(stage)) {
      GLP(LoadMatrixf)(_target._tex_matrix->get_mat(stage).get_data());
    } else {
      GLP(LoadIdentity)();

      // For some reason, the glLoadIdentity() call doesn't work on
      // my Dell laptop's IBM OpenGL driver, when used in
      // conjunction with glTexGen(), below.  But explicitly loading
      // an identity matrix does work.  But this buggy-driver
      // workaround might have other performance implications, so I
      // leave it out.
      // GLP(LoadMatrixf)(LMatrix4f::ident_mat().get_data());
    }
  }
  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::do_issue_tex_gen
//       Access: Protected
//  Description:
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
do_issue_tex_gen() {
  bool force_normal = false;

  nassertv(_num_active_texture_stages <= _max_texture_stages);

  // These are passed in for the four OBJECT_PLANE or EYE_PLANE
  // values; they effectively define an identity matrix that maps
  // the spatial coordinates one-for-one to UV's.  If you want a
  // mapping other than identity, use a TexMatrixAttrib (or a
  // TexProjectorEffect).
  static const float s_data[4] = { 1, 0, 0, 0 };
  static const float t_data[4] = { 0, 1, 0, 0 };
  static const float r_data[4] = { 0, 0, 1, 0 };
  static const float q_data[4] = { 0, 0, 0, 1 };

  _tex_gen_modifies_mat = false;

  bool got_point_sprites = false;

  for (int i = 0; i < _num_active_texture_stages; i++) {
    TextureStage *stage = _effective_texture->get_on_ff_stage(i);
    _glActiveTexture(GL_TEXTURE0 + i);
    GLP(Disable)(GL_TEXTURE_GEN_S);
    GLP(Disable)(GL_TEXTURE_GEN_T);
    GLP(Disable)(GL_TEXTURE_GEN_R);
    GLP(Disable)(GL_TEXTURE_GEN_Q);
    if (_supports_point_sprite) {
      GLP(TexEnvi)(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_FALSE);
    }

    TexGenAttrib::Mode mode = _effective_tex_gen->get_mode(stage);
    switch (mode) {
    case TexGenAttrib::M_off:
    case TexGenAttrib::M_light_vector:
      break;

    case TexGenAttrib::M_eye_sphere_map:
      GLP(TexGeni)(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
      GLP(TexGeni)(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
      GLP(Enable)(GL_TEXTURE_GEN_S);
      GLP(Enable)(GL_TEXTURE_GEN_T);
      force_normal = true;
      break;

    case TexGenAttrib::M_eye_cube_map:
      if (_supports_cube_map) {
        // We need to rotate the normals out of GL's coordinate
        // system and into the user's coordinate system.  We do this
        // by composing a transform onto the texture matrix.
        LMatrix4f mat = _inv_cs_transform->get_mat();
        mat.set_row(3, LVecBase3f(0.0f, 0.0f, 0.0f));
        GLP(MatrixMode)(GL_TEXTURE);
        GLP(MultMatrixf)(mat.get_data());

        // Now we need to reset the texture matrix next time
        // around to undo this.
        _tex_gen_modifies_mat = true;

        GLP(TexGeni)(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
        GLP(TexGeni)(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
        GLP(TexGeni)(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
        GLP(Enable)(GL_TEXTURE_GEN_S);
        GLP(Enable)(GL_TEXTURE_GEN_T);
        GLP(Enable)(GL_TEXTURE_GEN_R);
        force_normal = true;
      }
      break;

    case TexGenAttrib::M_world_cube_map:
      if (_supports_cube_map) {
        // We dynamically transform normals from eye space to world
        // space by applying the appropriate rotation transform to
        // the current texture matrix.  Unlike M_world_position, we
        // can't achieve this effect by monkeying with the modelview
        // transform, since the current modelview doesn't affect
        // GL_REFLECTION_MAP.
        CPT(TransformState) camera_transform = _scene_setup->get_camera_transform()->compose(_inv_cs_transform);

        LMatrix4f mat = camera_transform->get_mat();
        mat.set_row(3, LVecBase3f(0.0f, 0.0f, 0.0f));
        GLP(MatrixMode)(GL_TEXTURE);
        GLP(MultMatrixf)(mat.get_data());

        // Now we need to reset the texture matrix next time
        // around to undo this.
        _tex_gen_modifies_mat = true;

        GLP(TexGeni)(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
        GLP(TexGeni)(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
        GLP(TexGeni)(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
        GLP(Enable)(GL_TEXTURE_GEN_S);
        GLP(Enable)(GL_TEXTURE_GEN_T);
        GLP(Enable)(GL_TEXTURE_GEN_R);
        force_normal = true;
      }
      break;

    case TexGenAttrib::M_eye_normal:
      if (_supports_cube_map) {
        // We need to rotate the normals out of GL's coordinate
        // system and into the user's coordinate system.  We do this
        // by composing a transform onto the texture matrix.
        LMatrix4f mat = _inv_cs_transform->get_mat();
        mat.set_row(3, LVecBase3f(0.0f, 0.0f, 0.0f));
        GLP(MatrixMode)(GL_TEXTURE);
        GLP(MultMatrixf)(mat.get_data());

        // Now we need to reset the texture matrix next time
        // around to undo this.
        _tex_gen_modifies_mat = true;

        GLP(TexGeni)(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP);
        GLP(TexGeni)(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP);
        GLP(TexGeni)(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP);
        GLP(Enable)(GL_TEXTURE_GEN_S);
        GLP(Enable)(GL_TEXTURE_GEN_T);
        GLP(Enable)(GL_TEXTURE_GEN_R);
        force_normal = true;
      }
      break;

    case TexGenAttrib::M_world_normal:
      if (_supports_cube_map) {
        // We dynamically transform normals from eye space to world
        // space by applying the appropriate rotation transform to
        // the current texture matrix.  Unlike M_world_position, we
        // can't achieve this effect by monkeying with the modelview
        // transform, since the current modelview doesn't affect
        // GL_NORMAL_MAP.
        CPT(TransformState) camera_transform = _scene_setup->get_camera_transform()->compose(_inv_cs_transform);

        LMatrix4f mat = camera_transform->get_mat();
        mat.set_row(3, LVecBase3f(0.0f, 0.0f, 0.0f));
        GLP(MatrixMode)(GL_TEXTURE);
        GLP(MultMatrixf)(mat.get_data());

        // Now we need to reset the texture matrix next time
        // around to undo this.
        _tex_gen_modifies_mat = true;

        GLP(TexGeni)(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP);
        GLP(TexGeni)(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP);
        GLP(TexGeni)(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP);
        GLP(Enable)(GL_TEXTURE_GEN_S);
        GLP(Enable)(GL_TEXTURE_GEN_T);
        GLP(Enable)(GL_TEXTURE_GEN_R);
        force_normal = true;
      }
      break;

    case TexGenAttrib::M_eye_position:
      // To represent eye position correctly, we need to temporarily
      // load the coordinate-system transform.
      GLP(MatrixMode)(GL_MODELVIEW);
      GLP(PushMatrix)();
      GLP(LoadMatrixf)(_cs_transform->get_mat().get_data());

      GLP(TexGeni)(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
      GLP(TexGeni)(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
      GLP(TexGeni)(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
      GLP(TexGeni)(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);

      GLP(TexGenfv)(GL_S, GL_EYE_PLANE, s_data);
      GLP(TexGenfv)(GL_T, GL_EYE_PLANE, t_data);
      GLP(TexGenfv)(GL_R, GL_EYE_PLANE, r_data);
      GLP(TexGenfv)(GL_Q, GL_EYE_PLANE, q_data);

      GLP(Enable)(GL_TEXTURE_GEN_S);
      GLP(Enable)(GL_TEXTURE_GEN_T);
      GLP(Enable)(GL_TEXTURE_GEN_R);
      GLP(Enable)(GL_TEXTURE_GEN_Q);

      GLP(MatrixMode)(GL_MODELVIEW);
      GLP(PopMatrix)();
      break;

    case TexGenAttrib::M_world_position:
      // We achieve world position coordinates by using the eye
      // position mode, and loading the transform of the root
      // node--thus putting the "eye" at the root.
      {
        GLP(MatrixMode)(GL_MODELVIEW);
        GLP(PushMatrix)();
        CPT(TransformState) root_transform = _cs_transform->compose(_scene_setup->get_world_transform());
        GLP(LoadMatrixf)(root_transform->get_mat().get_data());
        GLP(TexGeni)(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
        GLP(TexGeni)(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
        GLP(TexGeni)(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
        GLP(TexGeni)(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);

        GLP(TexGenfv)(GL_S, GL_EYE_PLANE, s_data);
        GLP(TexGenfv)(GL_T, GL_EYE_PLANE, t_data);
        GLP(TexGenfv)(GL_R, GL_EYE_PLANE, r_data);
        GLP(TexGenfv)(GL_Q, GL_EYE_PLANE, q_data);

        GLP(Enable)(GL_TEXTURE_GEN_S);
        GLP(Enable)(GL_TEXTURE_GEN_T);
        GLP(Enable)(GL_TEXTURE_GEN_R);
        GLP(Enable)(GL_TEXTURE_GEN_Q);

        GLP(MatrixMode)(GL_MODELVIEW);
        GLP(PopMatrix)();
      }
      break;

    case TexGenAttrib::M_point_sprite:
      if (_supports_point_sprite) {
        GLP(TexEnvi)(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
        got_point_sprites = true;
      }
      break;

    case TexGenAttrib::M_constant:
      // To generate a constant UV(w) coordinate everywhere, we use
      // EYE_LINEAR mode, but we construct a special matrix that
      // flattens the vertex position to zero and then adds our
      // desired value.
      {
        const TexCoord3f &v = _effective_tex_gen->get_constant_value(stage);

        GLP(TexGeni)(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
        GLP(TexGeni)(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
        GLP(TexGeni)(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
        GLP(TexGeni)(GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

        LVecBase4f s(0.0f, 0.0f, 0.0f, v[0]);
        LVecBase4f t(0.0f, 0.0f, 0.0f, v[1]);
        LVecBase4f r(0.0f, 0.0f, 0.0f, v[2]);

        GLP(TexGenfv)(GL_S, GL_OBJECT_PLANE, s.get_data());
        GLP(TexGenfv)(GL_T, GL_OBJECT_PLANE, t.get_data());
        GLP(TexGenfv)(GL_R, GL_OBJECT_PLANE, r.get_data());
        GLP(TexGenfv)(GL_Q, GL_OBJECT_PLANE, q_data);

        GLP(Enable)(GL_TEXTURE_GEN_S);
        GLP(Enable)(GL_TEXTURE_GEN_T);
        GLP(Enable)(GL_TEXTURE_GEN_R);
        GLP(Enable)(GL_TEXTURE_GEN_Q);
      }
      break;

    case TexGenAttrib::M_unused:
      break;
    }
  }

  if (got_point_sprites != _tex_gen_point_sprite) {
    _tex_gen_point_sprite = got_point_sprites;
    if (_tex_gen_point_sprite) {
      GLP(Enable)(GL_POINT_SPRITE_ARB);
    } else {
      GLP(Disable)(GL_POINT_SPRITE_ARB);
    }
  }

  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::specify_texture
//       Access: Protected
//  Description: Specifies the texture parameters.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
specify_texture(Texture *tex) {
  GLenum target = get_texture_target(tex->get_texture_type());
  if (target == GL_NONE) {
    // Unsupported target (e.g. 3-d texturing on GL 1.1).
    return;
  }

  GLP(TexParameteri)(target, GL_TEXTURE_WRAP_S,
                     get_texture_wrap_mode(tex->get_wrap_u()));
  if (target != GL_TEXTURE_1D) {
    GLP(TexParameteri)(target, GL_TEXTURE_WRAP_T,
                       get_texture_wrap_mode(tex->get_wrap_v()));
  }
  if (target == GL_TEXTURE_3D) {
    GLP(TexParameteri)(target, GL_TEXTURE_WRAP_R,
                       get_texture_wrap_mode(tex->get_wrap_w()));
  }

  Colorf border_color = tex->get_border_color();
  GLP(TexParameterfv)(target, GL_TEXTURE_BORDER_COLOR,
                      border_color.get_data());

  Texture::FilterType minfilter = tex->get_minfilter();
  Texture::FilterType magfilter = tex->get_magfilter();
  bool uses_mipmaps = tex->uses_mipmaps() && !CLP(ignore_mipmaps);

#ifndef NDEBUG
  if (CLP(force_mipmaps)) {
    minfilter = Texture::FT_linear_mipmap_linear;
    magfilter = Texture::FT_linear;
    uses_mipmaps = true;
  }
#endif

  if (!tex->might_have_ram_image()) {
    // If it's a dynamically generated texture (that is, the RAM image
    // isn't available so it didn't pass through the CPU), we should
    // enable GL-generated mipmaps here if we can.
    if (_supports_generate_mipmap) {
      GLP(TexParameteri)(target, GL_GENERATE_MIPMAP, uses_mipmaps);

    } else {
      // Otherwise, don't try to use mipmaps.
      uses_mipmaps = false;
    }
  }

  GLP(TexParameteri)(target, GL_TEXTURE_MIN_FILTER,
                     get_texture_filter_type(minfilter, tex->get_format(), !uses_mipmaps));
  GLP(TexParameteri)(target, GL_TEXTURE_MAG_FILTER,
                     get_texture_filter_type(magfilter, tex->get_format(), true));

  if (tex->get_format() == Texture::F_depth_stencil) {
    GLP(TexParameteri)(target, GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY);
    if (_supports_shadow_filter) {
      if ((tex->get_magfilter() == Texture::FT_shadow) ||
          (tex->get_minfilter() == Texture::FT_shadow)) {
        GLP(TexParameteri)(target, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
        GLP(TexParameteri)(target, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
      } else {
        GLP(TexParameteri)(target, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
        GLP(TexParameteri)(target, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
      }
    }
  }

  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::apply_texture
//       Access: Protected
//  Description: Updates OpenGL with the current information for this
//               texture, and makes it the current texture available
//               for rendering.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
apply_texture(TextureContext *tc) {
  CLP(TextureContext) *gtc = DCAST(CLP(TextureContext), tc);

  gtc->set_active(true);
  GLenum target = get_texture_target(gtc->get_texture()->get_texture_type());
  if (target == GL_NONE) {
    return;
  }
  GLP(BindTexture)(target, gtc->_index);

  if (gtc->was_image_modified()) {
    // If the texture image was modified, reload the texture.  This
    // means we also re-specify the properties for good measure.
    specify_texture(gtc->get_texture());
    upload_texture(gtc);
    gtc->mark_loaded();

  } else if (gtc->was_properties_modified()) {
    // If only the properties have been modified, we don't necessarily
    // need to reload the texture.
    specify_texture(gtc->get_texture());
    gtc->mark_loaded();
  }

  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::upload_texture
//       Access: Protected
//  Description: Uploads the entire texture image to OpenGL, including
//               all pages.
//
//               The return value is true if successful, or false if
//               the texture has no image.
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
upload_texture(CLP(TextureContext) *gtc) {
  Texture *tex = gtc->get_texture();

  CPTA_uchar image = tex->get_ram_image();

  Texture::CompressionMode image_compression;
  if (image.is_null()) {
    image_compression = Texture::CM_off;
  } else {
    image_compression = tex->get_ram_image_compression();
  }

  int mipmap_bias = 0;

  int width = tex->get_x_size();
  int height = tex->get_y_size();
  int depth = tex->get_z_size();

  GLint internal_format = get_internal_image_format(tex);
  GLint external_format = get_external_image_format(tex);
  GLenum component_type = get_component_type(tex->get_component_type());

  // Ensure that the texture fits within the GL's specified limits.
  int max_dimension;
  switch (tex->get_texture_type()) {
  case Texture::TT_3d_texture:
    max_dimension = _max_3d_texture_dimension;
    break;

  case Texture::TT_cube_map:
    max_dimension = _max_cube_map_dimension;
    break;

  default:
    max_dimension = _max_texture_dimension;
  }

  if (max_dimension == 0) {
    // Guess this GL doesn't support cube mapping/3d textures.
    report_my_gl_errors();
    return false;
  }

  // If it doesn't fit, we have to reduce it on-the-fly.  We do this
  // by incrementing the mipmap_bias, so we're effectively loading a
  // lower mipmap level.  This requires generating the mipmaps on
  // the CPU if they haven't already been generated.  It would have
  // been better if the user had specified max-texture-dimension to
  // reduce the texture at load time instead; of course, the user
  // doesn't always know ahead of time what the hardware limits are.

  if (max_dimension > 0 && image_compression == Texture::CM_off) {
    while (tex->get_expected_mipmap_x_size(mipmap_bias) > max_dimension ||
           tex->get_expected_mipmap_y_size(mipmap_bias) > max_dimension ||
           tex->get_expected_mipmap_z_size(mipmap_bias) > max_dimension) {
      ++mipmap_bias;
    }

    if (mipmap_bias >= tex->get_num_ram_mipmap_images()) {
      // We need to generate some more mipmap images.
      if (tex->has_ram_image()) {
        tex->generate_ram_mipmap_images();
        if (mipmap_bias >= tex->get_num_ram_mipmap_images()) {
          // It didn't work.  Send the smallest we've got, and hope
          // for the best.
          mipmap_bias = tex->get_num_ram_mipmap_images() - 1;
        }
      }
    }

    if (mipmap_bias != 0) {
      GLCAT.info()
        << "Reducing image " << tex->get_name()
        << " from " << width << " x " << height << " x " << depth << " to "
        << tex->get_expected_mipmap_x_size(mipmap_bias) << " x "
        << tex->get_expected_mipmap_y_size(mipmap_bias) << " x "
        << tex->get_expected_mipmap_z_size(mipmap_bias) << "\n";
    }
  }

  GLP(PixelStorei)(GL_UNPACK_ALIGNMENT, 1);

  bool uses_mipmaps = (tex->uses_mipmaps() && !CLP(ignore_mipmaps)) || CLP(force_mipmaps);

#ifndef NDEBUG
  if (CLP(force_mipmaps)) {
    uses_mipmaps = true;
  }
#endif

  bool success = true;

  if (tex->get_texture_type() == Texture::TT_cube_map) {
    // A cube map must load six different 2-d images (which are stored
    // as the six pages of the system ram image).
    if (!_supports_cube_map) {
      report_my_gl_errors();
      return false;
    }

    success = success && upload_texture_image
      (gtc, uses_mipmaps, mipmap_bias,
       GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_POSITIVE_X,
       internal_format, external_format, component_type,
       true, 0, image_compression);

    success = success && upload_texture_image
      (gtc, uses_mipmaps, mipmap_bias,
       GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
       internal_format, external_format, component_type,
       true, 1, image_compression);

    success = success && upload_texture_image
      (gtc, uses_mipmaps, mipmap_bias,
       GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
       internal_format, external_format, component_type,
       true, 2, image_compression);

    success = success && upload_texture_image
      (gtc, uses_mipmaps, mipmap_bias,
       GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
       internal_format, external_format, component_type,
       true, 3, image_compression);

    success = success && upload_texture_image
      (gtc, uses_mipmaps, mipmap_bias,
       GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
       internal_format, external_format, component_type,
       true, 4, image_compression);

    success = success && upload_texture_image
      (gtc, uses_mipmaps, mipmap_bias,
       GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
       internal_format, external_format, component_type,
       true, 5, image_compression);

  } else {
    // Any other kind of texture can be loaded all at once.
    GLenum target = get_texture_target(tex->get_texture_type());
    success = upload_texture_image
      (gtc, uses_mipmaps, mipmap_bias, target, target,
       internal_format, external_format, component_type,
       false, 0, image_compression);
  }

  maybe_gl_finish();

  if (success) {
    gtc->_already_applied = true;
    gtc->_uses_mipmaps = uses_mipmaps;
    gtc->_internal_format = internal_format;
    gtc->_width = width;
    gtc->_height = height;
    gtc->_depth = depth;

#ifdef DO_PSTATS
    if (!image.is_null()) {
      gtc->update_data_size_bytes(get_texture_memory_size(tex));
    }
#endif

    tex->texture_uploaded();

    report_my_gl_errors();
    return true;
  }

  report_my_gl_errors();
  return false;
}


////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::upload_texture_image
//       Access: Protected
//  Description: Loads a texture image, or one page of a cube map
//               image, from system RAM to texture memory.
//
//               texture_target is normally the same thing as
//               page_target; both represent the GL target onto which
//               the texture image is loaded, e.g. GL_TEXTURE_1D,
//               GL_TEXTURE_2D, etc.  The only time they may differ is
//               in the case of cube mapping, in which case
//               texture_target will be target for the overall
//               texture, e.g. GL_TEXTURE_CUBE_MAP, and page_target
//               will the target for this particular page,
//               e.g. GL_TEXTURE_CUBE_MAP_POSITIVE_X.
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
upload_texture_image(CLP(TextureContext) *gtc,
                     bool uses_mipmaps, int mipmap_bias,
                     GLenum texture_target, GLenum page_target,
                     GLint internal_format,
                     GLint external_format, GLenum component_type,
                     bool one_page_only, int z,
                     Texture::CompressionMode image_compression) {
  // Make sure the error stack is cleared out before we begin.
  report_my_gl_errors();

  if (texture_target == GL_NONE) {
    // Unsupported target (e.g. 3-d texturing on GL 1.1).
    return false;
  }
  if (image_compression != Texture::CM_off && !_supports_compressed_texture) {
    return false;
  }

  PStatTimer timer(_load_texture_pcollector);
  Texture *tex = gtc->get_texture();
  nassertr(tex != (Texture *)NULL, false);

  CPTA_uchar image = tex->get_ram_mipmap_image(mipmap_bias);
  int width = tex->get_expected_mipmap_x_size(mipmap_bias);
  int height = tex->get_expected_mipmap_y_size(mipmap_bias);
  int depth = tex->get_expected_mipmap_z_size(mipmap_bias);

  if (GLCAT.is_debug()) {
    if (image_compression != Texture::CM_off) {
      GLCAT.debug()
        << "loading pre-compressed texture " << tex->get_name() << "\n";
    } else if (is_compressed_format(internal_format)) {
      GLCAT.debug()
        << "compressing texture " << tex->get_name() << "\n";
    } else {
      GLCAT.debug()
        << "loading uncompressed texture " << tex->get_name() << "\n";
    }
  }

  int num_ram_mipmap_levels = 0;
  bool load_ram_mipmaps = false;

  if (image.is_null()) {
    if (uses_mipmaps) {
      if (_supports_generate_mipmap) {
        GLP(TexParameteri)(texture_target, GL_GENERATE_MIPMAP, true);
      } else {
        // If it can't, do without mipmaps.
        GLP(TexParameteri)(texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        uses_mipmaps = false;
      }
    }
  } else {
    num_ram_mipmap_levels = 1;
    if (uses_mipmaps) {
      num_ram_mipmap_levels = tex->get_num_ram_mipmap_images();

      if (num_ram_mipmap_levels == 1) {
        // No RAM mipmap levels available.  Should we generate some?
        if (!_supports_generate_mipmap ||
            (!auto_generate_mipmaps && image_compression == Texture::CM_off)) {
          // Yes, the GL won't generate them, so we need to.
          tex->generate_ram_mipmap_images();
          num_ram_mipmap_levels = tex->get_num_ram_mipmap_images();
        }
      }

      if (num_ram_mipmap_levels != 1) {
        // We will load the mipmap levels from RAM.  Don't ask the GL to
        // generate them.
        if (_supports_generate_mipmap) {
          GLP(TexParameteri)(texture_target, GL_GENERATE_MIPMAP, false);
        }
        load_ram_mipmaps = true;

      } else {
        // We don't have mipmap levels in RAM.  Ask the GL to generate
        // them if it can.
        if (_supports_generate_mipmap) {
          GLP(TexParameteri)(texture_target, GL_GENERATE_MIPMAP, true);
        } else {
          // If it can't, do without mipmaps.
          GLP(TexParameteri)(texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          uses_mipmaps = false;
        }
      }
    }
  }

  int highest_level = 0;

  if (!gtc->_already_applied ||
      gtc->_uses_mipmaps != uses_mipmaps ||
      gtc->_internal_format != internal_format ||
      gtc->_width != width ||
      gtc->_height != height ||
      gtc->_depth != depth) {
    // We need to reload a new image.

    if (num_ram_mipmap_levels == 0) {
      if (external_format == GL_DEPTH_STENCIL_EXT) {
        GLP(TexImage2D)(page_target, 0, internal_format,
                        width, height, 0,
                        external_format, GL_UNSIGNED_INT_24_8_EXT, NULL);
      } else {
        GLP(TexImage2D)(page_target, 0, internal_format,
                        width, height, 0,
                        external_format, GL_UNSIGNED_BYTE, NULL);
      }
    }
    
    for (int n = mipmap_bias; n < num_ram_mipmap_levels; ++n) {
      CPTA_uchar ptimage = tex->get_ram_mipmap_image(n);
      if (ptimage == (const unsigned char *)NULL) {
        GLCAT.warning()
          << "No mipmap level " << n << " defined for " << tex->get_name()
          << "\n";
        // No mipmap level n; stop here.
        break;
      }
      const unsigned char *image_ptr = ptimage;

      size_t image_size = tex->get_ram_mipmap_image_size(n);
      if (one_page_only) {
        image_size = tex->get_ram_mipmap_page_size(n);
        image_ptr += image_size * z;
      }

      PTA_uchar bgr_image;
      if (!_supports_bgr && image_compression == Texture::CM_off) {
        // If the GL doesn't claim to support BGR, we may have to reverse
        // the component ordering of the image.
        image_ptr = fix_component_ordering(bgr_image, image_ptr, image_size,
                                           external_format, tex);
      }

      int width = tex->get_expected_mipmap_x_size(n);
      int height = tex->get_expected_mipmap_y_size(n);
      int depth = tex->get_expected_mipmap_z_size(n);

#ifdef DO_PSTATS
      _data_transferred_pcollector.add_level(image_size);
#endif
      switch (texture_target) {
      case GL_TEXTURE_1D:
        if (image_compression == Texture::CM_off) {
          GLP(TexImage1D)(page_target, n - mipmap_bias, internal_format,
                          width, 0,
                          external_format, component_type, image_ptr);
        } else {
          _glCompressedTexImage1D(page_target, n - mipmap_bias, external_format, width,
                                  0, image_size, image_ptr);
        }
        break;

      case GL_TEXTURE_3D:
        if (_supports_3d_texture) {
          if (image_compression == Texture::CM_off) {
            _glTexImage3D(page_target, n - mipmap_bias, internal_format,
                          width, height, depth, 0,
                          external_format, component_type, image_ptr);
          } else {
            _glCompressedTexImage3D(page_target, n - mipmap_bias, external_format, width,
                                    height, depth,
                                    0, image_size, image_ptr);
          }
        } else {
          report_my_gl_errors();
          return false;
        }
        break;

      default:
        if (image_compression == Texture::CM_off) {
          GLP(TexImage2D)(page_target, n - mipmap_bias, internal_format,
                          width, height, 0,
                          external_format, component_type, image_ptr);
        } else {
          _glCompressedTexImage2D(page_target, n - mipmap_bias, external_format, width, height,
                                  0, image_size, image_ptr);
        }
      }

      highest_level = n;
    }
  } else {
    // We can reload the image over the previous image, possibly
    // saving on texture memory fragmentation.
    for (int n = mipmap_bias; n < num_ram_mipmap_levels; ++n) {
      const unsigned char *image_ptr = tex->get_ram_mipmap_image(n);
      if (image_ptr == (const unsigned char *)NULL) {
        GLCAT.warning()
          << "No mipmap level " << n << " defined for " << tex->get_name()
          << "\n";
        // No mipmap level n; stop here.
        break;
      }

      size_t image_size = tex->get_ram_mipmap_image_size(n);
      if (one_page_only) {
        image_size = tex->get_ram_mipmap_page_size(n);
        image_ptr += image_size * z;
      }

      PTA_uchar bgr_image;
      if (!_supports_bgr && image_compression == Texture::CM_off) {
        // If the GL doesn't claim to support BGR, we may have to reverse
        // the component ordering of the image.
        image_ptr = fix_component_ordering(bgr_image, image_ptr, image_size,
                                           external_format, tex);
      }

      int width = tex->get_expected_mipmap_x_size(n);
      int height = tex->get_expected_mipmap_y_size(n);
      int depth = tex->get_expected_mipmap_z_size(n);

#ifdef DO_PSTATS
      _data_transferred_pcollector.add_level(image_size);
#endif
      switch (texture_target) {
      case GL_TEXTURE_1D:
        if (image_compression == Texture::CM_off) {
          GLP(TexSubImage1D)(page_target, n - mipmap_bias, 0, width,
                             external_format, component_type, image_ptr);
        } else {
          _glCompressedTexSubImage1D(page_target, n - mipmap_bias, 0, width,
                                     external_format, image_size, image_ptr);
        }
        break;

      case GL_TEXTURE_3D:
        if (_supports_3d_texture) {
          if (image_compression == Texture::CM_off) {
            _glTexSubImage3D(page_target, n - mipmap_bias, 0, 0, 0, width, height, depth,
                             external_format, component_type, image_ptr);
          } else {
            _glCompressedTexSubImage3D(page_target, n - mipmap_bias, 0, 0, 0, width, height, depth,
                                       external_format, image_size, image_ptr);
          }
        } else {
          report_my_gl_errors();
          return false;
        }
        break;

      default:
        if (image_compression == Texture::CM_off) {
          if (n==0) {
            // It's unfortunate that we can't adjust the width, too,
            // but TexSubImage2D doesn't accept a row-stride parameter.
            height = tex->get_y_size() - tex->get_pad_y_size();
          }
          GLP(TexSubImage2D)(page_target, n - mipmap_bias, 0, 0, width, height,
                             external_format, component_type, image_ptr);
        } else {
          _glCompressedTexSubImage2D(page_target, n - mipmap_bias, 0, 0, width, height,
                                     external_format, image_size, image_ptr);
        }
        break;
      }

      highest_level = n;
    }
  }

  if (is_at_least_version(1, 2)) {
    if (load_ram_mipmaps) {
      // By the time we get here, we have successfully loaded a certain
      // number of mipmap levels.  Tell the GL that's all it's going to
      // get.
      GLP(TexParameteri)(texture_target, GL_TEXTURE_MAX_LEVEL, highest_level - mipmap_bias);
      
    } else if (uses_mipmaps) {
      // Since the mipmap levels were auto-generated and are therefore
      // complete, make sure the GL doesn't remember some previous value
      // for GL_TEXTURE_MAX_LEVEL from the above call--set it to the
      // full count of mipmap levels.
      GLP(TexParameteri)(texture_target, GL_TEXTURE_MAX_LEVEL, tex->get_expected_num_mipmap_levels() - mipmap_bias - 1);
    }
  }

  // Report the error message explicitly if the GL texture creation
  // failed.
  GLenum error_code = GLP(GetError)();
  if (error_code != GL_NO_ERROR) {
    GLCAT.error()
      << "GL texture creation failed for " << tex->get_name()
      << " : " << get_error_string(error_code) << "\n";

    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::get_texture_memory_size
//       Access: Protected
//  Description: Asks OpenGL how much texture memory is consumed by
//               the indicated texture (which is also the
//               currently-selected texture).
////////////////////////////////////////////////////////////////////
size_t CLP(GraphicsStateGuardian)::
get_texture_memory_size(Texture *tex) {
  GLenum target = get_texture_target(tex->get_texture_type());

  GLenum page_target = target;
  GLint scale = 1;
  if (target == GL_TEXTURE_CUBE_MAP) {
    // We need a particular page to get the level parameter from.
    page_target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    scale = 6;
  }

  GLint minfilter;
  GLP(GetTexParameteriv)(target, GL_TEXTURE_MIN_FILTER, &minfilter);

  GLint internal_format;
  GLP(GetTexLevelParameteriv)(page_target, 0, GL_TEXTURE_INTERNAL_FORMAT, &internal_format);

  report_my_gl_errors();

  if (is_compressed_format(internal_format)) {
    // Try to get the compressed size.
    GLint image_size;
    GLP(GetTexLevelParameteriv)(page_target, 0,
                                GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &image_size);

    GLenum error_code = GLP(GetError)();
    if (error_code != GL_NO_ERROR) {
      if (GLCAT.is_debug()) {
        GLCAT.debug()
          << "Couldn't get compressed size for " << tex->get_name()
          << " : " << get_error_string(error_code) << "\n";
      }
      // Fall through to the noncompressed case.
    } else {
      return image_size * scale;
    }
  }

  // OK, get the noncompressed size.
  GLint red_size, green_size, blue_size, alpha_size,
    luminance_size, intensity_size;
  GLint depth_size = 0;
  GLP(GetTexLevelParameteriv)(page_target, 0,
                              GL_TEXTURE_RED_SIZE, &red_size);
  GLP(GetTexLevelParameteriv)(page_target, 0,
                              GL_TEXTURE_GREEN_SIZE, &green_size);
  GLP(GetTexLevelParameteriv)(page_target, 0,
                              GL_TEXTURE_BLUE_SIZE, &blue_size);
  GLP(GetTexLevelParameteriv)(page_target, 0,
                              GL_TEXTURE_ALPHA_SIZE, &alpha_size);
  GLP(GetTexLevelParameteriv)(page_target, 0,
                              GL_TEXTURE_LUMINANCE_SIZE, &luminance_size);
  GLP(GetTexLevelParameteriv)(page_target, 0,
                              GL_TEXTURE_INTENSITY_SIZE, &intensity_size);
  if (_supports_depth_texture) {
    // Actually, this seems to cause problems on some Mesa versions,
    // even though they advertise GL_ARB_depth_texture.  Who needs it.
    //    GLP(GetTexLevelParameteriv)(page_target, 0,
    //        GL_TEXTURE_DEPTH_SIZE, &depth_size);
  }

  GLint width = 1, height = 1, depth = 1;
  GLP(GetTexLevelParameteriv)(page_target, 0, GL_TEXTURE_WIDTH, &width);
  GLP(GetTexLevelParameteriv)(page_target, 0, GL_TEXTURE_HEIGHT, &height);
  if (_supports_3d_texture) {
    GLP(GetTexLevelParameteriv)(page_target, 0, GL_TEXTURE_DEPTH, &depth);
  }

  report_my_gl_errors();

  size_t num_bits = (red_size + green_size + blue_size + alpha_size + luminance_size + intensity_size + depth_size);
  size_t num_bytes = (num_bits + 7) / 8;

  size_t result = num_bytes * width * height * depth * scale;
  if (is_mipmap_filter(minfilter)) {
    result = (result * 4) / 3;
  }

  return result;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::check_nonresident_texture
//       Access: Private
//  Description: Checks the list of resident texture objects to see if
//               any have recently been evicted.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
check_nonresident_texture(BufferContextChain &chain) {
  size_t num_textures = chain.get_count();
  if (num_textures == 0) {
    return;
  }

  CLP(TextureContext) **gtc_list = (CLP(TextureContext) **)alloca(num_textures * sizeof(CLP(TextureContext) *));
  GLuint *texture_list = (GLuint *)alloca(num_textures * sizeof(GLuint));
  size_t ti = 0;
  BufferContext *node = chain.get_first();
  while (node != (BufferContext *)NULL) {
    CLP(TextureContext) *gtc = DCAST(CLP(TextureContext), node);
    gtc_list[ti] = gtc;
    texture_list[ti] = gtc->_index;
    node = node->get_next();
    ++ti;
  }
  nassertv(ti == num_textures);
  GLboolean *results = (GLboolean *)alloca(num_textures * sizeof(GLboolean));
  bool all_resident = (GLP(AreTexturesResident)(num_textures, texture_list, results) != 0);

  report_my_gl_errors();

  if (!all_resident) {
    // Some are now nonresident.
    for (ti = 0; ti < num_textures; ++ti) {
      if (!results[ti]) {
        gtc_list[ti]->set_resident(false);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::extract_texture_image
//       Access: Protected
//  Description: Called from extract_texture_data(), this gets just
//               the image array for a particular mipmap level (or for
//               the base image).
////////////////////////////////////////////////////////////////////
bool CLP(GraphicsStateGuardian)::
extract_texture_image(PTA_uchar &image, size_t &page_size,
                      Texture *tex, GLenum target, GLenum page_target,
                      Texture::ComponentType type,
                      Texture::CompressionMode compression, int n) {
  if (target == GL_TEXTURE_CUBE_MAP) {
    // A cube map, compressed or uncompressed.  This we must extract
    // one page at a time.

    // If the cube map is compressed, we assume that all the
    // compressed pages are exactly the same size.  OpenGL doesn't
    // make this assumption, but it happens to be true for all
    // currently extant compression schemes, and it makes things
    // simpler for us.  (It also makes things much simpler for the
    // graphics hardware, so it's likely to continue to be true for a
    // while at least.)

    GLenum external_format = get_external_image_format(tex);
    GLenum pixel_type = get_component_type(type);
    page_size = tex->get_expected_ram_mipmap_page_size(n);

    if (compression != Texture::CM_off) {
      GLint image_size;
      GLP(GetTexLevelParameteriv)(page_target, n,
                                  GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &image_size);
      nassertr(image_size <= (int)page_size, false);
      page_size = image_size;
    }

    image = PTA_uchar::empty_array(page_size * 6);

    for (int z = 0; z < 6; ++z) {
      page_target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + z;

      if (compression == Texture::CM_off) {
        GLP(GetTexImage)(page_target, n, external_format, pixel_type,
                         image.p() + z * page_size);
      } else {
        _glGetCompressedTexImage(page_target, 0, image.p() + z * page_size);
      }
    }

  } else if (compression == Texture::CM_off) {
    // An uncompressed 1-d, 2-d, or 3-d texture.
    image = PTA_uchar::empty_array(tex->get_expected_ram_mipmap_image_size(n));
    GLenum external_format = get_external_image_format(tex);
    GLenum pixel_type = get_component_type(type);
    GLP(GetTexImage)(target, n, external_format, pixel_type, image.p());

  } else {
    // A compressed 1-d, 2-d, or 3-d texture.
    GLint image_size;
    GLP(GetTexLevelParameteriv)(target, n, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &image_size);
    page_size = image_size / tex->get_z_size();
    image = PTA_uchar::empty_array(image_size);
    _glGetCompressedTexImage(target, n, image.p());
  }

  // Now see if we were successful.
  GLenum error_code = GLP(GetError)();
  if (error_code != GL_NO_ERROR) {
    GLCAT.error()
      << "Unable to extract texture for " << *tex
      << ", mipmap level " << n
      << " : " << get_error_string(error_code) << "\n";

    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::do_point_size
//       Access: Protected
//  Description: Internally sets the point size parameters after any
//               of the properties have changed that might affect
//               this.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
do_point_size() {
  if (!_point_perspective) {
    // Normal, constant-sized points.  Here _point_size is a width in
    // pixels.
    static LVecBase3f constant(1.0f, 0.0f, 0.0f);
    _glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, constant.get_data());

  } else {
    // Perspective-sized points.  Here _point_size is a width in 3-d
    // units.  To arrange that, we need to figure out the appropriate
    // scaling factor based on the current viewport and projection
    // matrix.
    LVector3f height(0.0f, _point_size, 1.0f);
    height = height * _projection_mat->get_mat();
    float s = height[1] * _viewport_height / _point_size;

    if (_current_lens->is_orthographic()) {
      // If we have an orthographic lens in effect, we don't actually
      // apply a perspective transform: we just scale the points once,
      // regardless of the distance from the camera.
      LVecBase3f constant(1.0f / (s * s), 0.0f, 0.0f);
      _glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, constant.get_data());

    } else {
      // Otherwise, we give it a true perspective adjustment.
      LVecBase3f square(0.0f, 0.0f, 1.0f / (s * s));
      _glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, square.get_data());
    }
  }

  report_my_gl_errors();
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::bind_fbo
//       Access: Protected
//  Description: Binds an FBO object.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
bind_fbo(GLuint fbo) {
  nassertv(_glBindFramebuffer != 0);
  _glBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo);
  _current_fbo = fbo;
}


////////////////////////////////////////////////////////////////////
//  GL stencil code section
////////////////////////////////////////////////////////////////////

static int gl_stencil_comparison_function_array [ ] = {
  GL_NEVER,
  GL_LESS,
  GL_EQUAL,
  GL_LEQUAL,
  GL_GREATER,
  GL_NOTEQUAL,
  GL_GEQUAL,
  GL_ALWAYS,
};

static int gl_stencil_operations_array [ ] = {
  GL_KEEP,
  GL_ZERO,
  GL_REPLACE,
  GL_INCR_WRAP,
  GL_DECR_WRAP,
  GL_INVERT,
  
  GL_INCR,
  GL_DECR,
};

void __glActiveStencilFace (GraphicsStateGuardian *gsg, GLenum face) {
  CLP(GraphicsStateGuardian) *glgsg;

  glgsg = (CLP(GraphicsStateGuardian) *) gsg;
  if (gsg -> get_supports_two_sided_stencil ( ) &&
      glgsg -> _glActiveStencilFaceEXT) {
    if (face == GL_FRONT) {
      // glActiveStencilFaceEXT (GL_FRONT);
      glgsg -> _glActiveStencilFaceEXT (GL_FRONT);
    }
    else {
      // glActiveStencilFaceEXT (GL_BACK);
      glgsg -> _glActiveStencilFaceEXT (GL_BACK);
    }
  }
}

void gl_front_stencil_function (StencilRenderStates::StencilRenderState stencil_render_state, StencilRenderStates *stencil_render_states) {

  __glActiveStencilFace (stencil_render_states -> _gsg, GL_FRONT);
  glStencilFunc
    (
     gl_stencil_comparison_function_array [stencil_render_states -> get_stencil_render_state (StencilRenderStates::SRS_front_comparison_function)],
     stencil_render_states -> get_stencil_render_state (StencilRenderStates::SRS_reference),
     stencil_render_states -> get_stencil_render_state (StencilRenderStates::SRS_read_mask)
     );
}
void gl_front_stencil_operation (StencilRenderStates::StencilRenderState stencil_render_state, StencilRenderStates *stencil_render_states) {
  __glActiveStencilFace (stencil_render_states -> _gsg, GL_FRONT);
  glStencilOp
    (
     gl_stencil_operations_array [stencil_render_states -> get_stencil_render_state (StencilRenderStates::SRS_front_stencil_fail_operation)],
     gl_stencil_operations_array [stencil_render_states -> get_stencil_render_state (StencilRenderStates::SRS_front_stencil_pass_z_fail_operation)],
     gl_stencil_operations_array [stencil_render_states -> get_stencil_render_state (StencilRenderStates::SRS_front_stencil_pass_z_pass_operation)]
     );
}

void gl_back_stencil_function (StencilRenderStates::StencilRenderState stencil_render_state, StencilRenderStates *stencil_render_states) {

  bool supports_two_sided_stencil;

  supports_two_sided_stencil = stencil_render_states -> _gsg -> get_supports_two_sided_stencil ( );
  if (supports_two_sided_stencil) {
    __glActiveStencilFace (stencil_render_states -> _gsg, GL_BACK);
    glStencilFunc
      (
       gl_stencil_comparison_function_array [stencil_render_states -> get_stencil_render_state (StencilRenderStates::SRS_back_comparison_function)],
       stencil_render_states -> get_stencil_render_state (StencilRenderStates::SRS_reference),
       stencil_render_states -> get_stencil_render_state (StencilRenderStates::SRS_read_mask)
       );
  }
}

void gl_back_stencil_operation (StencilRenderStates::StencilRenderState stencil_render_state, StencilRenderStates *stencil_render_states) {

  bool supports_two_sided_stencil;

  supports_two_sided_stencil = stencil_render_states -> _gsg -> get_supports_two_sided_stencil ( );
  if (supports_two_sided_stencil) {
    __glActiveStencilFace (stencil_render_states -> _gsg, GL_BACK);
    glStencilOp
      (
       gl_stencil_operations_array [stencil_render_states -> get_stencil_render_state (StencilRenderStates::SRS_back_stencil_fail_operation)],
       gl_stencil_operations_array [stencil_render_states -> get_stencil_render_state (StencilRenderStates::SRS_back_stencil_pass_z_fail_operation)],
       gl_stencil_operations_array [stencil_render_states -> get_stencil_render_state (StencilRenderStates::SRS_back_stencil_pass_z_pass_operation)]
       );
  }
}

void gl_front_back_stencil_function (StencilRenderStates::StencilRenderState stencil_render_state, StencilRenderStates *stencil_render_states) {
  gl_front_stencil_function (stencil_render_state, stencil_render_states);
  gl_back_stencil_function (stencil_render_state, stencil_render_states);
}

void gl_stencil_function (StencilRenderStates::StencilRenderState stencil_render_state, StencilRenderStates *stencil_render_states) {

  StencilType render_state_value;
  bool supports_two_sided_stencil;

  supports_two_sided_stencil = stencil_render_states -> _gsg -> get_supports_two_sided_stencil ( );

  render_state_value = stencil_render_states -> get_stencil_render_state (stencil_render_state);
  switch (stencil_render_state) {
  case StencilRenderStates::SRS_front_enable:
    if (render_state_value) {
      glEnable (GL_STENCIL_TEST);
    }
    else {
      glDisable (GL_STENCIL_TEST);
    }
    break;
  case StencilRenderStates::SRS_back_enable:
    if (supports_two_sided_stencil) {
      if (render_state_value) {
        glEnable (GL_STENCIL_TEST_TWO_SIDE_EXT);
      }
      else {
        glDisable (GL_STENCIL_TEST_TWO_SIDE_EXT);
      }
    }
    break;

  case StencilRenderStates::SRS_write_mask:
    glStencilMask (render_state_value);
    break;

  default:
    break;
  }
}

void gl_set_stencil_functions (StencilRenderStates *stencil_render_states) {

  if (stencil_render_states) {
    stencil_render_states -> set_stencil_function (StencilRenderStates::SRS_front_enable, gl_stencil_function);
    stencil_render_states -> set_stencil_function (StencilRenderStates::SRS_back_enable, gl_stencil_function);

    stencil_render_states -> set_stencil_function (StencilRenderStates::SRS_front_comparison_function, gl_front_stencil_function);
    stencil_render_states -> set_stencil_function (StencilRenderStates::SRS_front_stencil_fail_operation,  gl_front_stencil_operation);
    stencil_render_states -> set_stencil_function (StencilRenderStates::SRS_front_stencil_pass_z_fail_operation, gl_front_stencil_operation);
    stencil_render_states -> set_stencil_function (StencilRenderStates::SRS_front_stencil_pass_z_pass_operation, gl_front_stencil_operation);

    // GL seems to support different read masks and/or reference values for front and back, but DX does not.
    // This needs to be cross-platform so do it the DX way by setting the same read mask and reference for both front and back.
    stencil_render_states -> set_stencil_function (StencilRenderStates::SRS_reference, gl_front_back_stencil_function);
    stencil_render_states -> set_stencil_function (StencilRenderStates::SRS_read_mask, gl_front_back_stencil_function);

    stencil_render_states -> set_stencil_function (StencilRenderStates::SRS_write_mask, gl_stencil_function);

    stencil_render_states -> set_stencil_function (StencilRenderStates::SRS_back_comparison_function, gl_back_stencil_function);
    stencil_render_states -> set_stencil_function (StencilRenderStates::SRS_back_stencil_fail_operation, gl_back_stencil_operation);
    stencil_render_states -> set_stencil_function (StencilRenderStates::SRS_back_stencil_pass_z_fail_operation, gl_back_stencil_operation);
    stencil_render_states -> set_stencil_function (StencilRenderStates::SRS_back_stencil_pass_z_pass_operation, gl_back_stencil_operation);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: GLGraphicsStateGuardian::do_issue_stencil
//       Access: Protected
//  Description: Set stencil render states.
////////////////////////////////////////////////////////////////////
void CLP(GraphicsStateGuardian)::
do_issue_stencil() {
  const StencilAttrib *stencil;
  StencilRenderStates *stencil_render_states;

  stencil = _target._stencil;
  stencil_render_states = this -> _stencil_render_states;
  if (stencil && stencil_render_states) {

    // DEBUG
    if (false) {
      GLCAT.debug() << "STENCIL STATE CHANGE\n";
      GLCAT.debug() << "\n"
                    << "SRS_front_enable " << stencil -> get_render_state (StencilAttrib::SRS_front_enable) << "\n"
                    << "SRS_back_enable " << stencil -> get_render_state (StencilAttrib::SRS_back_enable) << "\n"
                    << "SRS_front_comparison_function " << stencil -> get_render_state (StencilAttrib::SRS_front_comparison_function) << "\n"
                    << "SRS_front_stencil_fail_operation " << stencil -> get_render_state (StencilAttrib::SRS_front_stencil_fail_operation) << "\n"
                    << "SRS_front_stencil_pass_z_fail_operation " << stencil -> get_render_state (StencilAttrib::SRS_front_stencil_pass_z_fail_operation) << "\n"
                    << "SRS_front_stencil_pass_z_pass_operation " << stencil -> get_render_state (StencilAttrib::SRS_front_stencil_pass_z_pass_operation) << "\n"
                    << "SRS_reference " << stencil -> get_render_state (StencilAttrib::SRS_reference) << "\n"
                    << "SRS_read_mask " << stencil -> get_render_state (StencilAttrib::SRS_read_mask) << "\n"
                    << "SRS_write_mask " << stencil -> get_render_state (StencilAttrib::SRS_write_mask) << "\n"
                    << "SRS_back_comparison_function " << stencil -> get_render_state (StencilAttrib::SRS_back_comparison_function) << "\n"
                    << "SRS_back_stencil_fail_operation " << stencil -> get_render_state (StencilAttrib::SRS_back_stencil_fail_operation) << "\n"
                    << "SRS_back_stencil_pass_z_fail_operation " << stencil -> get_render_state (StencilAttrib::SRS_back_stencil_pass_z_fail_operation) << "\n"
                    << "SRS_back_stencil_pass_z_pass_operation " << stencil -> get_render_state (StencilAttrib::SRS_back_stencil_pass_z_pass_operation) << "\n";
    }

    {
      bool on;

      on = false;
      stencil_render_states -> set_stencil_render_state (true, StencilRenderStates::SRS_front_enable, stencil -> get_render_state (StencilAttrib::SRS_front_enable));
      if (stencil -> get_render_state (StencilAttrib::SRS_front_enable)) {
        stencil_render_states -> set_stencil_render_state (true, StencilRenderStates::SRS_front_comparison_function, stencil -> get_render_state (StencilAttrib::SRS_front_comparison_function));
        stencil_render_states -> set_stencil_render_state (true, StencilRenderStates::SRS_front_stencil_fail_operation, stencil -> get_render_state (StencilAttrib::SRS_front_stencil_fail_operation));
        stencil_render_states -> set_stencil_render_state (true, StencilRenderStates::SRS_front_stencil_pass_z_fail_operation, stencil -> get_render_state (StencilAttrib::SRS_front_stencil_pass_z_fail_operation));
        stencil_render_states -> set_stencil_render_state (true, StencilRenderStates::SRS_front_stencil_pass_z_pass_operation, stencil -> get_render_state (StencilAttrib::SRS_front_stencil_pass_z_pass_operation));
        on = true;
      }

      stencil_render_states -> set_stencil_render_state (true, StencilRenderStates::SRS_back_enable, stencil -> get_render_state (StencilAttrib::SRS_back_enable));
      if (stencil -> get_render_state (StencilAttrib::SRS_back_enable)) {
        stencil_render_states -> set_stencil_render_state (true, StencilRenderStates::SRS_back_comparison_function, stencil -> get_render_state (StencilAttrib::SRS_back_comparison_function));
        stencil_render_states -> set_stencil_render_state (true, StencilRenderStates::SRS_back_stencil_fail_operation, stencil -> get_render_state (StencilAttrib::SRS_back_stencil_fail_operation));
        stencil_render_states -> set_stencil_render_state (true, StencilRenderStates::SRS_back_stencil_pass_z_fail_operation, stencil -> get_render_state (StencilAttrib::SRS_back_stencil_pass_z_fail_operation));
        stencil_render_states -> set_stencil_render_state (true, StencilRenderStates::SRS_back_stencil_pass_z_pass_operation, stencil -> get_render_state (StencilAttrib::SRS_back_stencil_pass_z_pass_operation));
        on = true;
      }

      if (on) {
        stencil_render_states -> set_stencil_render_state (true, StencilRenderStates::SRS_reference, stencil -> get_render_state (StencilAttrib::SRS_reference));
        stencil_render_states -> set_stencil_render_state (true, StencilRenderStates::SRS_read_mask, stencil -> get_render_state (StencilAttrib::SRS_read_mask));
        stencil_render_states -> set_stencil_render_state (true, StencilRenderStates::SRS_write_mask, stencil -> get_render_state (StencilAttrib::SRS_write_mask));
      }
    }

    if (stencil -> get_render_state (StencilAttrib::SRS_clear)) {    
      GLbitfield mask = 0;
      
      // clear stencil buffer
      GLP(ClearStencil)(stencil -> get_render_state (StencilAttrib::SRS_clear_value));
      mask |= GL_STENCIL_BUFFER_BIT;
      GLP(Clear)(mask);
    }
  }
  else {
    stencil_render_states -> set_stencil_render_state (true, StencilRenderStates::SRS_front_enable, 0);
    stencil_render_states -> set_stencil_render_state (true, StencilRenderStates::SRS_back_enable, 0);
  }
}
