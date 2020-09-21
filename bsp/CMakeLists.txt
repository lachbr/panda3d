if(NOT BUILD_PANDA)
  message(FATAL_ERROR "Cannot build bsp without panda!  Please enable the BUILD_PANDA option.")
endif()

add_subdirectory(src/bspbase)
add_subdirectory(src/bsplib)
add_subdirectory(src/leveleditor)
add_subdirectory(src/networksystem)
add_subdirectory(src/pbsp)
add_subdirectory(src/pcsg)
add_subdirectory(src/postprocess)
add_subdirectory(src/prad)
add_subdirectory(src/pvis)
add_subdirectory(src/raytrace)
add_subdirectory(src/recastnavigation)

# Add the bsp metalib
add_subdirectory(metalibs/bsp)

if(HAVE_PYTHON)
  add_python_module(panda3d.bsp p3bsplib p3bspinternal p3postprocess p3leveleditor p3networksystem
                    p3raytrace
                    IMPORT panda3d.core IMPORT panda3d.bullet COMPONENT BSPPython)

  add_python_module(panda3d.recastnavigation p3recastnavigation IMPORT panda3d.core COMPONENT BSPPython)

  # Install our Python code
  install_python_package(bsp SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src" LIB COMPONENT BSP)

endif()

#export_targets(BSP NAMESPACE "Panda3D::BSP" COMPONENT BSPDevel)
