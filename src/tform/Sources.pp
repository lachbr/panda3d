#define OTHER_LIBS interrogatedb:c dconfig:c dtoolconfig:m \
                   dtoolutil:c dtoolbase:c dtool:m

#begin lib_target
  #define TARGET tform
  #define LOCAL_LIBS \
    dgraph pgraph linmath display event putil gobj gsgbase \
    mathutil device

  #define COMBINED_SOURCES $[TARGET]_composite1.cxx $[TARGET]_composite2.cxx 

  #define SOURCES  \
    buttonThrower.h \
    config_tform.h \
    driveInterface.I driveInterface.h \
    mouseWatcher.I mouseWatcher.h \
    mouseWatcherGroup.h \
    mouseWatcherParameter.I mouseWatcherParameter.h \
    mouseWatcherRegion.I mouseWatcherRegion.h \
    trackball.h \
    transform2sg.h  
     
  #define INCLUDED_SOURCES  \
    buttonThrower.cxx \
    config_tform.cxx \
    driveInterface.cxx \
    mouseWatcher.cxx \
    mouseWatcherGroup.cxx \
    mouseWatcherParameter.cxx mouseWatcherRegion.cxx  \
    trackball.cxx \
    transform2sg.cxx 

  #define INSTALL_HEADERS \
    buttonThrower.h \
    driveInterface.I driveInterface.h \
    mouseWatcher.I mouseWatcher.h \
    mouseWatcherGroup.h \
    mouseWatcherParameter.I mouseWatcherParameter.h \
    mouseWatcherRegion.I mouseWatcherRegion.h \
    trackball.h \
    transform2sg.h

  #define IGATESCAN all

#end lib_target

