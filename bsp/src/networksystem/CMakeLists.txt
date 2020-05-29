if(NOT HAVE_ENET)
    return()
endif()

set(P3BSPNET_HEADERS
  config_networksystem.h
  network_host.h
  network_peer.h
  network_event.h
  enet_utils.h
)

set(P3BSPNET_SOURCES
  config_networksystem.cpp
  network_host.cpp
  network_peer.cpp
  network_event.cpp
)

set(P3BSPNET_IGATEEXT
  distributed_smooth_node_base.h
  distributed_smooth_node_base.cpp
)

composite_sources(p3networksystem P3BSPNET_SOURCES)
add_library(p3networksystem ${P3BSPNET_SOURCES} ${P3BSPNET_HEADERS})
set_target_properties(p3networksystem PROPERTIES DEFINE_SYMBOL BUILDING_NETWORKSYSTEM)
target_link_libraries(p3networksystem panda p3direct PKG::ENET)
target_interrogate(p3networksystem ALL EXTENSIONS ${P3BSPNET_IGATEEXT})

install(TARGETS p3networksystem
  EXPORT BSP COMPONENT BSP
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/panda3d
  ARCHIVE COMPONENT BSPDevel)
#install(FILES ${P3BSP_HEADERS} COMPONENT BSPDevel DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/panda3d)
