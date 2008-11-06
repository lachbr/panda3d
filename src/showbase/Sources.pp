#begin lib_target
  #define TARGET showbase
  #define LOCAL_LIBS \
    directbase
  #define OTHER_LIBS \
    pgraph:c pgraphnodes:c gsgbase:c gobj:c mathutil:c pstatclient:c \
    lerp:c downloader:c pandabase:c pnmimage:c prc:c \
    pipeline:c cull:c \
    $[if $[HAVE_NET],net:c] $[if $[WANT_NATIVE_NET],nativenet:c] \
    display:c linmath:c event:c putil:c panda:m \
    express:c pandaexpress:m \
    interrogatedb:c dconfig:c dtoolconfig:m \
    dtoolutil:c dtoolbase:c dtool:m

  #define SOURCES \
    showBase.cxx showBase.h

  #define IGATESCAN all
#end lib_target

#if $[CTPROJS]
  #define INSTALL_SCRIPTS ppython
#endif

