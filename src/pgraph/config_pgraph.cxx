// Filename: config_pgraph.cxx
// Created by:  drose (21Feb02)
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

#include "config_pgraph.h"

#include "alphaTestAttrib.h"
#include "ambientLight.h"
#include "antialiasAttrib.h"
#include "auxSceneData.h"
#include "billboardEffect.h"
#include "camera.h"
#include "clipPlaneAttrib.h"
#include "colorAttrib.h"
#include "colorBlendAttrib.h"
#include "colorScaleAttrib.h"
#include "colorWriteAttrib.h"
#include "compassEffect.h"
#include "cullFaceAttrib.h"
#include "cullBin.h"
#include "cullBinAttrib.h"
#include "cullTraverser.h"
#include "cullableObject.h"
#include "decalEffect.h"
#include "depthOffsetAttrib.h"
#include "depthTestAttrib.h"
#include "depthWriteAttrib.h"
#include "directionalLight.h"
#include "drawMaskAttrib.h"
#include "fadeLodNode.h"
#include "fadeLodNodeData.h"
#include "fog.h"
#include "fogAttrib.h"
#include "geomNode.h"
#include "lensNode.h"
#include "light.h"
#include "lightAttrib.h"
#include "lightLensNode.h"
#include "lightNode.h"
#include "loaderFileType.h"
#include "loaderFileTypeBam.h"
#include "loaderFileTypeRegistry.h"
#include "lodNode.h"
#include "materialAttrib.h"
#include "modelNode.h"
#include "modelRoot.h"
#include "nodePath.h"
#include "nodePathComponent.h"
#include "pandaNode.h"
#include "planeNode.h"
#include "pointLight.h"
#include "polylightNode.h"
#include "polylightEffect.h"
#include "portalNode.h"
#include "portalClipper.h"
#include "renderAttrib.h"
#include "renderEffect.h"
#include "renderEffects.h"
#include "renderModeAttrib.h"
#include "renderState.h"
#include "rescaleNormalAttrib.h"
#include "selectiveChildNode.h"
#include "sequenceNode.h"
#include "shadeModelAttrib.h"
#include "shaderInput.h"
#include "shaderAttrib.h"
#include "shader.h"
#include "showBoundsEffect.h"
#include "spotlight.h"
#include "stateMunger.h"
#include "switchNode.h"
#include "texMatrixAttrib.h"
#include "texProjectorEffect.h"
#include "textureAttrib.h"
#include "texGenAttrib.h"
#include "transformState.h"
#include "transparencyAttrib.h"
#include "nodePathLerps.h"

#include "dconfig.h"

ConfigureDef(config_pgraph);
NotifyCategoryDef(pgraph, "");
NotifyCategoryDef(loader, "");
NotifyCategoryDef(portal, "");

ConfigureFn(config_pgraph) {
  init_libpgraph();
}

ConfigVariableBool fake_view_frustum_cull
("fake-view-frustum-cull", false,
 PRC_DESC("Set this true to cause culling to be performed by rendering the "
          "object in red wireframe, rather than actually culling it.  This "
          "helps make culling errors obvious."));


ConfigVariableBool allow_portal_cull
("allow-portal-cull", false,
 PRC_DESC("Set this true to enable portal clipping.  This will enable the "
          "renderer to cull more objects that are clipped if not in the "
          "current list of portals.  This is still somewhat experimental."));

ConfigVariableBool unambiguous_graph
("unambiguous-graph", false,
 PRC_DESC("Set this true to make ambiguous path warning messages generate an "
          "assertion failure instead of just a warning (which can then be "
          "trapped with assert-abort)."));

ConfigVariableBool allow_unrelated_wrt
("allow-unrelated-wrt", true,
 PRC_DESC("Set this true to allow unrelated NodePaths (that is, nodes which "
          "have no common ancestor) to be adjusted relative to each other.  If "
          "true, these will be treated as if they had a common node above "
          "their top nodes."));

