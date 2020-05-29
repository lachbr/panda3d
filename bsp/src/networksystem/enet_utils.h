#pragma once

#include "socket_address.h"
#include "netAddress.h"
#ifndef CPPPARSER
#include "enet/enet.h"
#else
class ENetAddress;
#endif

ENetAddress enet_address_from_net_address(const NetAddress &addr) {
  ENetAddress enaddr;
  enet_address_set_host(&enaddr, addr.get_ip_string().c_str());
  enaddr.port = addr.get_port();
  return enaddr;
}

NetAddress net_address_from_enet_address(const ENetAddress &enaddr) {
  Socket_Address sock;
  sock.set_host(enaddr.host, enaddr.port);
  NetAddress addr(sock);
  return addr;
}
