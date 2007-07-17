// Filename: config_nativenet.cxx
// Created by:  drose (01Mar07)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001 - 2004, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://etc.cmu.edu/panda3d/docs/license/ .
//
// To contact the maintainers of this program write to
// panda3d-general@lists.sourceforge.net .
//
////////////////////////////////////////////////////////////////////

#include "config_nativenet.h"

#include "socket_ip.h"
#include "socket_tcp.h"
#include "socket_tcp_listen.h"
#include "socket_tcp_ssl.h"
#include "socket_udp_incoming.h"
#include "socket_udp_outgoing.h"
#include "socket_udp.h"
#include "socket_portable.h"
#include "buffered_datagramconnection.h"
#include "pandaSystem.h"

#include "dconfig.h"

Configure(config_nativenet);
NotifyCategoryDef(nativenet, "");

ConfigureFn(config_nativenet) {
  init_libnativenet();
}


////////////////////////////////////////////////////////////////////
//     Function: init_libnativenet
//  Description: Initializes the library.  This must be called at
//               least once before any of the functions or classes in
//               this library can be used.  Normally it will be
//               called by the static initializers and need not be
//               called explicitly, but special cases exist.
////////////////////////////////////////////////////////////////////
void
init_libnativenet() {
  static bool initialized = false;
  if (initialized) {
    return;
  }
  initialized = true;

  Socket_IP::init_type();
  Socket_TCP::init_type();
  Socket_TCP_Listen::init_type();
#ifdef HAVE_OPENSSL
  Socket_TCP_SSL::init_type();
#endif
  Socket_UDP_Incoming::init_type();
  Socket_UDP_Outgoing::init_type();
  Socket_UDP::init_type();
  Buffered_DatagramConnection::init_type();

  PandaSystem *ps = PandaSystem::get_global_ptr();
  ps->add_system("nativenet");

  init_network();
}

