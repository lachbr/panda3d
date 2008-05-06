// Filename: shaderGenerator.cxx
// Created by: jyelon (15Dec07)
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


#include "renderState.h"
#include "shaderAttrib.h"
#include "shaderGenerator.h"
#include "ambientLight.h"
#include "directionalLight.h"
#include "pointLight.h"
#include "spotlight.h"

TypeHandle ShaderGenerator::_type_handle;
PT(ShaderGenerator) ShaderGenerator::_default_generator;

////////////////////////////////////////////////////////////////////
//     Function: ShaderGenerator::Constructor
//       Access: Published
//  Description: Create a ShaderGenerator.  This has no state,
//               except possibly to cache certain results.
////////////////////////////////////////////////////////////////////
ShaderGenerator::
ShaderGenerator() {
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderGenerator::Destructor
//       Access: Published, Virtual
//  Description: Destroy a ShaderGenerator.
////////////////////////////////////////////////////////////////////
ShaderGenerator::
~ShaderGenerator() {
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderGenerator::reset_register_allocator
//       Access: Protected
//  Description: Clears the register allocator.  Initially, the pool
//               of available registers is empty.  You have to add
//               some if you want there to be any.
////////////////////////////////////////////////////////////////////
void ShaderGenerator::
reset_register_allocator() {
  _vtregs_used = 0;
  _vcregs_used = 0;
  _ftregs_used = 0;
  _fcregs_used = 0;
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderGenerator::alloc_vreg
//       Access: Protected
//  Description: Allocate a vreg.  
////////////////////////////////////////////////////////////////////
INLINE char *ShaderGenerator::
alloc_vreg() {
  switch (_vtregs_used) {
  case 0: _vtregs_used += 1; return "TEXCOORD0";
  case 1: _vtregs_used += 1; return "TEXCOORD1";
  case 2: _vtregs_used += 1; return "TEXCOORD2";
  case 3: _vtregs_used += 1; return "TEXCOORD3";
  case 4: _vtregs_used += 1; return "TEXCOORD4";
  case 5: _vtregs_used += 1; return "TEXCOORD5";
  case 6: _vtregs_used += 1; return "TEXCOORD6";
  case 7: _vtregs_used += 1; return "TEXCOORD7";
  }
  switch (_vcregs_used) {
  case 0: _vcregs_used += 1; return "COLOR0";
  case 1: _vcregs_used += 1; return "COLOR1";
  }
  return "UNKNOWN";
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderGenerator::alloc_freg
//       Access: Protected
//  Description: Allocate a freg.
////////////////////////////////////////////////////////////////////
INLINE char *ShaderGenerator::
alloc_freg() {
  switch (_ftregs_used) {
  case 0: _ftregs_used += 1; return "TEXCOORD0";
  case 1: _ftregs_used += 1; return "TEXCOORD1";
  case 2: _ftregs_used += 1; return "TEXCOORD2";
  case 3: _ftregs_used += 1; return "TEXCOORD3";
  case 4: _ftregs_used += 1; return "TEXCOORD4";
  case 5: _ftregs_used += 1; return "TEXCOORD5";
  case 6: _ftregs_used += 1; return "TEXCOORD6";
  case 7: _ftregs_used += 1; return "TEXCOORD7";
  }
  switch (_fcregs_used) {
  case 0: _fcregs_used += 1; return "COLOR0";
  case 1: _fcregs_used += 1; return "COLOR1";
  }
  return "UNKNOWN";
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderGenerator::get_default
//       Access: Published, Static
//  Description: Get a pointer to the default shader generator.
////////////////////////////////////////////////////////////////////
ShaderGenerator *ShaderGenerator::
get_default() {
  if (_default_generator == (ShaderGenerator *)NULL) {
    _default_generator = new ShaderGenerator;
  }
  return _default_generator;
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderGenerator::set_default
//       Access: Published, Static
//  Description: Set the default shader generator.
////////////////////////////////////////////////////////////////////
void ShaderGenerator::
set_default(ShaderGenerator *generator) {
  _default_generator = generator;
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderGenerator::analyze_renderstate
//       Access: Protected
//  Description: Analyzes the RenderState prior to shader generation.
//               The results of the analysis are stored in instance
//               variables of the Shader Generator.
////////////////////////////////////////////////////////////////////
void ShaderGenerator::
analyze_renderstate(const RenderState *rs) {
  clear_analysis();

  //  verify_enforce_attrib_lock();

  rs->store_into_slots(&_attribs);
  int outputs = _attribs._aux_bitplane->get_outputs();

  // Decide whether or not we need alpha testing or alpha blending.
  
  if ((_attribs._alpha_test->get_mode() != RenderAttrib::M_none)&&
      (_attribs._alpha_test->get_mode() != RenderAttrib::M_always)) {
    _have_alpha_test = true;
  }
  if (_attribs._color_blend->get_mode() != ColorBlendAttrib::M_none) {
    _have_alpha_blend = true;
  }
  if ((_attribs._transparency->get_mode() == TransparencyAttrib::M_alpha)||
      (_attribs._transparency->get_mode() == TransparencyAttrib::M_dual)) {
    _have_alpha_blend = true;
  }
  
  // Decide what to send to the framebuffer alpha, if anything.
  
  if (outputs & AuxBitplaneAttrib::ABO_glow) {
    if (_have_alpha_blend) {
      _calc_primary_alpha = true;
      _out_primary_glow = false;
      _disable_alpha_write = true;
    } else if (_have_alpha_test) {
      _calc_primary_alpha = true;
      _out_primary_glow = true;
      _subsume_alpha_test = true;
    }
  } else {
    if (_have_alpha_blend || _have_alpha_test) {
      _calc_primary_alpha = true;
    }
  }
  
  // Determine what to put into the aux bitplane.

  _out_aux_normal = (outputs & AuxBitplaneAttrib::ABO_aux_normal) ? true:false;
  _out_aux_glow = (outputs & AuxBitplaneAttrib::ABO_aux_glow) ? true:false;
  _out_aux_any = (_out_aux_normal || _out_aux_glow);
  
  // Count number of textures.

  _num_textures = 0;
  if (_attribs._texture) {
    _num_textures = _attribs._texture->get_num_on_stages();
  }
  
  // Determine whether or not vertex colors or flat colors are present.

  if (_attribs._color->get_color_type() == ColorAttrib::T_vertex) {
    _vertex_colors = true;
  } else if (_attribs._color->get_color_type() == ColorAttrib::T_flat) {
    _flat_colors = true;
  }

  // Break out the lights by type.

  const LightAttrib *la = _attribs._light;
  for (int i=0; i<la->get_num_on_lights(); i++) {
    NodePath light = la->get_on_light(i);
    nassertv(!light.is_empty());
    PandaNode *light_obj = light.node();
    nassertv(light_obj != (PandaNode *)NULL);
    
    if (light_obj->get_type() == AmbientLight::get_class_type()) {
      _alights_np.push_back(light);
      _alights.push_back((AmbientLight*)light_obj);
    }
    else if (light_obj->get_type() == DirectionalLight::get_class_type()) {
      _dlights_np.push_back(light);
      _dlights.push_back((DirectionalLight*)light_obj);
    }
    else if (light_obj->get_type() == PointLight::get_class_type()) {
      _plights_np.push_back(light);
      _plights.push_back((PointLight*)light_obj);
    }
    else if (light_obj->get_type() == Spotlight::get_class_type()) {
      _slights_np.push_back(light);
      _slights.push_back((Spotlight*)light_obj);
    }
  }

  // See if there is a normal map, height map, gloss map, or glow map.
  
  for (int i=0; i<_num_textures; i++) {
    TextureStage *stage = _attribs._texture->get_on_stage(i);
    TextureStage::Mode mode = stage->get_mode();
    if ((mode == TextureStage::M_normal)||(mode == TextureStage::M_normal_height)) {
      _map_index_normal = i;
    }
    if ((mode == TextureStage::M_height)||(mode == TextureStage::M_normal_height)) {
      _map_index_height = i;
    }
    if ((mode == TextureStage::M_glow)||(mode == TextureStage::M_modulate_glow)) {
      _map_index_glow = i;
    }
    if ((mode == TextureStage::M_gloss)||(mode == TextureStage::M_modulate_gloss)) {
      _map_index_gloss = i;
    }
  }
  
  // Determine whether lighting is needed.

  if (_attribs._light->get_num_on_lights() > 0) {
    _lighting = true;
  }
  
  // Find the material.

  if (!_attribs._material->is_off()) {
    _material = _attribs._material->get_material();
  } else {
    _material = Material::get_default();
  }
  
  // Decide which material modes need to be calculated.
  
  if (_lighting && (_alights.size() > 0)) {
    if (_material->has_ambient()) {
      Colorf a = _material->get_ambient();
      if ((a[0]!=0.0)||(a[1]!=0.0)||(a[2]!=0.0)) {
        _have_ambient = true;
      }
    } else {
      _have_ambient = true;
    }
  }

  if (_lighting && (_dlights.size() + _plights.size() + _slights.size())) {
    if (_material->has_diffuse()) {
      Colorf d = _material->get_diffuse();
      if ((d[0]!=0.0)||(d[1]!=0.0)||(d[2]!=0.0)) {
        _have_diffuse = true;
      }
    } else {
      _have_diffuse = true;
    }
  }
  
  if (_lighting && (_material->has_emission())) {
    Colorf e = _material->get_emission();
    if ((e[0]!=0.0)||(e[1]!=0.0)||(e[2]!=0.0)) {
      _have_emission = true;
    }
  }
  
  if (_lighting && (_dlights.size() + _plights.size() + _slights.size())) {
    if (_material->has_specular()) {
      Colorf s = _material->get_specular();
      if ((s[0]!=0.0)||(s[1]!=0.0)||(s[2]!=0.0)) {
        _have_specular = true;
      }
    } else if (_map_index_gloss >= 0) {
      _have_specular = true;
    }
  }
  
  // Decide whether to separate ambient and diffuse calculations.

  if (_have_ambient && _have_diffuse) {
    if (_material->has_ambient()) {
      if (_material->has_diffuse()) {
        _separate_ambient_diffuse = _material->get_ambient() != _material->get_diffuse();
      } else {
        _separate_ambient_diffuse = true;
      }
    } else {
      if (_material->has_diffuse()) {
        _separate_ambient_diffuse = true;
      } else {
        _separate_ambient_diffuse = false;
      }
    }
  }

  if (_lighting && 
      (_attribs._light_ramp->get_mode() != LightRampAttrib::LRT_identity)) {
    _separate_ambient_diffuse = true;
  }
  
  // Does the shader need material properties as input?
  
  _need_material_props = 
    (_have_ambient  && (_material->has_ambient()))||
    (_have_diffuse  && (_material->has_diffuse()))||
    (_have_emission && (_material->has_emission()))||
    (_have_specular && (_material->has_specular()));

  // Check for unimplemented features and issue warnings.

  if (!_attribs._tex_matrix->is_empty()) {
    pgraph_cat.error() << "Shader Generator does not support TexMatrix yet.\n";
  }
  if (!_attribs._tex_gen->is_empty()) {
    pgraph_cat.error() << "Shader Generator does not support TexGen yet.\n";
  }
  if (!_attribs._color_scale->is_identity()) {
    pgraph_cat.error() << "Shader Generator does not support ColorScale yet.\n";
  }
  if (!_attribs._fog->is_off()) {
    pgraph_cat.error() << "Shader Generator does not support Fog yet.\n";
  }
}



////////////////////////////////////////////////////////////////////
//     Function: ShaderGenerator::clear_analysis
//       Access: Protected
//  Description: Called after analyze_renderstate to discard all
//               the results of the analysis.  This is generally done
//               after shader generation is complete.
////////////////////////////////////////////////////////////////////
void ShaderGenerator::
clear_analysis() {
  _vertex_colors = false;
  _flat_colors = false;
  _lighting = false;
  _have_ambient = false;
  _have_diffuse = false;
  _have_emission = false;
  _have_specular = false;
  _separate_ambient_diffuse = false;
  _map_index_normal = -1;
  _map_index_height = -1;
  _map_index_glow = -1;
  _map_index_gloss = -1;
  _calc_primary_alpha = false;
  _have_alpha_test = false;
  _have_alpha_blend = false;
  _subsume_alpha_test = false;
  _disable_alpha_write = false;
  _out_primary_glow  = false;
  _out_aux_normal = false;
  _out_aux_glow   = false;
  _out_aux_any    = false;
  _attribs.clear_to_defaults();
  _material = (Material*)NULL;
  _need_material_props = false;
  _need_clipspace_pos = false;
  _alights.clear();
  _dlights.clear();
  _plights.clear();
  _slights.clear();
  _alights_np.clear();
  _dlights_np.clear();
  _plights_np.clear();
  _slights_np.clear();
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderGenerator::create_shader_attrib
//       Access: Protected
//  Description: Creates a ShaderAttrib given a generated shader's
//               body.  Also inserts the lights into the shader
//               attrib.
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderGenerator::
create_shader_attrib(const string &txt) {
  PT(Shader) shader = Shader::make(txt);
  CPT(RenderAttrib) shattr = ShaderAttrib::make();
  shattr=DCAST(ShaderAttrib, shattr)->set_shader(shader);
  if (_lighting) {
    for (int i=0; i<(int)_alights.size(); i++) {
      shattr=DCAST(ShaderAttrib, shattr)->set_shader_input(InternalName::make("alight", i), _alights_np[i]);
    }
    for (int i=0; i<(int)_dlights.size(); i++) {
      shattr=DCAST(ShaderAttrib, shattr)->set_shader_input(InternalName::make("dlight", i), _dlights_np[i]);
    }
    for (int i=0; i<(int)_plights.size(); i++) {
      shattr=DCAST(ShaderAttrib, shattr)->set_shader_input(InternalName::make("plight", i), _plights_np[i]);
    }
    for (int i=0; i<(int)_slights.size(); i++) {
      shattr=DCAST(ShaderAttrib, shattr)->set_shader_input(InternalName::make("slight", i), _slights_np[i]);
    }
  }
  return shattr;
}

////////////////////////////////////////////////////////////////////
//     Function: ShaderGenerator::synthesize_shader
//       Access: Published, Virtual
//  Description: This is the routine that implements the next-gen
//               fixed function pipeline by synthesizing a shader.
//
//               Currently supports:
//               - flat colors
//               - vertex colors
//               - lighting
//               - normal maps
//               - gloss maps
//               - glow maps
//               - materials, but not updates to materials
//               - 2D textures
//               - texture stage modes: modulate, decal, add
//               - light ramps (for cartoon shading)
//
//               Not yet supported:
//               - 3D textures, cube textures
//               - texture stage modes: replace, blend
//               - color scale attrib
//               - texgen
//               - texmatrix
//               - fog
//               - other TextureStage::Modes
//
//               Potential optimizations
//               - omit attenuation calculations if attenuation off
//
////////////////////////////////////////////////////////////////////
CPT(RenderAttrib) ShaderGenerator::
synthesize_shader(const RenderState *rs) {
  analyze_renderstate(rs);
  reset_register_allocator();

  // These variables will hold the results of register allocation.

  char *pos_freg = 0;
  char *normal_vreg = 0;
  char *normal_freg = 0;
  char *tangent_vreg = 0;
  char *tangent_freg = 0;
  char *binormal_vreg = 0;
  char *binormal_freg = 0;
  char *csnormal_freg = 0;
  char *clip_freg = 0;
  pvector<char *> texcoord_vreg;
  pvector<char *> texcoord_freg;
  pvector<char *> tslightvec_freg;
  
  if (_vertex_colors) {
    _vcregs_used = 1;
    _fcregs_used = 1;
  }

  // Generate the shader's text.

  ostringstream text;
  
  text << "//Cg\n";

  text << "void vshader(\n";
  for (int i=0; i<_num_textures; i++) {
    texcoord_vreg.push_back(alloc_vreg());
    texcoord_freg.push_back(alloc_freg());
    text << "\t in float4 vtx_texcoord" << i << " : " << texcoord_vreg[i] << ",\n";
    text << "\t out float4 l_texcoord" << i << " : " << texcoord_freg[i] << ",\n";
  }
  if (_vertex_colors) {
    text << "\t in float4 vtx_color : COLOR,\n";
    text << "\t out float4 l_color : COLOR,\n";
  }
  if (_lighting) {
    pos_freg = alloc_freg();
    normal_vreg = alloc_vreg();
    normal_freg = alloc_freg();
    text << "\t in float4 vtx_normal : " << normal_vreg << ",\n";
    text << "\t out float4 l_normal : " << normal_freg << ",\n";
    text << "\t out float4 l_pos : " << pos_freg << ",\n";
    if (_map_index_normal >= 0) {
      tangent_vreg = alloc_vreg();
      tangent_freg = alloc_freg();
      binormal_vreg = alloc_vreg();
      binormal_freg = alloc_freg();
      text << "\t in float4 vtx_tangent" << _map_index_normal << " : " << tangent_vreg << ",\n";
      text << "\t in float4 vtx_binormal" << _map_index_normal << " : " << binormal_vreg << ",\n";
      text << "\t out float4 l_tangent : " << tangent_freg << ",\n";
      text << "\t out float4 l_binormal : " << binormal_freg << ",\n";
    }
  }
  if (_need_clipspace_pos) {
    clip_freg = alloc_freg();
    text << "\t out float4 l_clip : " << clip_freg << ",\n";
  }
  if (_out_aux_normal) {
    if (normal_vreg == 0) {
      normal_vreg = alloc_vreg();
      text << "\t in float4 vtx_normal : " << normal_vreg << ",\n";
    }
    csnormal_freg = alloc_freg();
    text << "\t uniform float4x4 itp_modelview,\n";
    text << "\t out float4 l_csnormal : " << csnormal_freg << ",\n";
  }
  
  text << "\t float4 vtx_position : POSITION,\n";
  text << "\t out float4 l_position : POSITION,\n";
  text << "\t uniform float4x4 mat_modelproj\n";
  text << ") {\n";
  
  text << "\t l_position = mul(mat_modelproj, vtx_position);\n";
  if (_need_clipspace_pos) {
    text << "\t l_clip = l_position;\n";
  }
  for (int i=0; i<_num_textures; i++) {
    text << "\t l_texcoord" << i << " = vtx_texcoord" << i << ";\n";
  }
  if (_vertex_colors) {
    text << "\t l_color = vtx_color;\n";
  }
  if (_lighting) {
    text << "\t l_pos = vtx_position;\n";
    text << "\t l_normal = vtx_normal;\n";
    if (_map_index_normal >= 0) {
      text << "\t l_tangent = vtx_tangent" << _map_index_normal << ";\n";
      text << "\t l_binormal = -vtx_binormal" << _map_index_normal << ";\n";
    }
  }
  if (_out_aux_normal) {
    text << "\t l_csnormal.xyz = mul(itp_modelview, vtx_normal);\n";
    text << "\t l_csnormal.w = 0;\n";
  }
  text << "}\n\n";
  
  text << "void fshader(\n";
  
  for (int i=0; i<_num_textures; i++) {
    text << "\t in float4 l_texcoord" << i << " : " << texcoord_freg[i] << ",\n";
    text << "\t uniform sampler2D tex_" << i << ",\n";
  }
  if (_lighting) {
    text << "\t in float3 l_normal : " << normal_freg << ",\n";
    if (_map_index_normal >= 0) {
      text << "\t in float3 l_tangent : " << tangent_freg << ",\n";
      text << "\t in float3 l_binormal : " << binormal_freg << ",\n";
    }
  }
  if (_lighting) {
    text << "\t in float4 l_pos : " << pos_freg << ",\n";
    for (int i=0; i<(int)_alights.size(); i++) {
      text << "\t uniform float4 alight_alight" << i << ",\n";
    }
    for (int i=0; i<(int)_dlights.size(); i++) {
      text << "\t uniform float4x4 dlight_dlight" << i << "_rel_model,\n";
    }
    for (int i=0; i<(int)_plights.size(); i++) {
      text << "\t uniform float4x4 plight_plight" << i << "_rel_model,\n";
    }
    for (int i=0; i<(int)_slights.size(); i++) {
      text << "\t uniform float4x4 slight_slight" << i << "_rel_model,\n";
      text << "\t uniform float4   satten_slight" << i << ",\n";
    }
    if (_need_material_props) {
      text << "\t uniform float4x4 attr_material,\n";
    }
    if (_have_specular) {
      if (_material->get_local()) {
        text << "\t uniform float4 mspos_view,\n";
      } else {
        text << "\t uniform float4 row1_view_to_model,\n";
      }
    }
  }
  if (_out_aux_normal) {
    text << "\t in float4 l_csnormal : " << csnormal_freg << ",\n";
  }
  if (_need_clipspace_pos) {
    text << "\t in float4 l_clip : " << clip_freg << ",\n";
  }
  if (_out_aux_any) {
    text << "\t out float4 o_aux : COLOR1,\n";
  }
  text << "\t out float4 o_color : COLOR0,\n";
  if (_vertex_colors) {
    text << "\t in float4 l_color : COLOR\n";
  } else {
    text << "\t uniform float4 attr_color\n";
  }
  text << ") {\n";
  text << "\t float4 result;\n";
  if (_out_aux_any) {
    text << "\t o_aux = float4(0,0,0,0);\n";
  }
  text << "\t // Fetch all textures.\n";
  for (int i=0; i<_num_textures; i++) {
    text << "\t float4 tex" << i << " = tex2D(tex_" << i << ", float2(l_texcoord" << i << "));\n";
  }
  if (_lighting) {
    if (_map_index_normal >= 0) {
      text << "\t // Translate tangent-space normal in map to model-space.\n";
      text << "\t float3 tsnormal = ((float3)tex" << _map_index_normal << " * 2) - 1;\n";
      text << "\t l_normal = l_normal * tsnormal.z;\n";
      text << "\t l_normal+= l_tangent * tsnormal.x;\n";
      text << "\t l_normal+= l_binormal * tsnormal.y;\n";
      text << "\t l_normal = normalize(l_normal);\n";
    } else {
      text << "\t // Correct the surface normal for interpolation effects\n";
      text << "\t l_normal = normalize(l_normal);\n";
    }
  }
  if (_out_aux_normal) {
    text << "\t // Output the camera-space surface normal\n";
    text << "\t o_aux.rg = ((normalize(l_csnormal)*0.5) + float4(0.5,0.5,0.5,0)).rg;\n";
  }
  if (_lighting) {
    text << "\t // Begin model-space light calculations\n";
    text << "\t float ldist,lattenv,langle;\n";
    text << "\t float4 lcolor,lspec,lvec,lpoint,latten,ldir,leye,lhalf;\n";
    if (_separate_ambient_diffuse) {
      if (_have_ambient) {
        text << "\t float4 tot_ambient = float4(0,0,0,0);\n";
      }
      if (_have_diffuse) {
        text << "\t float4 tot_diffuse = float4(0,0,0,0);\n";
      }
    } else {
      if (_have_ambient || _have_diffuse) {
        text << "\t float4 tot_diffuse = float4(0,0,0,0);\n";
      }
    }
    if (_have_specular) {
      text << "\t float4 tot_specular = float4(0,0,0,0);\n";
      if (_material->has_specular()) {
        text << "\t float shininess = attr_material[3].w;\n";
      } else {
        text << "\t float shininess = 50; // no shininess specified, using default\n";
      }
    }
    for (int i=0; i<(int)_alights.size(); i++) {
      text << "\t // Ambient Light " << i << "\n";
      text << "\t lcolor = alight_alight" << i << ";\n";
      if (_separate_ambient_diffuse) {
        text << "\t tot_ambient += lcolor;\n";
      } else {
        text << "\t tot_diffuse += lcolor;\n";
      }
    }
    for (int i=0; i<(int)_dlights.size(); i++) {
      text << "\t // Directional Light " << i << "\n";
      text << "\t lcolor = dlight_dlight" << i << "_rel_model[0];\n";
      text << "\t lspec  = dlight_dlight" << i << "_rel_model[1];\n";
      text << "\t lvec   = dlight_dlight" << i << "_rel_model[2];\n";
      text << "\t lcolor *= saturate(dot(l_normal, lvec.xyz));\n";
      text << "\t tot_diffuse += lcolor;\n";
      if (_have_specular) {
        if (_material->get_local()) {
          text << "\t lhalf  = normalize(normalize(mspos_view - l_pos) + lvec);\n";
        } else {
          text << "\t lhalf = dlight_dlight" << i << "_rel_model[3];\n";
        }
        text << "\t lspec *= pow(saturate(dot(l_normal, lhalf.xyz)), shininess);\n";
        text << "\t tot_specular += lspec;\n";
      }
    }
    for (int i=0; i<(int)_plights.size(); i++) {
      text << "\t // Point Light " << i << "\n";
      text << "\t lcolor = plight_plight" << i << "_rel_model[0];\n";
      text << "\t lspec  = plight_plight" << i << "_rel_model[1];\n";
      text << "\t lpoint = plight_plight" << i << "_rel_model[2];\n";
      text << "\t latten = plight_plight" << i << "_rel_model[3];\n";
      text << "\t lvec   = lpoint - l_pos;\n";
      text << "\t ldist = length(lvec);\n";
      text << "\t lvec /= ldist;\n";
      text << "\t lattenv = 1/(latten.x + latten.y*ldist + latten.z*ldist*ldist);\n";
      text << "\t lcolor *= lattenv * saturate(dot(l_normal, lvec.xyz));\n";
      text << "\t tot_diffuse += lcolor;\n";
      if (_have_specular) {
        if (_material->get_local()) {
          text << "\t lhalf  = normalize(normalize(mspos_view - l_pos) + lvec);\n";
        } else {
          text << "\t lhalf = normalize(lvec - row1_view_to_model);\n";
        }
        text << "\t lspec *= lattenv;\n";
        text << "\t lspec *= pow(saturate(dot(l_normal, lhalf.xyz)), shininess);\n";
        text << "\t tot_specular += lspec;\n";
      }
    }
    for (int i=0; i<(int)_slights.size(); i++) {
      text << "\t // Spot Light " << i << "\n";
      text << "\t lcolor = slight_slight" << i << "_rel_model[0];\n";
      text << "\t lspec  = slight_slight" << i << "_rel_model[1];\n";
      text << "\t lpoint = slight_slight" << i << "_rel_model[2];\n";
      text << "\t ldir   = slight_slight" << i << "_rel_model[3];\n";
      text << "\t latten = satten_slight" << i << ";\n";
      text << "\t lvec   = lpoint - l_pos;\n";
      text << "\t ldist  = length(lvec);\n";
      text << "\t lvec /= ldist;\n";
      text << "\t langle = saturate(dot(ldir.xyz, lvec.xyz));\n";
      text << "\t lattenv = 1/(latten.x + latten.y*ldist + latten.z*ldist*ldist);\n";
      text << "\t lattenv *= pow(langle, latten.w);\n";
      text << "\t if (langle < ldir.w) lattenv = 0;\n";
      text << "\t lcolor *= lattenv * saturate(dot(l_normal, lvec.xyz));\n";
      text << "\t tot_diffuse += lcolor;\n";
      if (_have_specular) {
        if (_material->get_local()) {
          text << "\t lhalf  = normalize(normalize(mspos_view - l_pos) + lvec);\n";
        } else {
          text << "\t lhalf = normalize(lvec - row1_view_to_model);\n";
        }
        text << "\t lspec *= lattenv;\n";
        text << "\t lspec *= pow(saturate(dot(l_normal, lhalf.xyz)), shininess);\n";
        text << "\t tot_specular += lspec;\n";
      }
    }
    switch (_attribs._light_ramp->get_mode()) {
    case LightRampAttrib::LRT_single_threshold:
      {
        float t = _attribs._light_ramp->get_threshold(0);
        float l0 = _attribs._light_ramp->get_level(0);
        text << "\t // Single-threshold light ramp\n";
        text << "\t float lr_in = dot(tot_diffuse.rgb, float3(0.33,0.34,0.33));\n";
        text << "\t float lr_scale = (lr_in < " << t << ") ? 0.0 : (" << l0 << "/lr_in);\n";
        text << "\t tot_diffuse = tot_diffuse * lr_scale;\n";
        break;
      }
    case LightRampAttrib::LRT_double_threshold:
      {
        float t0 = _attribs._light_ramp->get_threshold(0);
        float t1 = _attribs._light_ramp->get_threshold(1);
        float l0 = _attribs._light_ramp->get_level(0);
        float l1 = _attribs._light_ramp->get_level(1);
        float l2 = _attribs._light_ramp->get_level(2);
        text << "\t // Double-threshold light ramp\n";
        text << "\t float lr_in = dot(tot_diffuse.rgb, float3(0.33,0.34,0.33));\n";
        text << "\t float lr_out = " << l0 << "\n";
        text << "\t if (lr_in > " << t0 << ") lr_out=" << l1 << ";\n";
        text << "\t if (lr_in > " << t1 << ") lr_out=" << l2 << ";\n";
        text << "\t tot_diffuse = tot_diffuse * (lr_out / lr_in);\n";
        break;
      }
    }
    text << "\t // Begin model-space light summation\n";
    if (_have_emission) {
      if (_map_index_glow >= 0) {
        text << "\t result = attr_material[2] * saturate(2 * (tex" << _map_index_glow << ".a - 0.5));\n";
      } else {
        text << "\t result = attr_material[2];\n";
      }
    } else {
      if (_map_index_glow >= 0) {
        text << "\t result = saturate(2 * (tex" << _map_index_glow << ".a - 0.5));\n";
      } else {
        text << "\t result = float4(0,0,0,0);\n";
      }
    }
    if ((_have_ambient)&&(_separate_ambient_diffuse)) {
      if (_material->has_ambient()) {
        text << "\t result += tot_ambient * attr_material[0];\n";
      } else if (_vertex_colors) {
        text << "\t result += tot_ambient * l_color;\n";
      } else if (_flat_colors) {
        text << "\t result += tot_ambient * attr_color;\n";
      } else {
        text << "\t result += tot_ambient;\n";
      }
    }
    if (_have_diffuse) {
      if (_material->has_diffuse()) {
        text << "\t result += tot_diffuse * attr_material[1];\n";
      } else if (_vertex_colors) {
        text << "\t result += tot_diffuse * l_color;\n";
      } else if (_flat_colors) {
        text << "\t result += tot_diffuse * attr_color;\n";
      } else {
        text << "\t result += tot_diffuse;\n";
      }
    }
    if (_attribs._light_ramp->get_mode() == LightRampAttrib::LRT_default) {
      text << "\t result = saturate(result);\n";
    }
    text << "\t // End model-space light calculations\n";

    // Combine in alpha, which bypasses lighting calculations.
    // Use of lerp here is a workaround for a radeon driver bug.
    if (_calc_primary_alpha) {
      if (_vertex_colors) {
        text << "\t result.a = l_color.a;\n";
      } else if (_flat_colors) {
        text << "\t result.a = attr_color.a;\n";
      } else {
        text << "\t result.a = 1;\n";
      }
    }
  } else {
    if (_vertex_colors) {
      text << "\t result = l_color;\n";
    } else if (_flat_colors) {
      text << "\t result = attr_color;\n";
    } else {
      text << "\t result = float4(1,1,1,1);\n";
    }
  }

  for (int i=0; i<_num_textures; i++) {
    TextureStage *stage = _attribs._texture->get_on_stage(i);
    switch (stage->get_mode()) {
    case TextureStage::M_modulate:
    case TextureStage::M_modulate_glow:
    case TextureStage::M_modulate_gloss:
      text << "\t result *= tex" << i << ";\n";
      break;
    case TextureStage::M_decal:
      text << "\t result.rgb = lerp(result, tex" << i << ", tex" << i << ".a).rgb;\n";
      break;
    case TextureStage::M_blend:
      pgraph_cat.error() << "TextureStage::Mode BLEND not yet supported in per-pixel mode.\n";
      break;
    case TextureStage::M_replace:
      pgraph_cat.error() << "TextureStage::Mode REPLACE not yet supported in per-pixel mode.\n";
      break;
    case TextureStage::M_add:
      text << "\t result.rbg = result.rgb + tex" << i << ".rgb;\n";
      if (_calc_primary_alpha) {
        text << "\t result.a   = result.a * tex" << i << ".a;\n";
      }
      break;
    case TextureStage::M_combine:
      pgraph_cat.error() << "TextureStage::Mode COMBINE not yet supported in per-pixel mode.\n";
      break;
    case TextureStage::M_blend_color_scale:
      pgraph_cat.error() << "TextureStage::Mode BLEND_COLOR_SCALE not yet supported in per-pixel mode.\n";
      break;
    default:
      break;
    }
  }

  if (_subsume_alpha_test) {
    text << "\t // Shader includes alpha test:\n";
    double ref = _attribs._alpha_test->get_reference_alpha();
    switch (_attribs._alpha_test->get_mode()) {
    case RenderAttrib::M_never:          text<<"\t discard;\n";
    case RenderAttrib::M_less:           text<<"\t if (result.a >= "<<ref<<") discard;\n";
    case RenderAttrib::M_equal:          text<<"\t if (result.a != "<<ref<<") discard;\n";
    case RenderAttrib::M_less_equal:     text<<"\t if (result.a >  "<<ref<<") discard;\n";
    case RenderAttrib::M_greater:        text<<"\t if (result.a <= "<<ref<<") discard;\n";
    case RenderAttrib::M_not_equal:      text<<"\t if (result.a == "<<ref<<") discard;\n";
    case RenderAttrib::M_greater_equal:  text<<"\t if (result.a <  "<<ref<<") discard;\n";
    }
  }
  
  if (_out_primary_glow) {
    if (_map_index_glow >= 0) {
      text << "\t result.a = tex" << _map_index_glow << ".a;\n";
    } else {
      text << "\t result.a = 0.5;\n";
    }
  }
  if (_out_aux_glow) {
    if (_map_index_glow >= 0) {
      text << "\t o_aux.a = tex" << _map_index_glow << ".a;\n";
    } else {
      text << "\t o_aux.a = 0.5;\n";
    }
  }
  
  if (_lighting) {
    if (_have_specular) {
      if (_material->has_specular()) {
        text << "\t tot_specular *= attr_material[3];\n";
      }
      if (_map_index_gloss >= 0) {
        text << "\t tot_specular *= tex" << _map_index_gloss << ".a;\n";
      }
      text << "\t result.rgb = result.rgb + tot_specular.rgb;\n";
    }
  }
  
  switch (_attribs._light_ramp->get_mode()) {
  case LightRampAttrib::LRT_hdr0:
    text << "\t result.rgb = (result*result*result + result*result + result) / (result*result*result + result*result + result + 1);\n";
    break;
  case LightRampAttrib::LRT_hdr1:
    text << "\t result.rgb = (result*result + result) / (result*result + result + 1);\n";
    break;
  case LightRampAttrib::LRT_hdr2:
    text << "\t result.rgb = result / (result + 1);\n";
    break;
  default: break;
  }
  
  // The multiply is a workaround for a radeon driver bug.
  // It's annoying as heck, since it produces an extra instruction.
  text << "\t o_color = result * 1.000001;\n"; 
  if (_subsume_alpha_test) {
    text << "\t // Shader subsumes normal alpha test.\n";
  }
  if (_disable_alpha_write) {
    text << "\t // Shader disables alpha write.\n";
  }
  text << "}\n";
  
  // Insert the shader into the shader attrib.
  CPT(RenderAttrib) shattr = create_shader_attrib(text.str());
  if (_subsume_alpha_test) {
    shattr=DCAST(ShaderAttrib, shattr)->set_flag(ShaderAttrib::F_subsume_alpha_test, true);
  }
  if (_disable_alpha_write) {
    shattr=DCAST(ShaderAttrib, shattr)->set_flag(ShaderAttrib::F_disable_alpha_write, true);
  }
  clear_analysis();
  reset_register_allocator();
  return shattr;
}

