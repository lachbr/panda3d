
// This package is temporarily disabled.
#define BUILD_DIRECTORY
//#define BUILD_DIRECTORY $[HAVE_DX]
#define USE_PACKAGES dx

#begin ss_lib_target
  #define TARGET xfile
  #define LOCAL_LIBS eggbase progbase pandatoolbase
  #define OTHER_LIBS \
    egg:c pandaegg:m \
    mathutil:c linmath:c putil:c panda:m \
    express:c pandaexpress:m \
    dtoolconfig dtool pystub \

  #define WIN_SYS_LIBS \
    d3dxof.lib dxguid.lib
    
  #define COMBINED_SOURCES $[TARGET]_composite1.cxx     
    
  #define SOURCES \
     config_xfile.h xFileFace.h xFileMaker.h xFileMaterial.h \
     xFileMesh.h xFileNormal.h xFileTemplates.h \
     xFileToEggConverter.h xFileVertex.h 

  #define INCLUDED_SOURCES \
     config_xfile.cxx xFileFace.cxx xFileMaker.cxx xFileMaterial.cxx \
     xFileMesh.cxx xFileNormal.cxx xFileTemplates.cxx \
     xFileToEggConverter.cxx xFileVertex.cxx 

#end ss_lib_target
