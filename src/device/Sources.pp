#define OTHER_LIBS interrogatedb:c dconfig:c dtoolconfig:m \
                   dtoolutil:c dtoolbase:c dtool:m
				   
#if $[eq $[USE_COMPILER], INTEL]
#define USE_COMPILER MSVC
#include $[DTOOL]/pptempl/compilerSettings.pp
#endif				   

#begin lib_target
  #define TARGET device
  #define LOCAL_LIBS \
    dgraph display gobj sgraph graph gsgbase ipc mathutil linmath putil

  #define SOURCES \
    analogNode.I analogNode.cxx analogNode.h \
    buttonNode.I buttonNode.h buttonNode.cxx \
    clientAnalogDevice.I clientAnalogDevice.cxx clientAnalogDevice.h \
    clientBase.I clientBase.cxx clientBase.h \
    clientButtonDevice.I clientButtonDevice.cxx clientButtonDevice.h \
    clientDevice.I clientDevice.cxx clientDevice.h \
    clientDialDevice.I clientDialDevice.cxx clientDialDevice.h \
    clientTrackerDevice.I clientTrackerDevice.cxx clientTrackerDevice.h \
    config_device.cxx config_device.h \
    dialNode.I dialNode.h dialNode.cxx \
    mouse.cxx mouse.h \
    trackerData.I trackerData.cxx trackerData.h \
    trackerNode.I trackerNode.cxx trackerNode.h

  #define INSTALL_HEADERS \
    analogNode.I analogNode.h \
    buttonNode.I buttonNode.h \
    clientAnalogDevice.I clientAnalogDevice.h \
    clientBase.I clientBase.h \
    clientButtonDevice.I clientButtonDevice.h \
    clientDevice.I clientDevice.h \
    clientDialDevice.I clientDialDevice.h \
    clientTrackerDevice.I clientTrackerDevice.h \
    config_device.h mouse.h \
    dialNode.I dialNode.h \
    trackerData.I trackerData.h \
    trackerNode.I trackerNode.h

  #define IGATESCAN all

#end lib_target

