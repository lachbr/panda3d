/* dtool_config.h.  Generated automatically by CMake. */

/* Debug / non-debug symbols.  OPTIMIZE = $[OPTIMIZE] */
#cmakedefine _DEBUG
#cmakedefine NDEBUG

/* Define if we have Eigen available. */
#cmakedefine HAVE_EIGEN
#cmakedefine LINMATH_ALIGN

/* Define if we have Python installed.  */
#cmakedefine HAVE_PYTHON
#cmakedefine USE_DEBUG_PYTHON
/* Define if we have Python as a framework (Mac OS X).  */
#cmakedefine PYTHON_FRAMEWORK

/* Define if we have RAD game tools, Miles Sound System installed.  */
#cmakedefine HAVE_RAD_MSS

/* Define if we have FMODex installed. */
#cmakedefine HAVE_FMODEX

/* Define if we have OpenAL installed. */
#cmakedefine HAVE_OPENAL

/* Define if we have Freetype 2.0 or better available. */
#cmakedefine HAVE_FREETYPE

/* Define if we are using SpeedTree. */
#cmakedefine HAVE_SPEEDTREE

/* Define if we want to compile in a default font. */
#cmakedefine COMPILE_IN_DEFAULT_FONT

/* Define to use doubles for most numbers, intead of single-precision floats. */
#cmakedefine STDFLOAT_DOUBLE

/* Define if we have Maya available. */
#cmakedefine HAVE_MAYA
#cmakedefine MAYA_PRE_5_0

/* Define if we have libRocket available and built with the Rocket Debugger. */
#cmakedefine HAVE_ROCKET_DEBUGGER

/* Define if we have built libRocket available and built with Python support. */
#cmakedefine HAVE_ROCKET_PYTHON

/* Define if we have SoftImage available. */
#cmakedefine HAVE_SOFTIMAGE

/* Define if we have FCollada available. */
#cmakedefine HAVE_FCOLLADA

/* Define if we have ARToolKit available. */
#cmakedefine HAVE_ARTOOLKIT

/* Define if we have libvorbisfile available. */
#cmakedefine HAVE_VORBIS

/* Define if we have OpenSSL installed.  */
#cmakedefine HAVE_OPENSSL
#cmakedefine REPORT_OPENSSL_ERRORS

/* Define if we have libjpeg installed.  */
#cmakedefine HAVE_JPEG
#cmakedefine PHAVE_JPEGINT_H

/* Define to build video-for-linux. */
#cmakedefine HAVE_VIDEO4LINUX

/* Define if we have libpng installed.  */
#cmakedefine HAVE_PNG

/* Define if we have libtiff installed.  */
#cmakedefine HAVE_TIFF

/* Define if we want to build these other image file formats. */
#cmakedefine HAVE_SGI_RGB
#cmakedefine HAVE_TGA
#cmakedefine HAVE_IMG
#cmakedefine HAVE_SOFTIMAGE_PIC
#cmakedefine HAVE_BMP
#cmakedefine HAVE_PNM

/* Define if we have libtar installed.  */
#cmakedefine HAVE_TAR

/* Define if we have libfftw installed.  */
#cmakedefine HAVE_FFTW

/* Define if we have libsquish installed.  */
#cmakedefine HAVE_SQUISH

/* Define if we have Berkeley DB installed.  */
#cmakedefine HAVE_BDB

/* Define if we have HELIX installed.  */
#cmakedefine HAVE_HELIX

/* Define if we have CG installed.  */
#cmakedefine HAVE_CG

/* Define if we have CGGL installed.  */
#cmakedefine HAVE_CGGL

/* Define if we have CGDX8 installed.  */
#cmakedefine HAVE_CGDX8

/* Define if we have CGDX9 installed.  */
#cmakedefine HAVE_CGDX9

/* Define if we have CGDX10 installed.  */
#cmakedefine HAVE_CGDX10

/* Define for dxerr.h instead of dxerr9.h. */
#cmakedefine USE_GENERIC_DXERR_LIBRARY

/* Define if we have zlib installed.  */
#cmakedefine HAVE_ZLIB

/* Define if we have OpenGL installed and want to build for GL.  */
#cmakedefine HAVE_GL
#if HAVE_GL
#cmakedefine MIN_GL_VERSION_MAJOR
#cmakedefine MIN_GL_VERSION_MINOR
#endif

