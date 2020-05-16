if(NOT BUILD_PANDA)
  message(FATAL_ERROR "Cannot build bsp without panda!  Please enable the BUILD_PANDA option.")
endif()

add_subdirectory(src/bspbase)
add_subdirectory(src/bsp)
add_subdirectory(src/networksystem)

add_subdirectory(src/pcsg)
add_subdirectory(src/pbsp)
add_subdirectory(src/pvis)
add_subdirectory(src/prad)

if(HAVE_PYTHON)
	add_python_module(panda3d.bsp p3bsp p3bspinternal LINK p3bsp LINK panda
		IMPORT panda3d.core IMPORT panda3d.bullet COMPONENT BSPPython)

  add_python_module(panda3d.networksystem p3networksystem IMPORT panda3d.core COMPONENT BSPPython)
endif()

#export_targets(BSP COMPONENT BSPDevel)
