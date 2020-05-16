set(P3RECAST_HEADERS
  config_recast.h
  recastnavigation_includes.h
  rnTools.h rnTools.I
)

set(P3RECAST_SOURCES
  config_recast.cxx
  rnTools.cxx
)

set(P3RECAST_IGATEEXT
  rnCrowdAgent.h rnCrowdAgent.I
  rnNavMesh.h rnNavMesh.I
  rnNavMeshManager.h rnNavMeshManager.I
  rnCrowdAgent.cxx
  rnNavMesh.cxx
  rnNavMeshManager.cxx
)

set(P3RECAST_LIB_HEADERS
  library/DebugDraw.h
  library/DetourAlloc.h
  library/DetourAssert.h
  library/DetourCommon.h
  library/DetourCrowd.h
  library/DetourDebugDraw.h
  library/DetourLocalBoundary.h
  library/DetourMath.h
  library/DetourNavMesh.h
  library/DetourNavMeshBuilder.h
  library/DetourNavMeshQuery.h
  library/DetourNode.h
  library/DetourObstacleAvoidance.h
  library/DetourPathCorridor.h
  library/DetourPathQueue.h
  library/DetourProximityGrid.h
  library/DetourStatus.h
  library/DetourTileCache.h
  library/DetourTileCacheBuilder.h
  
  library/Recast.h
  library/RecastAlloc.h
  library/RecastAssert.h
  library/RecastDebugDraw.h
  library/RecastDump.h
  
  support/ChunkyTriMesh.h
  support/common.h
  support/ConvexVolumeTool.h
  support/CrowdTool.h
  support/DebugInterfaces.h
  support/fastlz.h
  support/InputGeom.h
  support/MeshLoaderObj.h
  support/NavMeshTesterTool.h
  support/NavMeshType.h
  support/NavMeshType_Obstacle.h
  support/NavMeshType_Solo.h
  support/NavMeshType_Tile.h
  support/OffMeshConnectionTool.h
  support/PerfTimer.h
  support/pstdint.h
)

set(P3RECAST_LIB_SOURCES
  library/DebugDraw.cpp
  library/DetourAlloc.cpp
  library/DetourAssert.cpp
  library/DetourCommon.cpp
  library/DetourCrowd.cpp
  library/DetourDebugDraw.cpp
  library/DetourLocalBoundary.cpp
  library/DetourNavMesh.cpp
  library/DetourNavMeshBuilder.cpp
  library/DetourNavMeshQuery.cpp
  library/DetourNode.cpp
  library/DetourObstacleAvoidance.cpp
  library/DetourPathCorridor.cpp
  library/DetourPathQueue.cpp
  library/DetourProximityGrid.cpp
  library/DetourTileCache.cpp
  library/DetourTileCacheBuilder.cpp
  
  library/Recast.cpp
  library/RecastAlloc.cpp
  library/RecastArea.cpp
  library/RecastAssert.cpp
  library/RecastContour.cpp
  library/RecastDebugDraw.cpp
  library/RecastDump.cpp
  library/RecastFilter.cpp
  library/RecastLayers.cpp
  library/RecastMesh.cpp
  library/RecastMeshDetail.cpp
  library/RecastRasterization.cpp
  library/RecastRegion.cpp
  
  support/ChunkyTriMesh.cpp
  support/ConvexVolumeTool.cpp
  support/CrowdTool.cpp
  support/DebugInterfaces.cpp
  support/fastlz.c
  support/InputGeom.cpp
  support/MeshLoaderObj.cpp
  support/NavMeshTesterTool.cpp
  support/NavMeshType.cpp
  support/NavMeshType_Obstacle.cpp
  support/NavMeshType_Solo.cpp
  support/NavMeshType_Tile.cpp
  support/OffMeshConnectionTool.cpp
  support/PerfTimer.cpp
)

set(P3RECAST_COMPOSITE ${P3RECAST_SOURCES} ${P3RECAST_LIB_SOURCES})

composite_sources(p3recastnavigation P3RECAST_COMPOSITE)
add_library(p3recastnavigation ${P3RECAST_SOURCES} ${P3RECAST_LIB_SOURCES} ${P3RECAST_HEADERS} ${P3RECAST_LIB_HEADERS})
target_compile_definitions(p3recastnavigation PUBLIC PYTHON_BUILD)
target_compile_definitions(p3recastnavigation PRIVATE BUILDING_P3RECAST)
target_include_directories(p3recastnavigation PUBLIC ./library ./support)
target_link_libraries(p3recastnavigation panda PKG::PYTHON)
target_interrogate(p3recastnavigation ${P3RECAST_HEADERS} EXTENSIONS ${P3RECAST_IGATEEXT})

install(TARGETS p3recastnavigation
  EXPORT BSP COMPONENT BSP
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/panda3d
  ARCHIVE COMPONENT BSPDevel)