ConfigVariableBool paranoid_compose
("paranoid-compose", false,
 PRC_DESC("Set this true to double-check the componentwise transform compose "
          "(or invert) operation against the equivalent matrix-based "
          "operation.  This has no effect if NDEBUG is defined."));

ConfigVariableBool compose_componentwise
("compose-componentwise", true,
 PRC_DESC("Set this true to perform componentwise compose and invert "
          "operations when possible.  If this is false, the compositions "
          "are always computed by matrix."));

ConfigVariableBool uniquify_matrix
("uniquify-matrix", false,
 PRC_DESC("Set this true to look up arbitarary 4x4 transform matrices in the "
          "cache, to ensure that two differently-computed transforms that "
          "happen to encode the same matrix (an unlikely occurrence) will be "
          "collapsed into a single pointer (a tiny benefit).  We're usually "
          "better off not paying the cost of this comparison, and just "
          "assuming that any two differently-computed transforms are "
          "essentially different."));

ConfigVariableBool paranoid_const
("paranoid-const", false,
 PRC_DESC("Set this true to double-check that nothing is inappropriately "
          "modifying the supposedly const structures like RenderState, "
          "RenderAttrib, TransformState, and RenderEffect.  This has no effect "
          "if NDEBUG is defined."));

ConfigVariableBool auto_break_cycles
("auto-break-cycles", true,
 PRC_DESC("Set this true to automatically detect and break reference-count "
          "cycles in the TransformState and RenderState caches.  When this "
          "is false, you must explicitly call TransformState.clear_cache() "
          "from time to time to prevent gradual memory bloat."));

ConfigVariableBool transform_cache
("transform-cache", true,
 PRC_DESC("Set this true to enable the cache of TransformState objects.  "
          "Using the cache saves time computing transforms and inverse "
          "transforms, but imposes some overhead for maintaining the "
          "cache itself."));

ConfigVariableBool state_cache
("state-cache", true,
 PRC_DESC("Set this true to enable the cache of RenderState objects, "
          "similar to the TransformState cache controlled via "
          "transform-cache."));

ConfigVariableBool retransform_sprites
("retransform-sprites", true,
 PRC_DESC("To render sprite-based particle effects, Panda must convert "
          "the sprite points from object space into clip space, and compute "
          "the corners of the quads in clip space.  When this variable is "
          "false, the resulting quads are then sent to the graphics "
          "hardware in clip space.  When this is true, the quads are "
          "re-transformed back into the original object space, which is "
          "necessary in order for fog to work correctly on the sprites."));

ConfigVariableInt max_collect_vertices
("max-collect-vertices", 65535,
 PRC_DESC("Specifies the maximum number of vertices that are allowed to be "
          "accumulated into any one GeomVertexData structure as a result "
          "of collecting objects together during a flatten operation.  "
          "This prevents the accidental generation of large vertex buffers "
          "from lots of smaller vertex buffers, while not "
          "imposing a limit on the original size of any one "
          "GeomVertexData structure."));

ConfigVariableInt max_collect_indices
("max-collect-indices", 65535,
 PRC_DESC("Specifies the maximum number of vertex indices that are allowed "
          "to be accumulated into any one GeomPrimitive as a result "
          "of collecting objects together during a flatten operation.  "
          "This prevents the accidental generation of large index buffers "
          "from lots of smaller index buffers, while not "
          "imposing a limit on the original size of any one "
          "GeomPrimitive."));

ConfigVariableBool polylight_info
("polylight-info", false,
 PRC_DESC("Set this true to view some info statements regarding the polylight. "
          "It is helpful for debugging."));

ConfigVariableDouble lod_fade_time
("lod-fade-time", 0.5,
 PRC_DESC("The default amount of time (in seconds) over which a FadeLODNode "
          "transitions between its different levels."));


ConfigVariableBool show_vertex_animation
("show-vertex-animation", false,
 PRC_DESC("Set this true to flash any objects whose vertices are animated "
          "by Panda on the CPU (flash red) or by hardware (flash blue).  "
          "This only has effect when NDEBUG is not defined."));

