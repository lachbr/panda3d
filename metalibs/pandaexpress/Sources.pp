// DIR_TYPE "metalib" indicates we are building a shared library that
// consists mostly of references to other shared libraries.  Under
// Windows, this directly produces a DLL (as opposed to the regular
// src libraries, which don't produce anything but a pile of OBJ files
// under Windows).

#define DIR_TYPE metalib
#define BUILDING_DLL BUILDING_PANDAEXPRESS
#define USE_NET yes

#define COMPONENT_LIBS downloader event ipc express pandabase
#define OTHER_LIBS dtool

#begin metalib_target
  #define TARGET pandaexpress

  #define SOURCES pandaexpress.cxx
#end metalib_target
