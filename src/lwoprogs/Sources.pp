#begin bin_target
  #define TARGET lwo2egg
  #define LOCAL_LIBS lwo lwoegg eggbase progbase

  #define OTHER_LIBS \
    egg:c pandaegg:m \
    linmath:c panda:m \
    express:c pandaexpress:m \
    dtoolutil:c dconfig:c dtoolconfig:m dtool:m pystub

  #define SOURCES \
    lwoToEgg.cxx lwoToEgg.h

#end bin_target

#begin bin_target
  #define TARGET lwo-scan
  #define LOCAL_LIBS lwo progbase

  #define SOURCES \
    lwoScan.cxx lwoScan.h

#end bin_target