ConfigVariableBool show_transparency
("show-transparency", false,
 PRC_DESC("Set this true to flash any objects that are rendered in "
          "some transparency mode.  The color chosen is based on the  "
          "particular transparency mode in effect.  This only has effect "
          "when NDEBUG is not defined."));

ConfigVariableBool m_dual
("m-dual", true,
 PRC_DESC("Set this false to disable TransparencyAttrib::M_dual altogether "
          "(and use M_alpha in its place)."));

ConfigVariableBool m_dual_opaque
("m-dual-opaque", true,
 PRC_DESC("Set this false to disable just the opaque part of M_dual."));

ConfigVariableBool m_dual_transparent
("m-dual-transparent", true,
 PRC_DESC("Set this false to disable just the transparent part of M_dual."));

ConfigVariableBool m_dual_flash
("m-dual-flash", false,
 PRC_DESC("Set this true to flash any objects that use M_dual, for debugging."));


ConfigVariableBool asynchronous_loads
("asynchronous-loads", false,
 PRC_DESC("Set this true to support actual asynchronous loads via the "
          "request_load()/fetch_load() interface to Loader.  Set it false to "
          "map these to blocking, synchronous loads instead.  Currently, the "
          "rest of Panda isn't quite ready for asynchronous loads, so leave "
          "this false for now."));

ConfigVariableList load_file_type
("load-file-type",
 PRC_DESC("List the model loader modules that Panda will automatically "
          "import when a new, unknown model type is loaded.  This may be "
          "either the name of a module, or a space-separate list of filename "
          "extensions, followed by the name of the module."));

