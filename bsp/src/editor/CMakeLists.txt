set(P3EDITOR_HEADERS
  cLine.h
  config_editor.h
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
  config_editor.cxx
  cPolygon.cxx
  faceMaterial.cxx
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

composite_sources(p3editor P3EDITOR_SOURCES)
add_library(p3editor ${P3EDITOR_SOURCES} ${P3EDITOR_HEADERS})
set_target_properties(p3editor PROPERTIES DEFINE_SYMBOL BUILDING_EDITOR)
target_link_libraries(p3editor panda p3bsp)
target_interrogate(p3editor ALL EXTENSIONS ${P3EDITOR_EXT})

install(TARGETS p3editor
  EXPORT BSP COMPONENT BSP
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/panda3d
  ARCHIVE COMPONENT BSPDevel)
install(FILES ${P3EDITOR_HEADERS} COMPONENT BSPDevel DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/panda3d)
