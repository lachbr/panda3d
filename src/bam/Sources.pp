#begin bin_target
  #define TARGET bam-info
  #define LOCAL_LIBS \
    progbase
  #define OTHER_LIBS \
    parametrics:c collide:c chan:c char:c \
    loader:c egg:c pnmimagetypes:c \
    putil:c linmath:c express:c panda:m pandaexpress:m \
    interrogatedb:c dtoolutil:c dtoolbase:c dconfig:c dtoolconfig:m dtool:m pystub
  #define UNIX_SYS_LIBS \
    m

  #define SOURCES \
    bamInfo.cxx bamInfo.h

  #define INSTALL_HEADERS \

#end bin_target

#begin bin_target
  #define TARGET egg2bam
  #define LOCAL_LIBS \
    eggbase progbase
  #define OTHER_LIBS \
    dgraph:c \
    loader:c egg2pg:c builder:c egg:c pnmimagetypes:c gobj:c \
    chan:c putil:c linmath:c \
    express:c pandaegg:m panda:m pandaexpress:m \
    interrogatedb:c dtoolutil:c dtoolbase:c dconfig:c dtoolconfig:m dtool:m pystub
  #define UNIX_SYS_LIBS \
    m

  #define SOURCES \
    eggToBam.cxx eggToBam.h

#end bin_target


#if // Temporarily broken until we can bring it into the new scene graph.
#begin bin_target
  #define TARGET bam2egg
  #define LOCAL_LIBS \
    converter eggbase progbase
  #define OTHER_LIBS \
    egg:c pandaegg:m \
    parametrics:c collide:c chan:c char:c \
    loader:c gobj:c pnmimagetypes:c pstatclient:c \
    putil:c linmath:c express:c panda:m pandaexpress:m \
    interrogatedb:c dtoolutil:c dtoolbase:c dconfig:c dtoolconfig:m dtool:m pystub

  #define UNIX_SYS_LIBS \
    m

  #define SOURCES \
    bamToEgg.cxx bamToEgg.h

#end bin_target
#endif // Temporarily broken until we can bring it into the new scene graph.
