set(P3BSPLIB_HEADERS
  config_bsplib.h
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
  bsplevel.h
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
  imagePacker.h
  plane_culled_geom_node.h
  dynamicRender.h

  kdtree/KDTree.h
  kdtree/utils.h
)

set(P3BSPLIB_IGATE_HEADERS
  config_bsplib.h
  bsploader.h
  bsplevel.h
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
  plane_culled_geom_node.h
)

set(P3BSPLIB_IGATE_EXTENSIONS
  py_bsploader.h
  py_bsploader.cpp
  entity.h
  entity.cpp
  physics_character_controller.h
  physics_character_controller.cpp
)

set(P3BSPLIB_SOURCES
  config_bsplib.cpp
  ambient_boost_effect.cpp
  ambient_probes.cpp
  audio_3d_manager.cpp
  aux_data_attrib.cpp
  bloom_attrib.cpp
  bounding_kdop.cpp
  bsp_render.cpp
  bsp_trace.cpp
  bsploader.cpp
  bsplevel.cpp
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
  imagePacker.cxx
  plane_culled_geom_node.cpp
  dynamicRender.cxx

  kdtree/KDTree.cpp
)

composite_sources(p3bsplib P3BSPLIB_SOURCES)
add_component_library(p3bsplib SYMBOL BUILDING_LIBPANDABSP ${P3BSPLIB_SOURCES} ${P3BSPLIB_HEADERS})
target_compile_definitions(p3bsplib PUBLIC CIO)
if(HAVE_PYTHON)
  target_compile_definitions(p3bsplib PUBLIC _PYTHON_VERSION)
endif()
target_link_libraries(p3bsplib panda pandaegg p3eggbase p3bullet p3bspbase p3postprocess p3interrogatedb PKG::EMBREE)
target_interrogate(p3bsplib ALL EXTENSIONS ${P3BSPLIB_IGATE_EXTENSIONS})

if(NOT BUILD_METALIBS)
  install(TARGETS p3bsplib
    EXPORT BSP COMPONENT BSP
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/panda3d
    ARCHIVE COMPONENT BSPDevel)
endif()
install(FILES ${P3BSPLIB_HEADERS} COMPONENT BSPDevel DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/panda3d)
