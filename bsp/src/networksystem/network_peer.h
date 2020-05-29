#pragma once

#include "config_networksystem.h"
#include "referenceCount.h"
#include "datagram.h"

#ifndef CPPPARSER
#include "enet/enet.h"
#else
class ENetPeer;
#endif

class EXPCL_NETWORKSYSTEM NetworkPeer : public ReferenceCount {
PUBLISHED:
  enum PacketFlags {
    PF_reliable = 1 << 0,
    PF_unsequencee = 1 << 1,
    PF_no_allocate = 1 << 2,
    PF_unreliable_fragment = 1 << 3,
    PF_sent = 1 << 8,
  };

  ~NetworkPeer();

  void reset();
  void disconnect();

  void send(const Datagram &dg, PacketFlags flags = PF_reliable);
private:
  NetworkPeer(ENetPeer *peer);

private:
  ENetPeer *_peer;

  friend class NetworkHost;
};

INLINE NetworkPeer::
NetworkPeer(ENetPeer *peer) :
  _peer(peer) {

  // Attach ourself
  _peer->data = this;
}
