#pragma once

#include "config_networksystem.h"
#include "referenceCount.h"
#include "datagram.h"

#include "network_peer.h"

#ifndef CPPPARSER
#include "enet/enet.h"
#else
class ENetEvent;
#endif

class EXPCL_NETWORKSYSTEM NetworkEvent : public ReferenceCount {

PUBLISHED:
  enum EventType {
    ET_none,
    ET_connect,
    ET_disconnect,
    ET_receive,
  };

  NetworkEvent() = default;

  NetworkPeer *get_peer() const;
  EventType get_type() const;
  const Datagram &get_datagram() const;

private:
  void copy_packet_to_datagram();

private:
  ENetEvent _event;
  Datagram _dg;

  friend class NetworkHost;
};

INLINE NetworkPeer *NetworkEvent::
get_peer() const {
  return (NetworkPeer *)_event.peer->data;
}

INLINE NetworkEvent::EventType NetworkEvent::
get_type() const {
  return (EventType)_event.type;
}

INLINE const Datagram &NetworkEvent::
get_datagram() const {
  return _dg;
}

INLINE void NetworkEvent::
copy_packet_to_datagram() {
  if (!_event.packet) {
    return;
  }

  _dg = Datagram(_event.packet->data, _event.packet->dataLength);
}
