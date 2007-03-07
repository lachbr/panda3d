// Filename: connectionManager.cxx
// Created by:  jns (07Feb00)
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

#include "connectionManager.h"
#include "connection.h"
#include "connectionReader.h"
#include "connectionWriter.h"
#include "netAddress.h"
#include "config_net.h"
#include "mutexHolder.h"

#ifdef WIN32_VC
#include <winsock.h>  // For gethostname()
#endif

////////////////////////////////////////////////////////////////////
//     Function: ConnectionManager::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
ConnectionManager::
ConnectionManager() {
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionManager::Destructor
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
ConnectionManager::
~ConnectionManager() {
  // Notify all of our associated readers and writers that we're gone.
  Readers::iterator ri;
  for (ri = _readers.begin(); ri != _readers.end(); ++ri) {
    (*ri)->clear_manager();
  }
  Writers::iterator wi;
  for (wi = _writers.begin(); wi != _writers.end(); ++wi) {
    (*wi)->clear_manager();
  }
}


////////////////////////////////////////////////////////////////////
//     Function: ConnectionManager::open_UDP_connection
//       Access: Public
//  Description: Opens a socket for sending and/or receiving UDP
//               packets.  If the port number is greater than zero,
//               the UDP connection will be opened for listening on
//               the indicated port; otherwise, it will be useful only
//               for sending.
//
//               Use a ConnectionReader and ConnectionWriter to handle
//               the actual communication.
////////////////////////////////////////////////////////////////////
PT(Connection) ConnectionManager::
open_UDP_connection(int port) {
  Socket_UDP *socket = new Socket_UDP;

  if (port > 0) {
    NetAddress address;
    address.set_any(port);
    
    if (!socket->OpenForInput(address.get_addr())) {
      net_cat.error()
        << "Unable to bind to port " << port << " for UDP.\n";
      delete socket;
      return PT(Connection)();
    }

    net_cat.info()
      << "Creating UDP connection for port " << port << "\n";

  } else {
    if (!socket->InitNoAddress()) {
      net_cat.error()
        << "Unable to initialize outgoing UDP.\n";
      delete socket;
      return PT(Connection)();
    }

    net_cat.info()
      << "Creating outgoing UDP connection\n";
  }

  PT(Connection) connection = new Connection(this, socket);
  new_connection(connection);
  return connection;
}


////////////////////////////////////////////////////////////////////
//     Function: ConnectionManager::open_TCP_server_rendezvous
//       Access: Public
//  Description: Creates a socket to be used as a rendezvous socket
//               for a server to listen for TCP connections.  The
//               socket returned by this call should only be added to
//               a ConnectionListener (not to a generic
//               ConnectionReader).
//
//               backlog is the maximum length of the queue of pending
//               connections.
////////////////////////////////////////////////////////////////////
PT(Connection) ConnectionManager::
open_TCP_server_rendezvous(int port, int backlog) {
  NetAddress address;
  address.set_any(port);

  Socket_TCP_Listen *socket = new Socket_TCP_Listen;
  bool okflag = socket->OpenForListen(address.get_addr(), backlog);
  if (!okflag) {
    net_cat.info()
      << "Unable to listen to port " << port << " for TCP.\n";
    delete socket;
    return PT(Connection)();
  }

  net_cat.info()
    << "Listening for TCP connections on port " << port << "\n";

  PT(Connection) connection = new Connection(this, socket);
  new_connection(connection);
  return connection;
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionManager::open_TCP_client_connection
//       Access: Public
//  Description: Attempts to establish a TCP client connection to a
//               server at the indicated address.  If the connection
//               is not established within timeout_ms milliseconds, a
//               null connection is returned.
////////////////////////////////////////////////////////////////////
PT(Connection) ConnectionManager::
open_TCP_client_connection(const NetAddress &address, int timeout_ms) {
  Socket_TCP *socket = new Socket_TCP;
  bool okflag = socket->ActiveOpen(address.get_addr());
  if (!okflag) {
    net_cat.error()
      << "Unable to open TCP connection to server "
      << address.get_ip_string() << " on port " << address.get_port() << "\n";
    delete socket;
    return PT(Connection)();
  }

  net_cat.info()
    << "Opened TCP connection to server " << address.get_ip_string() << " "
    << " on port " << address.get_port() << "\n";

  PT(Connection) connection = new Connection(this, socket);
  new_connection(connection);
  return connection;
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionManager::open_TCP_client_connection
//       Access: Public
//  Description: This is a shorthand version of the function to
//               directly establish communications to a named host and
//               port.
////////////////////////////////////////////////////////////////////
PT(Connection) ConnectionManager::
open_TCP_client_connection(const string &hostname, int port,
                           int timeout_ms) {
  NetAddress address;
  if (!address.set_host(hostname, port)) {
    return PT(Connection)();
  }

  return open_TCP_client_connection(address, timeout_ms);
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionManager::close_connection
//       Access: Public
//  Description: Terminates a UDP or TCP socket previously opened.
//               This also removes it from any associated
//               ConnectionReader or ConnectionListeners.
//
//               The socket itself may not be immediately closed--it
//               will not be closed until all outstanding pointers to
//               it are cleared, including any pointers remaining in
//               NetDatagrams recently received from the socket.
//
//               The return value is true if the connection was marked
//               to be closed, or false if close_connection() had
//               already been called (or the connection did not belong
//               to this ConnectionManager).  In neither case can you
//               infer anything about whether the connection has
//               *actually* been closed yet based on the return value.
////////////////////////////////////////////////////////////////////
bool ConnectionManager::
close_connection(const PT(Connection) &connection) {
  if (connection != (Connection *)NULL) {
    connection->flush();
  }

  {
    MutexHolder holder(_set_mutex);
    Connections::iterator ci = _connections.find(connection);
    if (ci == _connections.end()) {
      // Already closed, or not part of this ConnectionManager.
      return false;
    }
    _connections.erase(ci);
    
    Readers::iterator ri;
    for (ri = _readers.begin(); ri != _readers.end(); ++ri) {
      (*ri)->remove_connection(connection);
    }
  }

  Socket_IP *socket = connection->get_socket();

  // We can't *actually* close the connection right now, because
  // there might be outstanding pointers to it.  But we can at least
  // shut it down.  It will be eventually closed when all the
  // pointers let go.
  
  net_cat.info()
    << "Shutting down connection " << (void *)connection
    << " locally.\n";
  socket->Close();

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionManager::get_host_name
//       Access: Public, Static
//  Description: Returns the name of this particular machine on the
//               network, if available, or the empty string if the
//               hostname cannot be determined.
////////////////////////////////////////////////////////////////////
string ConnectionManager::
get_host_name() {
  char temp_buff[1024];
  if (gethostname(temp_buff, 1024) == 0) {
    return string(temp_buff);
  }

  return string();
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionManager::new_connection
//       Access: Protected
//  Description: This internal function is called whenever a new
//               connection is established.  It allows the
//               ConnectionManager to save all of the pointers to open
//               connections so they can't be inadvertently deleted
//               until close_connection() is called.
////////////////////////////////////////////////////////////////////
void ConnectionManager::
new_connection(const PT(Connection) &connection) {
  MutexHolder holder(_set_mutex);
  _connections.insert(connection);
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionManager::connection_reset
//       Access: Protected, Virtual
//  Description: An internal function called by the ConnectionReader,
//               ConnectionWriter, or ConnectionListener when a
//               connection has been externally reset.  This adds the
//               connection to the queue of those which have recently
//               been reset.
////////////////////////////////////////////////////////////////////
void ConnectionManager::
connection_reset(const PT(Connection) &connection, bool okflag) {
  if (net_cat.is_info()) {
    if (okflag) {
      net_cat.info()
        << "Connection " << (void *)connection
        << " was closed normally by the other end";

    } else {
      net_cat.info()
        << "Lost connection " << (void *)connection
        << " unexpectedly\n";
    }
  }

  // Turns out we do need to explicitly mark the connection as closed
  // immediately, rather than waiting for the user to do it, since
  // otherwise we'll keep trying to listen for noise on the socket and
  // we'll always here a "yes" answer.
  close_connection(connection);
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionManager::add_reader
//       Access: Protected
//  Description: This internal function is called by ConnectionReader
//               when it is constructed.
////////////////////////////////////////////////////////////////////
void ConnectionManager::
add_reader(ConnectionReader *reader) {
  MutexHolder holder(_set_mutex);
  _readers.insert(reader);
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionManager::remove_reader
//       Access: Protected
//  Description: This internal function is called by ConnectionReader
//               when it is destructed.
////////////////////////////////////////////////////////////////////
void ConnectionManager::
remove_reader(ConnectionReader *reader) {
  MutexHolder holder(_set_mutex);
  _readers.erase(reader);
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionManager::add_writer
//       Access: Protected
//  Description: This internal function is called by ConnectionWriter
//               when it is constructed.
////////////////////////////////////////////////////////////////////
void ConnectionManager::
add_writer(ConnectionWriter *writer) {
  MutexHolder holder(_set_mutex);
  _writers.insert(writer);
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionManager::remove_writer
//       Access: Protected
//  Description: This internal function is called by ConnectionWriter
//               when it is destructed.
////////////////////////////////////////////////////////////////////
void ConnectionManager::
remove_writer(ConnectionWriter *writer) {
  MutexHolder holder(_set_mutex);
  _writers.erase(writer);
}
