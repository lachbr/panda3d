set(P3PVIS_HEADERS
  vis.h
  zones.h
)

set(P3PVIS_SOURCES
  flow.cpp
  vis.cpp
  zones.cpp
)

composite_sources(p3pvis P3PVIS_SOURCES)
add_executable(p3pvis ${P3PVIS_HEADERS} ${P3PVIS_SOURCES})
target_link_libraries(p3pvis panda p3bspbase)

set_target_properties(p3pvis PROPERTIES OUTPUT_NAME "pvis")
target_compile_definitions(p3pvis PRIVATE HLVIS)

install(TARGETS p3pvis EXPORT BSP COMPONENT BSP DESTINATION ${CMAKE_INSTALL_BINDIR})
