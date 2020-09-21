if (NOT HAVE_EMBREE)
  return()
endif()

set(P3RAYTRACE_HEADERS
  config_raytrace.h
  rayTrace.I rayTrace.h
  rayTraceGeometry.h
  rayTraceHitResult.h
  rayTraceHitResult4.h
  rayTraceScene.h
  rayTraceTriangleMesh.h
)

set(P3RAYTRACE_SOURCES
  config_raytrace.cxx
  rayTrace.cxx
  rayTraceGeometry.cxx
  rayTraceScene.cxx
  rayTraceTriangleMesh.cxx
)

composite_sources(p3raytrace P3BSPNET_SOURCES)
add_component_library(p3raytrace SYMBOL BUILDING_BSP_RAYTRACE ${P3RAYTRACE_SOURCES} ${P3RAYTRACE_HEADERS})
target_link_libraries(p3raytrace panda p3bspbase PKG::EMBREE)
target_interrogate(p3raytrace ALL)

if(NOT BUILD_METALIBS)
  install(TARGETS p3raytrace
    EXPORT BSP COMPONENT BSP
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/panda3d
    ARCHIVE COMPONENT BSPDevel)
endif()
install(FILES ${P3RAYTRACE_HEADERS} COMPONENT BSPDevel DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/panda3d)