////////////////////////////////////////////////////////////////////
//     Function: init_libpgraph
//  Description: Initializes the library.  This must be called at
//               least once before any of the functions or classes in
//               this library can be used.  Normally it will be
//               called by the static initializers and need not be
//               called explicitly, but special cases exist.
////////////////////////////////////////////////////////////////////
void
init_libpgraph() {
  static bool initialized = false;
  if (initialized) {
    return;
  }
  initialized = true;

  AlphaTestAttrib::init_type();
  AmbientLight::init_type();
  AntialiasAttrib::init_type();
  AuxSceneData::init_type();
  BillboardEffect::init_type();
  Camera::init_type();
  ClipPlaneAttrib::init_type();
  ColorAttrib::init_type();
  ColorBlendAttrib::init_type();
  ColorScaleAttrib::init_type();
  ColorWriteAttrib::init_type();
  CompassEffect::init_type();
  CullFaceAttrib::init_type();
  CullBin::init_type();
  CullBinAttrib::init_type();
  CullTraverser::init_type();
  CullableObject::init_type();
  DecalEffect::init_type();
  DepthOffsetAttrib::init_type();
  DepthTestAttrib::init_type();
  DepthWriteAttrib::init_type();
  DirectionalLight::init_type();
  DrawMaskAttrib::init_type();
  FadeLODNode::init_type();
  FadeLODNodeData::init_type();
  Fog::init_type();
  FogAttrib::init_type();
  GeomNode::init_type();
  LensNode::init_type();
  Light::init_type();
  LightAttrib::init_type();
  LightLensNode::init_type();
  LightNode::init_type();
  LODNode::init_type();
  LoaderFileType::init_type();
  LoaderFileTypeBam::init_type();
  MaterialAttrib::init_type();
  ModelNode::init_type();

  ModelRoot::init_type();
  NodePath::init_type();
  NodePathComponent::init_type();
  PandaNode::init_type();
  PlaneNode::init_type();
  PointLight::init_type();
  PolylightNode::init_type();
  PolylightEffect::init_type();
  PortalNode::init_type();
  PortalClipper::init_type();
  RenderAttrib::init_type();
  RenderEffect::init_type();
  RenderEffects::init_type();
  RenderModeAttrib::init_type();
  RenderState::init_type();
  RescaleNormalAttrib::init_type();
  SelectiveChildNode::init_type();
  SequenceNode::init_type();
  ShadeModelAttrib::init_type();
  ShaderInput::init_type();
  ShaderAttrib::init_type();
  Shader::init_type();
  ShowBoundsEffect::init_type();
  Spotlight::init_type();
  StateMunger::init_type();
  StencilAttrib::init_type();
  SwitchNode::init_type();
  TexMatrixAttrib::init_type();
  TexProjectorEffect::init_type();
  TextureAttrib::init_type();
  TexGenAttrib::init_type();
  TransformState::init_type();
  TransparencyAttrib::init_type();
  PosLerpFunctor::init_type();
  HprLerpFunctor::init_type();
  ScaleLerpFunctor::init_type();
  PosHprLerpFunctor::init_type();
  HprScaleLerpFunctor::init_type();
  PosHprScaleLerpFunctor::init_type();
  ColorLerpFunctor::init_type();
  ColorScaleLerpFunctor::init_type();

  AlphaTestAttrib::register_with_read_factory();
  AmbientLight::register_with_read_factory();
  AntialiasAttrib::register_with_read_factory();
  BillboardEffect::register_with_read_factory();
  Camera::register_with_read_factory();
  ClipPlaneAttrib::register_with_read_factory();
  CompassEffect::register_with_read_factory();
  ColorAttrib::register_with_read_factory();
  ColorBlendAttrib::register_with_read_factory();
  ColorScaleAttrib::register_with_read_factory();
  ColorWriteAttrib::register_with_read_factory();
  CullBinAttrib::register_with_read_factory();
  CullFaceAttrib::register_with_read_factory();
  DecalEffect::register_with_read_factory();
  DepthOffsetAttrib::register_with_read_factory();
  DepthTestAttrib::register_with_read_factory();
  DepthWriteAttrib::register_with_read_factory();
  DirectionalLight::register_with_read_factory();
  DrawMaskAttrib::register_with_read_factory();
  Fog::register_with_read_factory();
  FogAttrib::register_with_read_factory();
  GeomNode::register_with_read_factory();
  LensNode::register_with_read_factory();
  LightAttrib::register_with_read_factory();
  LODNode::register_with_read_factory();
  MaterialAttrib::register_with_read_factory();
  ModelNode::register_with_read_factory();
  ModelRoot::register_with_read_factory();
  PandaNode::register_with_read_factory();
  PlaneNode::register_with_read_factory();
  PointLight::register_with_read_factory();
  PolylightNode::register_with_read_factory();
  PortalNode::register_with_read_factory();
  RenderEffects::register_with_read_factory();
  RenderModeAttrib::register_with_read_factory();
  RenderState::register_with_read_factory();
  RescaleNormalAttrib::register_with_read_factory();
  SequenceNode::register_with_read_factory();
  ShadeModelAttrib::register_with_read_factory();
  ShaderInput::register_with_read_factory();
  ShaderAttrib::register_with_read_factory();
  Shader::register_with_read_factory();
  ShowBoundsEffect::register_with_read_factory();
  Spotlight::register_with_read_factory();
  SwitchNode::register_with_read_factory();
  TexMatrixAttrib::register_with_read_factory();
  TexProjectorEffect::register_with_read_factory();
  TextureAttrib::register_with_read_factory();
  TexGenAttrib::register_with_read_factory();
  TransformState::register_with_read_factory();
  TransparencyAttrib::register_with_read_factory();

  // By initializing the _states map up front, we also guarantee that
  // the _states_lock mutex gets created before we spawn any threads
  // (assuming no one is creating threads at static init time).
  TransformState::init_states();
  RenderState::init_states();
  RenderEffects::init_states();

  LoaderFileTypeRegistry *reg = LoaderFileTypeRegistry::get_global_ptr();
  reg->register_type(new LoaderFileTypeBam);
}