/* Define if we have OpenGL ES installed and want to build for GLES. */
#cmakedefine HAVE_GLES

/* Define if we have OpenGL ES installed and want to build for GLES2. */
#cmakedefine HAVE_GLES2

/* Define if we have OpenCV installed and want to build for OpenCV.  */
#cmakedefine HAVE_OPENCV
#cmakedefine OPENCV_VER_23

/* Define if we have FFMPEG installed and want to build for FFMPEG.  */
#cmakedefine HAVE_FFMPEG
#cmakedefine HAVE_SWSCALE
#cmakedefine HAVE_SWRESAMPLE

/* Define if we have ODE installed and want to build for ODE.  */
#cmakedefine HAVE_ODE

/* Define if we have AWESOMIUM installed and want to build for AWESOMIUM.  */
#cmakedefine HAVE_AWESOMIUM

/* Define if we have Mesa installed and want to build mesadisplay.  */
#cmakedefine HAVE_MESA
#cmakedefine MESA_MGL
#if HAVE_MESA
#cmakedefine MIN_MESA_VERSION_MAJOR
#cmakedefine MIN_MESA_VERSION_MINOR
#endif

/* Define if we have GLX installed and want to build for GLX.  */
#cmakedefine HAVE_GLX

/* Define if we have EGL installed and want to build for EGL.  */
#cmakedefine HAVE_EGL

/* Define if we have Windows-GL installed and want to build for Wgl.  */
#cmakedefine HAVE_WGL

/* Define if we have DirectX installed and want to build for DX.  */
#cmakedefine HAVE_DX8

/* Define if we have DirectX installed and want to build for DX.  */
#cmakedefine HAVE_DX9

/* The choice of generic vs. the specific dxerr library largely
   depends on which SDK you have installed. */
#cmakedefine USE_GENERIC_DXERR_LIBRARY

/* Define if we want to build tinydisplay. */
#cmakedefine HAVE_TINYDISPLAY

/* Define if we have the SDL library. */
#cmakedefine HAVE_SDL

/* Define if we have X11. */
#cmakedefine HAVE_X11

/* Define if we have the XFree86-DGA extension. */
#cmakedefine HAVE_XF86DGA

/* Define if we have the XRandR extension. */
#cmakedefine HAVE_XRANDR

/* Define if we have the XCursor extension. */
#cmakedefine HAVE_XCURSOR

/* Define if we want to compile the threading code.  */
#cmakedefine HAVE_THREADS

/* Define if we want to use fast, user-space simulated threads.  */
#cmakedefine SIMPLE_THREADS

/* Define if SIMPLE_THREADS should be implemented with the OS-provided
   threading layer (if available). */
#cmakedefine OS_SIMPLE_THREADS

/* Define to enable deadlock detection, mutex recursion checks, etc. */
#cmakedefine DEBUG_THREADS

/* Define to implement mutexes and condition variables via a user-space spinlock. */
#cmakedefine MUTEX_SPINLOCK

/* Define to enable the PandaFileStream implementation of pfstream etc. */
#cmakedefine USE_PANDAFILESTREAM

/* Define if we want to compile the net code.  */
#cmakedefine HAVE_NET

/* Define if we want to compile the egg code.  */
#cmakedefine HAVE_EGG

/* Define if we want to compile the audio code.  */
#cmakedefine HAVE_AUDIO

/* Define if we have bison and flex available. */
#cmakedefine HAVE_BISON

/* Define if we want to use PStats.  */
#cmakedefine DO_PSTATS

/* Define if we want to type-check downcasts.  */
#cmakedefine DO_DCAST

/* Define if we want to provide collision system recording and
   visualization tools. */
#cmakedefine DO_COLLISION_RECORDING

/* Define if we want to enable track-memory-usage.  */
#cmakedefine DO_MEMORY_USAGE

/* Define if we want to enable min-lag and max-lag.  */
#cmakedefine SIMULATE_NETWORK_DELAY

/* Define if we want to allow immediate mode OpenGL rendering.  */
#cmakedefine SUPPORT_IMMEDIATE_MODE

