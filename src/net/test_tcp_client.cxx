// Filename: test_tcp_client.cxx
// Created by:  drose (09Feb00)
// 
////////////////////////////////////////////////////////////////////

#include "queuedConnectionManager.h"
#include "queuedConnectionReader.h"
#include "connectionWriter.h"
#include "netAddress.h"
#include "connection.h"
#include "netDatagram.h"

#include "datagram_ui.h"

int
main(int argc, char *argv[]) {
  if (argc != 3) {
    nout << "test_tcp_client host port\n";
    exit(1);
  }

  string hostname = argv[1];
  int port = atoi(argv[2]);

  NetAddress host;
  if (!host.set_host(hostname, port)) {
    nout << "Unknown host: " << hostname << "\n";
  }

  QueuedConnectionManager cm;
  PT(Connection) c = cm.open_TCP_client_connection(host, 5000);

  if (c.is_null()) {
    nout << "No connection.\n";
    exit(1);
  }

  nout << "Successfully opened TCP connection to " << hostname 
       << " on port " << port << "\n";

  QueuedConnectionReader reader(&cm, 0);
  reader.add_connection(c);
  ConnectionWriter writer(&cm, 0);

  NetDatagram datagram;
  cout << "Enter a datagram.\n";
  cin >> datagram;

  bool lost_connection = false;

  while (!cin.fail() && !lost_connection) {
    // Send the datagram.
    writer.send(datagram, c);
    
    // Check for a lost connection.
    while (cm.reset_connection_available()) {
      PT(Connection) connection;
      if (cm.get_reset_connection(connection)) {
	nout << "Lost connection from "
	     << connection->get_address().get_ip() << "\n";
	cm.close_connection(connection);
	if (connection == c) {
	  lost_connection = true;
	}
      }
    }

    // Now poll for new datagrams on the socket.
    while (reader.data_available()) {
      if (reader.get_data(datagram)) {
	nout << "Got datagram " << datagram << "from " 
	     << datagram.get_address().get_ip() << "\n";
      }
    }

    if (!lost_connection) {
      cout << "\nEnter a datagram.\n";
      cin >> datagram;
    }
  }
  nout << "Exiting\n";

  return (0);
}



    
  
