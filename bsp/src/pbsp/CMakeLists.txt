set(P3PBSP_HEADERS
  bsp5.h
)

set(P3PBSP_SOURCES
  merge.cpp
  outside.cpp
  portals.cpp
  qbsp.cpp
  solidbsp.cpp
  surfaces.cpp
  tjunc.cpp
  writebsp.cpp
)

composite_sources(p3pbsp P3PBSP_SOURCES)
add_executable(p3pbsp ${P3PBSP_HEADERS} ${P3PBSP_SOURCES})
target_link_libraries(p3pbsp panda p3bspbase)

set_target_properties(p3pbsp PROPERTIES OUTPUT_NAME "pbsp")
target_compile_definitions(p3pbsp PRIVATE HLBSP)

install(TARGETS p3pbsp EXPORT BSP COMPONENT BSP DESTINATION ${CMAKE_INSTALL_BINDIR})