/* Define for either of the alternative malloc schemes. */
#cmakedefine USE_MEMORY_DLMALLOC
#cmakedefine USE_MEMORY_PTMALLOC2

/* Define if we want to compile in support for pipelining.  */
#cmakedefine DO_PIPELINING

/* Define if we want to keep Notify debug messages around, or undefine
   to compile them out.  */
#cmakedefine NOTIFY_DEBUG

/* Define if we want to export template classes from the DLL.  Only
   makes sense to MSVC++. */
#cmakedefine EXPORT_TEMPLATES

/* Define if we are linking PANDAPHYSX in with PANDA. */
#cmakedefine LINK_IN_PHYSX

/* The compiled-in character(s) to expect to separate different
   components of a path list (e.g. $PRC_PATH). */
#cmakedefine DEFAULT_PATHSEP

/* Many of the prc variables are exported by
   dtool/src/prc/prc_parameters.h.pp, instead of here.  Only those prc
   variables that must be visible outside of the prc directory are
   exported here. */

/* The filename that specifies the public keys to import into
   config. */
#cmakedefine PRC_PUBLIC_KEYS_FILENAME
#cmakedefine PRC_PUBLIC_KEYS_INCLUDE

/* Define if you want to save the descriptions for ConfigVariables. */
#cmakedefine PRC_SAVE_DESCRIPTIONS


/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */
#cmakedefine WORDS_BIGENDIAN

/* Define if the C++ compiler uses namespaces.  */
#cmakedefine HAVE_NAMESPACE

/* Define if fstream::open() accepts a third parameter for umask. */
#cmakedefine HAVE_OPEN_MASK

/* Define if we have a lockf() function. */
#cmakedefine HAVE_LOCKF

/* Define if some header file defines wchar_t. */
#cmakedefine HAVE_WCHAR_T

/* Define if the <string> header file defines wstring. */
#cmakedefine HAVE_WSTRING

/* Define if the C++ compiler supports the typename keyword.  */
#cmakedefine HAVE_TYPENAME

/* Define if we can trust the compiler not to insert extra bytes in
   structs between base structs and derived structs. */
#cmakedefine SIMPLE_STRUCT_POINTERS

/* Define if we have Dinkumware STL installed.  */
#cmakedefine HAVE_DINKUM

/* Define if we have STL hash_map etc. available  */
#cmakedefine HAVE_STL_HASH

/* Define if we have a gettimeofday() function. */
#cmakedefine HAVE_GETTIMEOFDAY

/* Define if gettimeofday() takes only one parameter. */
#cmakedefine GETTIMEOFDAY_ONE_PARAM

/* Define if you have the getopt function.  */
#cmakedefine HAVE_GETOPT

/* Define if you have the getopt_long_only function.  */
#cmakedefine HAVE_GETOPT_LONG_ONLY

/* Define if getopt appears in getopt.h.  */
#cmakedefine PHAVE_GETOPT_H

/* Define if you have ioctl(TIOCGWINSZ) to determine terminal width. */
#cmakedefine IOCTL_TERMINAL_WIDTH

/* Do the system headers define a "streamsize" typedef? */
#cmakedefine HAVE_STREAMSIZE

/* Do the system headers define key ios typedefs like ios::openmode
   and ios::fmtflags? */
#cmakedefine HAVE_IOS_TYPEDEFS

/* Define if the C++ iostream library defines ios::binary.  */
#cmakedefine HAVE_IOS_BINARY

/* Can we safely call getenv() at static init time? */
#cmakedefine STATIC_INIT_GETENV

/* Can we read the file /proc/self/[*] to determine our
   environment variables at static init time? */
#cmakedefine HAVE_PROC_SELF_EXE
#cmakedefine HAVE_PROC_SELF_MAPS
#cmakedefine HAVE_PROC_SELF_ENVIRON
#cmakedefine HAVE_PROC_SELF_CMDLINE
#cmakedefine HAVE_PROC_CURPROC_FILE
#cmakedefine HAVE_PROC_CURPROC_MAP
#cmakedefine HAVE_PROC_CURPROC_CMDLINE

/* Do we have a global pair of argc/argv variables that we can read at
   static init time?  Should we prototype them?  What are they called? */
