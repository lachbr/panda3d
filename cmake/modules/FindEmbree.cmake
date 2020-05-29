# Filename: FindEmbree.cmake
# Authors: lachbr (13 May, 2020)
#
# Usage:
#   find_package(Embree [REQUIRED] [QUIET])
#
# Once done this will define:
#   EMBREE_FOUND       - system has Embree
#   EMBREE_INCLUDE_DIR - the include directory containing embree3/rtcore.h
#   EMBREE_LIBRARY     - the path to the Embree library
#   EMBREE_TBB_LIBRARY - the path to the TBB library
#

find_path(EMBREE_INCLUDE_DIR
  NAMES "embree3/rtcore.h"
  PATH_SUFFIXES "embree")

find_library(EMBREE_LIBRARY
  NAMES "embree3")

find_library(EMBREE_TBB_LIBRARY
  NAMES "tbb")

mark_as_advanced(EMBREE_INCLUDE_DIR EMBREE_LIBRARY EMBREE_TBB_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Embree DEFAULT_MSG EMBREE_INCLUDE_DIR EMBREE_LIBRARY EMBREE_TBB_LIBRARY)

message("${EMBREE_FOUND}")
if(EMBREE_FOUND)
  set(EMBREE_LIBRARIES ${EMBREE_LIBRARY} ${EMBREE_TBB_LIBRARY})
  mark_as_advanced(EMBREE_LIBRARY EMBREE_TBB_LIBRARY)
endif()
