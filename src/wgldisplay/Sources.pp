#define BUILD_DIRECTORY $[HAVE_WGL]

#define USE_PACKAGES gl

#define OTHER_LIBS interrogatedb:c dconfig:c dtoolconfig:m \
                   dtoolutil:c dtoolbase:c dtool:m

#begin lib_target
  #define TARGET wgldisplay
  #define LOCAL_LIBS \
    display putil windisplay glgsg
    
  #define COMBINED_SOURCES $[TARGET]_composite1.cxx 
  
  #define INSTALL_HEADERS \
     config_wgldisplay.h \
     wglGraphicsPipe.I wglGraphicsPipe.h \
     wglGraphicsWindow.I wglGraphicsWindow.h
//     Win32Defs.h  
    
  #define INCLUDED_SOURCES \
    config_wgldisplay.cxx wglGraphicsPipe.cxx wglGraphicsWindow.cxx

  #define SOURCES \
    $[INSTALL_HEADERS]
//    wglext.h


#end lib_target
