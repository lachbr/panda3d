set(P3BSP_HEADERS
  config_bsp.h
  #actor.h
  ambient_boost_effect.h
  ambient_probes.h
  audio_3d_manager.h
  aux_data_attrib.h
  bloom_attrib.h
  bounding_kdop.h
  bsp_render.h
  bsp_trace.h
  bsploader.h
  ciolib.h
  cubemaps.h
  decals.h
  glow_node.h
  interpolated.h
  interpolatedvar.h
  lerp_functions.h
  lighting_origin_effect.h
  lightmap_palettes.h
  planar_reflections.h
  pssmCameraRig.h
  rangecheckedvar.h
  raytrace.h
  shader_csmrender.h
  shader_decalmodulate.h
  shader_features.h
  shader_generator.h
  shader_lightmappedgeneric.h
  shader_skybox.h
  shader_spec.h
  shader_unlitgeneric.h
  shader_unlitnomat.h
  shader_vertexlitgeneric.h
  static_props.h
  texture_filter.h
  TexturePacker.h

  kdtree/KDTree.h
  kdtree/utils.h

  postprocess/bloom.h
  postprocess/blur_passes.h
  postprocess/fxaa.h
  postprocess/hdr.h
  postprocess/postprocess.h
  postprocess/postprocess_defines.h
  postprocess/postprocess_effect.h
  postprocess/postprocess_pass.h
  postprocess/postprocess_scene_pass.h
  postprocess/ssao.h
)

set(P3BSP_IGATE_HEADERS
  config_bsp.h
  bsploader.h
  bsp_render.h
  shader_generator.h
  shader_spec.h
  TexturePacker.h
  shader_vertexlitgeneric.h
  shader_lightmappedgeneric.h
  shader_unlitgeneric.h
  shader_unlitnomat.h
  shader_csmrender.h
  raytrace.h
  shader_skybox.h
  ambient_boost_effect.h
  audio_3d_manager.h
  ciolib.h
  bounding_kdop.h
  shader_decalmodulate.h
  glow_node.h
  lighting_origin_effect.h
  planar_reflections.h
  bloom_attrib.h
  interpolatedvar.h
  interpolated.h

  postprocess/postprocess.h
  postprocess/hdr.h
  postprocess/bloom.h
  postprocess/fxaa.h
  postprocess/ssao.h
)

set(P3BSP_IGATE_EXTENSIONS
  py_bsploader.h
  py_bsploader.cpp
  entity.h
  entity.cpp
  physics_character_controller.h
  physics_character_controller.cpp
)

set(P3BSP_SOURCES
  config_bsp.cpp
  ambient_boost_effect.cpp
  ambient_probes.cpp
  audio_3d_manager.cpp
  aux_data_attrib.cpp
  bloom_attrib.cpp
  bounding_kdop.cpp
  bsp_render.cpp
  bsp_trace.cpp
  bsploader.cpp
  ciolib.cpp
  decals.cpp
  glow_node.cpp
  interpolated.cpp
  interpolatedvar.cpp
  lighting_origin_effect.cpp
  lightmap_palettes.cpp
  planar_reflections.cpp
  pssmCameraRig.cpp
  rangecheckedvar.cpp
  raytrace.cpp
  shader_csmrender.cpp
  shader_decalmodulate.cpp
  shader_features.cpp
  shader_generator.cpp
  shader_lightmappedgeneric.cpp
  shader_skybox.cpp
  shader_spec.cpp
  shader_unlitgeneric.cpp
  shader_unlitnomat.cpp
  shader_vertexlitgeneric.cpp
  static_props.cpp
  texture_filter.cpp
  TexturePacker.cpp

  kdtree/KDTree.cpp

  postprocess/bloom.cpp
  postprocess/fxaa.cpp
  postprocess/hdr.cpp
  postprocess/postprocess.cpp
  postprocess/postprocess_effect.cpp
  postprocess/postprocess_pass.cpp
  postprocess/postprocess_scene_pass.cpp
  postprocess/ssao.cpp
)

composite_sources(p3bsp P3BSP_SOURCES)
add_library(p3bsp ${P3BSP_SOURCES} ${P3BSP_HEADERS})
set_target_properties(p3bsp PROPERTIES DEFINE_SYMBOL BUILDING_LIBPANDABSP)
if(HAVE_PYTHON)
  target_compile_definitions(p3bsp PUBLIC _PYTHON_VERSION)
endif()
target_link_libraries(p3bsp panda pandaegg p3eggbase p3bullet p3bspbase p3interrogatedb PKG::EMBREE PKG::PYTHON)
target_interrogate(p3bsp ALL EXTENSIONS ${P3BSP_IGATE_EXTENSIONS})

install(TARGETS p3bsp
  EXPORT BSP COMPONENT BSP
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/panda3d
  ARCHIVE COMPONENT BSPDevel)
install(FILES ${P3BSP_HEADERS} COMPONENT BSPDevel DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/panda3d)
