set(P3EDITOR_HEADERS
  cLine.h
  config_leveleditor.h
  cPlane.h
  cPointCloud.h
  cPolygon.h
  cRay.h
  editorUtils.h
  faceMaterial.h
  faceOrientation.h
  materialReference.h
  planeClassification.h
  solid.h
  solidFace.h
  solidVertex.h
  archBrush.h
  baseBrush.h
  solidCollection.h
  solidNode.h
  solidGeomNode.h
)

set(P3EDITOR_SOURCES
  config_leveleditor.cxx
  cPolygon.cxx
  faceMaterial.cxx
  faceOrientation.cxx
  solid.cxx
  solidFace.cxx
  solidVertex.cxx
  archBrush.cxx
  solidNode.cxx
  solidGeomNode.cxx
)

set(P3EDITOR_EXT
  solid_ext.h
)

composite_sources(p3leveleditor P3EDITOR_SOURCES)
add_component_library(p3leveleditor SYMBOL BUILDING_EDITOR ${P3EDITOR_SOURCES} ${P3EDITOR_HEADERS})
target_link_libraries(p3leveleditor panda p3bsplib)
target_interrogate(p3leveleditor ALL EXTENSIONS ${P3EDITOR_EXT})

if(NOT BUILD_METALIBS)
  install(TARGETS p3leveleditor
    EXPORT BSP COMPONENT BSP
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/panda3d
    ARCHIVE COMPONENT BSPDevel)
endif()
install(FILES ${P3EDITOR_HEADERS} COMPONENT BSPDevel DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/panda3d)
