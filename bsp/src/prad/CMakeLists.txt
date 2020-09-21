set(P3PRAD_HEADERS
  leaf_ambient_lighting.h
  lightingutils.h
  lightmap.h
  lights.h
  qrad.h
  radial.h
  radstaticprop.h
  trace.h
  vismat.h
)

set(P3PRAD_SOURCES
  leaf_ambient_lighting.cpp
  lightingutils.cpp
  lightmap.cpp
  lights.cpp
  loadtextures.cpp
  mathutil.cpp
  qrad.cpp
  qradutil.cpp
  radial.cpp
  radstaticprop.cpp
  trace.cpp
  vismat.cpp
)

composite_sources(p3prad P3PRAD_SOURCES)
add_executable(p3prad ${P3PRAD_HEADERS} ${P3PRAD_SOURCES})
target_link_libraries(p3prad panda p3bsp)

set_target_properties(p3prad PROPERTIES OUTPUT_NAME "prad")
target_compile_definitions(p3prad PRIVATE HLRAD)

install(TARGETS p3prad EXPORT BSP COMPONENT BSP DESTINATION ${CMAKE_INSTALL_BINDIR})
