# Filename: FindGameNetworkingSockets.cmake
# Authors: lachbr (13 May, 2020)
#
# Usage:
#   find_package(GameNetworkingSockets [REQUIRED] [QUIET])
#
# Once done this will define:
#   GNS_FOUND       - system has GNS
#   GNS_INCLUDE_DIR - the include directory containing steam/isteamnetworkingsockets.h
#   GNS_LIBRARY     - the path to the GNS library
#

find_path(GNS_INCLUDE_DIR
  NAMES "steam/isteamnetworkingsockets.h"
  PATH_SUFFIXES "gamenetworkingsockets")

find_library(GNS_LIBRARY
  NAMES "GameNetworkingSockets_s")

mark_as_advanced(GNS_INCLUDE_DIR GNS_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GameNetworkingSockets DEFAULT_MSG GNS_INCLUDE_DIR GNS_LIBRARY)
