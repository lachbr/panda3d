#pragma once

#include "referenceCount.h"
#include "netAddress.h"

#include "network_peer.h"
#include "network_event.h"

#include "config_networksystem.h"

#ifndef CPPPARSER
#include "enet/enet.h"
#else
class ENetHost;
#endif

class EXPCL_NETWORKSYSTEM NetworkHost : public ReferenceCount {
PUBLISHED:
  NetworkHost() = default;
  NetworkHost(const NetAddress &bind_address, int max_clients = 32, int max_channels = 1,
              int incoming_bandwith = 0, int outgoing_bandwith = 0);

  NetworkHost(int max_clients, int max_channels = 1,
              int incoming_bandwith = 0, int outgoing_bandwith = 0);

  ~NetworkHost();

  bool service(uint32_t timeout = 0);

  PT(NetworkPeer) connect(const NetAddress &addr, int num_channels = 1);

  NetworkEvent *get_event() const;

private:
  ENetHost *_host;

  PT(NetworkEvent) _event;
};

INLINE NetworkEvent *NetworkHost::
get_event() const {
  return _event;
}
