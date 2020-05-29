#include "network_host.h"
#include "enet_utils.h"

NetworkHost::
NetworkHost(const NetAddress &bind_address, int max_clients, int max_channels,
            int incoming_bandwith, int outgoing_bandwith) {

  _event = nullptr;

  ENetAddress enaddr = enet_address_from_net_address(bind_address);
  _host = enet_host_create(&enaddr, max_clients, max_channels,
                           incoming_bandwith, outgoing_bandwith);
  if (!_host) {
    nassert_raise("Unable to create host!");
  }

}

NetworkHost::
NetworkHost(int max_clients, int max_channels,
            int incoming_bandwith, int outgoing_bandwith) {

  _event = nullptr;

  _host = enet_host_create(nullptr, max_clients, max_channels,
                           incoming_bandwith, outgoing_bandwith);
  if (!_host) {
    nassert_raise("Unable to create host!");
  }
}

NetworkHost::
~NetworkHost() {
  if (_host) {
    enet_host_destroy(_host);
    _host = nullptr;
  }
}

bool NetworkHost::
service(uint32_t timeout) {
  nassertr(_host != nullptr, false);

  _event = new NetworkEvent;
  if (enet_host_service(_host, &_event->_event, timeout) > 0) {
    // Got an event!

    // Copy the received packet if we got one.
    _event->copy_packet_to_datagram();

    return true;
  }

  _event = nullptr;

  return false;
}

PT(NetworkPeer) NetworkHost::
connect(const NetAddress &addr, int num_channels) {
  nassertr(_host != nullptr, nullptr);

  ENetAddress enaddr = enet_address_from_net_address(addr);
  ENetPeer *peer = enet_host_connect(_host, &enaddr, num_channels, 0);
  nassertr(peer != nullptr, nullptr);
  PT(NetworkPeer) npeer = new NetworkPeer(peer);
  return npeer;
}