#cmakedefine HAVE_GLOBAL_ARGV
#cmakedefine PROTOTYPE_GLOBAL_ARGV
#cmakedefine GLOBAL_ARGV
#cmakedefine GLOBAL_ARGC

/* Define if you have the <io.h> header file.  */
#cmakedefine PHAVE_IO_H

/* Define if you have the <iostream> header file.  */
#cmakedefine PHAVE_IOSTREAM

/* Define if you have the <malloc.h> header file.  */
#cmakedefine PHAVE_MALLOC_H

/* Define if you have the <sys/malloc.h> header file.  */
#cmakedefine PHAVE_SYS_MALLOC_H

/* Define if you have the <alloca.h> header file.  */
#cmakedefine PHAVE_ALLOCA_H

/* Define if you have the <locale.h> header file.  */
#cmakedefine PHAVE_LOCALE_H

/* Define if you have the <string.h> header file.  */
#cmakedefine PHAVE_STRING_H

/* Define if you have the <stdlib.h> header file.  */
#cmakedefine PHAVE_STDLIB_H

/* Define if you have the <limits.h> header file.  */
#cmakedefine PHAVE_LIMITS_H

/* Define if you have the <minmax.h> header file.  */
#cmakedefine PHAVE_MINMAX_H

/* Define if you have the <sstream> header file.  */
#cmakedefine PHAVE_SSTREAM

/* Define if you have the <new> header file.  */
#cmakedefine PHAVE_NEW

/* Define if you have the <sys/types.h> header file.  */
#cmakedefine PHAVE_SYS_TYPES_H

/* Define if you have the <sys/time.h> header file.  */
#cmakedefine PHAVE_SYS_TIME_H

/* Define if you have the <unistd.h> header file.  */
#cmakedefine PHAVE_UNISTD_H

/* Define if you have the <utime.h> header file.  */
#cmakedefine PHAVE_UTIME_H

/* Define if you have the <glob.h> header file.  */
#cmakedefine PHAVE_GLOB_H

/* Define if you have the <dirent.h> header file.  */
#cmakedefine PHAVE_DIRENT_H

/* Define if you have the <drfftw.h> header file.  */
#cmakedefine PHAVE_DRFFTW_H

/* Do we have <sys/soundcard.h> (and presumably a Linux-style audio
   interface)? */
#cmakedefine PHAVE_SYS_SOUNDCARD_H

/* Do we have <ucontext.h> (and therefore makecontext() /
   swapcontext())? */
#cmakedefine PHAVE_UCONTEXT_H

/* Do we have <linux/input.h> ? This enables us to use raw mouse input. */
#cmakedefine PHAVE_LINUX_INPUT_H

/* Do we have <stdint.h>? */
#cmakedefine PHAVE_STDINT_H

/* Do we have RTTI (and <typeinfo>)? */
#cmakedefine HAVE_RTTI

/* Do we have Posix threads? */
#cmakedefine HAVE_POSIX_THREADS

/* Is the code being compiled with the Tau profiler's instrumentor? */
#cmakedefine USE_TAU

/* Define if needed to have 64-bit file i/o */
#cmakedefine __USE_LARGEFILE64

// To activate the DELETED_CHAIN macros.
#cmakedefine USE_DELETED_CHAIN

// To build the Windows TOUCHINPUT interfaces (requires Windows 7).
#cmakedefine HAVE_WIN_TOUCHINPUT

// If we are to build the native net interfaces.
#cmakedefine WANT_NATIVE_NET

/* Turn off warnings for using scanf and such */
#if 0
	#cmakedefine _CRT_SECURE_NO_WARNINGS
        #pragma warning( disable : 4996 4275 4267 4099 4049 4013 4005 )
#endif

/* Static linkage instead of the normal dynamic linkage? */
#cmakedefine LINK_ALL_STATIC

/* Define to compile the plugin code. */
#cmakedefine HAVE_P3D_PLUGIN

/* Define to compile for Cocoa or Carbon on Mac OS X. */
#cmakedefine HAVE_COCOA
#cmakedefine HAVE_CARBON

/* Platform-identifying defines. */
#cmakedefine IS_OSX
#cmakedefine IS_LINUX
#cmakedefine IS_FREEBSD
#cmakedefine BUILD_IPHONE
#cmakedefine UNIVERSAL_BINARIES
