// DIR_TYPE "metalib" indicates we are building a shared library that
// consists mostly of references to other shared libraries.  Under
// Windows, this directly produces a DLL (as opposed to the regular
// src libraries, which don't produce anything but a pile of OBJ files
// under Windows).

#define DIR_TYPE metalib
#define BUILDING_DLL BUILDING_PANDA
#define USE_NET yes

#define COMPONENT_LIBS \
    pvrpn grutil chan chancfg pstatclient \
    char chat collide cull device \
    dgraph display gobj graph gsgbase \
    gsgmisc light linmath mathutil net \
    parametrics pnm \
    pnmimagetypes pnmimage sgattrib sgmanip sgraph sgraphutil \
    switchnode text tform tiff lerp loader putil \
    audio pgui pandabase 




#define LOCAL_LIBS downloader event ipc express pandabase
#define OTHER_LIBS dtoolconfig dtool

#if $[LINK_IN_GL]
  #define BUILDING_DLL $[BUILDING_DLL] BUILDING_PANDAGL
  #define COMPONENT_LIBS $[COMPONENT_LIBS] \
    glgsg glxdisplay wgldisplay glutdisplay \
    sgidisplay sgiglxdisplay sgiglutdisplay
#endif

#if $[LINK_IN_PHYSICS]
  #define BUILDING_DLL $[BUILDING_DLL] BUILDING_PANDAPHYSICS
  #define COMPONENT_LIBS $[COMPONENT_LIBS] \
    physics particlesystem
#endif

#begin metalib_target
  #define TARGET panda

  #define SOURCES panda.cxx panda.h
  #define INSTALL_HEADERS panda.h
#end metalib_target
