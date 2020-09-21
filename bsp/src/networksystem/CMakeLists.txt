if (NOT HAVE_GAMENETWORKINGSOCKETS)
  return()
endif()

set(P3BSPNET_HEADERS
  config_networksystem.h
)

set(P3BSPNET_SOURCES
  config_networksystem.cpp
)

set(P3BSPNET_IGATEEXT
  networksystem.h
  networksystem.cpp
)

composite_sources(p3networksystem P3BSPNET_SOURCES)
add_component_library(p3networksystem SYMBOL BUILDING_NETWORKSYSTEM ${P3BSPNET_SOURCES} ${P3BSPNET_HEADERS})
target_compile_definitions(p3networksystem PUBLIC STEAMNETWORKINGSOCKETS_STATIC_LINK)
target_link_libraries(p3networksystem panda GameNetworkingSockets::GameNetworkingSockets_s)
target_interrogate(p3networksystem ALL EXTENSIONS ${P3BSPNET_IGATEEXT})

if(NOT BUILD_METALIBS)
  install(TARGETS p3networksystem
    EXPORT BSP COMPONENT BSP
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/panda3d
    ARCHIVE COMPONENT BSPDevel)
endif()
#install(FILES ${P3BSPNET_HEADERS} COMPONENT BSPDevel DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/panda3d)
