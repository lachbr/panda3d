#define LOCAL_LIBS \
  dcparser
#define OTHER_LIBS \
  express:c pandaexpress:m \
  interrogatedb:c dconfig:c dtoolconfig:m \
  dtoolutil:c dtoolbase:c dtool:m \
  pystub \
  prc:c express:c 

#define C++FLAGS -DWITHIN_PANDA

#begin bin_target
  #define TARGET dcparse

  #define SOURCES \
    dcparse.cxx
#end bin_target

