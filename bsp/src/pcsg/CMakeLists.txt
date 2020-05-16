set(P3PCSG_HEADERS
  csg.h
)

set(P3PCSG_SOURCES
  ansitoutf8.cpp
  autowad.cpp
  brush.cpp
  brushunion.cpp
  hullfile.cpp
  map.cpp
  netvis_in_vis.cpp
  properties.cpp
  qcsg.cpp
  textures.cpp
)

composite_sources(p3pcsg P3PCSG_SOURCES)
add_executable(p3pcsg ${P3PCSG_HEADERS} ${P3PCSG_SOURCES})
target_link_libraries(p3pcsg panda p3bspbase)

set_target_properties(p3pcsg PROPERTIES OUTPUT_NAME "pcsg")
target_compile_definitions(p3pcsg PRIVATE HLCSG)

install(TARGETS p3pcsg EXPORT BSP COMPONENT BSP DESTINATION ${CMAKE_INSTALL_BINDIR})
