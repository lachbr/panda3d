#include "network_peer.h"

NetworkPeer::
~NetworkPeer() {
  if (_peer) {
    reset();
  }
}

void NetworkPeer::
reset() {
  enet_peer_reset(_peer);
}

void NetworkPeer::
disconnect() {
  enet_peer_disconnect(_peer, 0);
}

void NetworkPeer::
send(const Datagram &dg, NetworkPeer::PacketFlags flags) {
  ENetPacket packet;
  packet.data = (uint8_t *)dg.get_data();
  packet.dataLength = dg.get_length();
  packet.flags = flags;

  enet_peer_send(_peer, 0, &packet);
}
