set(P3POSTPROCESS_HEADERS
  config_postprocess.h
  bloom.h
  blurPasses.h
  fxaa.h
  hdr.h
  motionBlur.h
  postProcess.h
  postProcessDefines.h
  postProcessEffect.h
  postProcessPass.h
  postProcessScenePass.h
  ssao.h
)

set(P3POSTPROCESS_SOURCES
  config_postprocess.cxx
  bloom.cxx
  fxaa.cxx
  hdr.cxx
  motionBlur.cxx
  postProcess.cxx
  postProcessEffect.cxx
  postProcessPass.cxx
  postProcessScenePass.cxx
  ssao.cxx
)

composite_sources(p3postprocess P3POSTPROCESS_SOURCES)
add_component_library(p3postprocess SYMBOL BUILDING_BSP_POSTPROCESS ${P3POSTPROCESS_SOURCES} ${P3POSTPROCESS_HEADERS})
target_link_libraries(p3postprocess panda p3bspbase)
target_interrogate(p3postprocess ALL)

if(NOT BUILD_METALIBS)
  install(TARGETS p3postprocess
    EXPORT BSP COMPONENT BSP
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/panda3d
    ARCHIVE COMPONENT BSPDevel)
endif()
install(FILES ${P3POSTPROCESS_HEADERS} COMPONENT BSPDevel DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/panda3d)
