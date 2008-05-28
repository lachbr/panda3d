// Filename: connectionReader.cxx
// Created by:  drose (08Feb00)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) Carnegie Mellon University.  All rights reserved.
//
// All use of this software is subject to the terms of the revised BSD
// license.  You should have received a copy of this license along
// with this source code in a file named "LICENSE."
//
////////////////////////////////////////////////////////////////////

#include "connectionReader.h"
#include "connectionManager.h"
#include "netDatagram.h"
#include "datagramTCPHeader.h"
#include "datagramUDPHeader.h"
#include "config_net.h"
#include "trueClock.h"
#include "socket_udp.h"
#include "socket_tcp.h"
#include "mutexHolder.h"
#include "pnotify.h"
#include "atomicAdjust.h"

static const int read_buffer_size = maximum_udp_datagram + datagram_udp_header_size;

static const int max_timeout_ms = 100;

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::SocketInfo::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
ConnectionReader::SocketInfo::
SocketInfo(const PT(Connection) &connection) :
  _connection(connection)
{
  _busy = false;
  _error = false;
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::SocketInfo::is_udp
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
bool ConnectionReader::SocketInfo::
is_udp() const {
  return (_connection->get_socket()->is_exact_type(Socket_UDP::get_class_type()));
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::SocketInfo::get_socket
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
Socket_IP *ConnectionReader::SocketInfo::
get_socket() const {
  return _connection->get_socket();
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::ReaderThread::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
ConnectionReader::ReaderThread::
ReaderThread(ConnectionReader *reader, int thread_index) :
  Thread("ReaderThread", "ReaderThread"),
  _reader(reader),
  _thread_index(thread_index)
{
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::ReaderThread::thread_main
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
void ConnectionReader::ReaderThread::
thread_main() {
  _reader->thread_run(_thread_index);
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::Constructor
//       Access: Public
//  Description: Creates a new ConnectionReader with the indicated
//               number of threads to handle requests.  If num_threads
//               is 0, the sockets will only be read by polling,
//               during an explicit poll() call.
//               (QueuedConnectionReader will do this automatically.)
////////////////////////////////////////////////////////////////////
ConnectionReader::
ConnectionReader(ConnectionManager *manager, int num_threads) :
  _manager(manager)
{
  if (!Thread::is_true_threads()) {
    // There is no point in using threads for this kind of I/O unless
    // we actually have real threads available (i.e. HAVE_THREADS is
    // defined, and SIMPLE_THREADS is not).
#ifndef NDEBUG
    if (num_threads != 0) {
      if (net_cat.is_debug()) {
        net_cat.debug()
          << "Threading support is not available.\n";
      }
    }
#endif  // NDEBUG
    num_threads = 0;
  }

  _raw_mode = false;
  _tcp_header_size = datagram_tcp16_header_size;
  _polling = (num_threads <= 0);

  _shutdown = false;

  _next_index = 0;
  _num_results = 0;

  _currently_polling_thread = -1;

  int i;
  for (i = 0; i < num_threads; i++) {
    PT(ReaderThread) thread = new ReaderThread(this, i);
    _threads.push_back(thread);
  }
  for (i = 0; i < num_threads; i++) {
    _threads[i]->start(TP_normal, true);
  }

  _manager->add_reader(this);
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::Destructor
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
ConnectionReader::
~ConnectionReader() {
  if (_manager != (ConnectionManager *)NULL) {
    _manager->remove_reader(this);
  }

  shutdown();

  // Delete all of our old sockets.
  Sockets::iterator si;
  for (si = _sockets.begin(); si != _sockets.end(); ++si) {
    delete (*si);
  }
  for (si = _removed_sockets.begin(); si != _removed_sockets.end(); ++si) {
    SocketInfo *sinfo = (*si);
    if (!sinfo->_busy) {
      delete sinfo;
    } else {
      net_cat.error()
        << "Reentrant deletion of ConnectionReader--don't delete these\n"
        << "in response to connection_reset().\n";

      // We'll have to do the best we can to recover.
      sinfo->_connection.clear();
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::add_connection
//       Access: Public
//  Description: Adds a new socket to the list of sockets the
//               ConnectionReader will monitor.  A datagram that comes
//               in on any of the monitored sockets will be reported.
//               In the case of a ConnectionListener, this adds a new
//               rendezvous socket; any activity on any of the
//               monitored sockets will cause a connection to be
//               accepted.
//
//               The return value is true if the connection was added,
//               false if it was already there.
//
//               add_connection() is thread-safe, and may be called at
//               will by any thread.
////////////////////////////////////////////////////////////////////
bool ConnectionReader::
add_connection(Connection *connection) {
  nassertr(connection != (Connection *)NULL, false);

  MutexHolder holder(_sockets_mutex);

  // Make sure it's not already on the _sockets list.
  Sockets::const_iterator si;
  for (si = _sockets.begin(); si != _sockets.end(); ++si) {
    if ((*si)->_connection == connection) {
      // Whoops, already there.
      return false;
    }
  }

  _sockets.push_back(new SocketInfo(connection));

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::remove_connection
//       Access: Public
//  Description: Removes a socket from the list of sockets being
//               monitored.  Returns true if the socket was correctly
//               removed, false if it was not on the list in the first
//               place.
//
//               remove_connection() is thread-safe, and may be called
//               at will by any thread.
////////////////////////////////////////////////////////////////////
bool ConnectionReader::
remove_connection(Connection *connection) {
  MutexHolder holder(_sockets_mutex);

  // Walk through the list of sockets to find the one we're removing.
  Sockets::iterator si;
  si = _sockets.begin();
  while (si != _sockets.end() && (*si)->_connection != connection) {
    ++si;
  }
  if (si == _sockets.end()) {
    return false;
  }

  _removed_sockets.push_back(*si);
  _sockets.erase(si);

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::is_connection_ok
//       Access: Public
//  Description: Returns true if the indicated connection has been
//               added to the ConnectionReader and is being monitored
//               properly, false if it is not known, or if there was
//               some error condition detected on the connection.  (If
//               there was an error condition, normally the
//               ConnectionManager would have been informed and closed
//               the connection.)
////////////////////////////////////////////////////////////////////
bool ConnectionReader::
is_connection_ok(Connection *connection) {
  MutexHolder holder(_sockets_mutex);

  // Walk through the list of sockets to find the one we're asking
  // about.
  Sockets::iterator si;
  si = _sockets.begin();
  while (si != _sockets.end() && (*si)->_connection != connection) {
    ++si;
  }
  if (si == _sockets.end()) {
    // Don't know that connection.
    return false;
  }

  SocketInfo *sinfo = (*si);
  bool is_ok = !sinfo->_error;

  return is_ok;
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::poll
//       Access: Public
//  Description: Explicitly polls the available sockets to see if any
//               of them have any noise.  This function does nothing
//               unless this is a polling-type ConnectionReader,
//               i.e. it was created with zero threads (and
//               is_polling() will return true).
//
//               It is not necessary to call this explicitly for a
//               QueuedConnectionReader.
////////////////////////////////////////////////////////////////////
void ConnectionReader::
poll() {
  if (!_polling) {
    return;
  }

  SocketInfo *sinfo = get_next_available_socket(false, -2);
  if (sinfo != (SocketInfo *)NULL) {
    double max_poll_cycle = get_max_poll_cycle();
    if (max_poll_cycle < 0.0) {
      // Continue to read all data.
      while (sinfo != (SocketInfo *)NULL) {
	process_incoming_data(sinfo);
	sinfo = get_next_available_socket(false, -2);
      }

    } else {
      // Read only until a certain amount of time has elapsed.
      TrueClock *global_clock = TrueClock::get_global_ptr();
      double stop = global_clock->get_short_time() + max_poll_cycle;

      while (sinfo != (SocketInfo *)NULL) {
	process_incoming_data(sinfo);
	if (global_clock->get_short_time() >= stop) {
	  return;
	}
	sinfo = get_next_available_socket(false, -2);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::get_manager
//       Access: Public
//  Description: Returns a pointer to the ConnectionManager object
//               that serves this ConnectionReader.
////////////////////////////////////////////////////////////////////
ConnectionManager *ConnectionReader::
get_manager() const {
  return _manager;
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::is_polling
//       Access: Public
//  Description: Returns true if the reader is a polling reader,
//               i.e. it has no threads.
////////////////////////////////////////////////////////////////////
bool ConnectionReader::
is_polling() const {
  return _polling;
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::get_num_threads
//       Access: Public
//  Description: Returns the number of threads the ConnectionReader
//               has been created with.
////////////////////////////////////////////////////////////////////
int ConnectionReader::
get_num_threads() const {
  return _threads.size();
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::set_raw_mode
//       Access: Public
//  Description: Sets the ConnectionReader into raw mode (or turns off
//               raw mode).  In raw mode, datagram headers are not
//               expected; instead, all the data available on the pipe
//               is treated as a single datagram.
//
//               This is similar to set_tcp_header_size(0), except that it
//               also turns off headers for UDP packets.
////////////////////////////////////////////////////////////////////
void ConnectionReader::
set_raw_mode(bool mode) {
  _raw_mode = mode;
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::get_raw_mode
//       Access: Public
//  Description: Returns the current setting of the raw mode flag.
//               See set_raw_mode().
////////////////////////////////////////////////////////////////////
bool ConnectionReader::
get_raw_mode() const {
  return _raw_mode;
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::set_tcp_header_size
//       Access: Public
//  Description: Sets the header size of TCP packets.  At the present,
//               legal values for this are 0, 2, or 4; this specifies
//               the number of bytes to use encode the datagram length
//               at the start of each TCP datagram.  Sender and
//               receiver must independently agree on this.
////////////////////////////////////////////////////////////////////
void ConnectionReader::
set_tcp_header_size(int tcp_header_size) {
  _tcp_header_size = tcp_header_size;
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::get_tcp_header_size
//       Access: Public
//  Description: Returns the current setting of TCP header size.
//               See set_tcp_header_size().
////////////////////////////////////////////////////////////////////
int ConnectionReader::
get_tcp_header_size() const {
  return _tcp_header_size;
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::shutdown
//       Access: Protected
//  Description: Terminates all threads cleanly.  Normally this is
//               only called by the destructor.
////////////////////////////////////////////////////////////////////
void ConnectionReader::
shutdown() {
  if (_shutdown) {
    return;
  }

  // First, begin the shutdown.  This will tell our threads we want
  // them to quit.
  _shutdown = true;

  // Now wait for all of our threads to terminate.
  Threads::iterator ti;
  for (ti = _threads.begin(); ti != _threads.end(); ++ti) {
    (*ti)->join();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::clear_manager
//       Access: Protected
//  Description: This should normally only be called when the
//               associated ConnectionManager destructs.  It resets
//               the ConnectionManager pointer to NULL so we don't
//               have a floating pointer.  This makes the
//               ConnectionReader invalid; presumably it also will be
//               destructed momentarily.
////////////////////////////////////////////////////////////////////
void ConnectionReader::
clear_manager() {
  _manager = (ConnectionManager *)NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::finish_socket
//       Access: Protected
//  Description: To be called when a socket has been fully read and is
//               ready for polling for additional data.
////////////////////////////////////////////////////////////////////
void ConnectionReader::
finish_socket(SocketInfo *sinfo) {
  nassertv(sinfo->_busy);

  // By marking the SocketInfo nonbusy, we make it available for
  // future polls.
  sinfo->_busy = false;
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::process_incoming_data
//       Access: Protected, Virtual
//  Description: This is run within a thread when the call to
//               select() indicates there is data available on a
//               socket.
////////////////////////////////////////////////////////////////////
void ConnectionReader::
process_incoming_data(SocketInfo *sinfo) {
  if (_raw_mode) {
    if (sinfo->is_udp()) {
      process_raw_incoming_udp_data(sinfo);
    } else {
      process_raw_incoming_tcp_data(sinfo);
    }
  } else {
    if (sinfo->is_udp()) {
      process_incoming_udp_data(sinfo);
    } else {
      process_incoming_tcp_data(sinfo);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::process_incoming_udp_data
//       Access: Protected
//  Description:
////////////////////////////////////////////////////////////////////
void ConnectionReader::
process_incoming_udp_data(SocketInfo *sinfo) {
  Socket_UDP *socket;
  DCAST_INTO_V(socket, sinfo->get_socket());
  Socket_Address addr;

  // Read as many bytes as we can.
  char buffer[read_buffer_size];
  int bytes_read = read_buffer_size;

  bool okflag = socket->GetPacket(buffer, &bytes_read, addr);

  if (!okflag) {
    finish_socket(sinfo);
    return;

  } else if (bytes_read == 0) {
    // The socket was closed (!).  This shouldn't happen with a UDP
    // connection.  Oh well.  Report that and return.
    if (_manager != (ConnectionManager *)NULL) {
      _manager->connection_reset(sinfo->_connection, 0);
    }
    finish_socket(sinfo);
    return;
  }

  // Since we are not running in raw mode, we decode the header to
  // determine how big the datagram is.  This means we must have read
  // at least a full header.
  if (bytes_read < datagram_udp_header_size) {
    net_cat.error()
      << "Did not read entire header, discarding UDP datagram.\n";
    finish_socket(sinfo);
    return;
  }
  
  DatagramUDPHeader header(buffer);
  
  char *dp = buffer + datagram_udp_header_size;
  bytes_read -= datagram_udp_header_size;
  
  NetDatagram datagram(dp, bytes_read);
  
  // Now that we've read all the data, it's time to finish the socket
  // so another thread can read the next datagram.
  finish_socket(sinfo);
  
  if (_shutdown) {
    return;
  }
  
  // And now do whatever we need to do to process the datagram.
  if (!header.verify_datagram(datagram)) {
    net_cat.error()
      << "Ignoring invalid UDP datagram.\n";
  } else {
    datagram.set_connection(sinfo->_connection);
    datagram.set_address(NetAddress(addr));
    receive_datagram(datagram);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::process_incoming_tcp_data
//       Access: Protected
//  Description:
////////////////////////////////////////////////////////////////////
void ConnectionReader::
process_incoming_tcp_data(SocketInfo *sinfo) {
  Socket_TCP *socket;
  DCAST_INTO_V(socket, sinfo->get_socket());

  // Read only the header bytes to start with.
  char buffer[read_buffer_size];
  int header_bytes_read = 0;

  // First, we have to read the first _tcp_header_size bytes.
  while (header_bytes_read < _tcp_header_size) {
    int bytes_read =
      socket->RecvData(buffer + header_bytes_read,
                       _tcp_header_size - header_bytes_read);

    if (bytes_read <= 0) {
      // The socket was closed.  Report that and return.
      if (_manager != (ConnectionManager *)NULL) {
        _manager->connection_reset(sinfo->_connection, 0);
      }
      finish_socket(sinfo);
      return;
    }

    header_bytes_read += bytes_read;
  }

  // Now we must decode the header to determine how big the datagram
  // is.  This means we must have read at least a full header.
  if (header_bytes_read != _tcp_header_size) {
    // This should actually be impossible, by the read-loop logic
    // above.
    net_cat.error()
      << "Did not read entire header, discarding TCP datagram.\n";
    finish_socket(sinfo);
    return;
  }

  DatagramTCPHeader header(buffer, _tcp_header_size);
  int size = header.get_datagram_size(_tcp_header_size);

  // We have to loop until the entire datagram is read.
  NetDatagram datagram;

  while (!_shutdown && (int)datagram.get_length() < size) {
    int bytes_read;

    bytes_read =
      socket->RecvData(buffer, min(read_buffer_size,
                                   (int)(size - datagram.get_length())));
    char *dp = buffer;

    if (bytes_read <= 0) {
      // The socket was closed.  Report that and return.
      if (_manager != (ConnectionManager *)NULL) {
        _manager->connection_reset(sinfo->_connection, 0);
      }
      finish_socket(sinfo);
      return;
    }

    int datagram_bytes =
      min(bytes_read, (int)(size - datagram.get_length()));
    datagram.append_data(dp, datagram_bytes);

    if (bytes_read > datagram_bytes) {
      // There were some extra bytes at the end of the datagram.  Maybe
      // the beginning of the next datagram?  Huh.
      net_cat.error()
        << "Discarding " << bytes_read - datagram_bytes
        << " bytes following TCP datagram.\n";
    }
  }

  // Now that we've read all the data, it's time to finish the socket
  // so another thread can read the next datagram.
  finish_socket(sinfo);

  if (_shutdown) {
    return;
  }

  // And now do whatever we need to do to process the datagram.
  if (!header.verify_datagram(datagram, _tcp_header_size)) {
    net_cat.error()
      << "Ignoring invalid TCP datagram.\n";
  } else {
    datagram.set_connection(sinfo->_connection);
    datagram.set_address(NetAddress(socket->GetPeerName()));
    receive_datagram(datagram);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::process_raw_incoming_udp_data
//       Access: Protected
//  Description:
////////////////////////////////////////////////////////////////////
void ConnectionReader::
process_raw_incoming_udp_data(SocketInfo *sinfo) {
  Socket_UDP *socket;
  DCAST_INTO_V(socket, sinfo->get_socket());
  Socket_Address addr;

  // Read as many bytes as we can.
  char buffer[read_buffer_size];
  int bytes_read = read_buffer_size;

  bool okflag = socket->GetPacket(buffer, &bytes_read, addr);

  if (!okflag) {
    finish_socket(sinfo);
    return;

  } else if (bytes_read == 0) {
    // The socket was closed (!).  This shouldn't happen with a UDP
    // connection.  Oh well.  Report that and return.
    if (_manager != (ConnectionManager *)NULL) {
      _manager->connection_reset(sinfo->_connection, 0);
    }
    finish_socket(sinfo);
    return;
  }

  // In raw mode, we simply extract all the bytes and make that a
  // datagram.
  NetDatagram datagram(buffer, bytes_read);
  
  // Now that we've read all the data, it's time to finish the socket
  // so another thread can read the next datagram.
  finish_socket(sinfo);
  
  if (_shutdown) {
    return;
  }
  
  datagram.set_connection(sinfo->_connection);
  datagram.set_address(NetAddress(addr));
  receive_datagram(datagram);
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::process_raw_incoming_tcp_data
//       Access: Protected
//  Description:
////////////////////////////////////////////////////////////////////
void ConnectionReader::
process_raw_incoming_tcp_data(SocketInfo *sinfo) {
  Socket_TCP *socket;
  DCAST_INTO_V(socket, sinfo->get_socket());

  // Read as many bytes as we can.
  char buffer[read_buffer_size];
  int bytes_read = socket->RecvData(buffer, read_buffer_size);

  if (bytes_read <= 0) {
    // The socket was closed.  Report that and return.
    if (_manager != (ConnectionManager *)NULL) {
      _manager->connection_reset(sinfo->_connection, 0);
    }
    finish_socket(sinfo);
    return;
  }

  // In raw mode, we simply extract all the bytes and make that a
  // datagram.
  NetDatagram datagram(buffer, bytes_read);
  
  // Now that we've read all the data, it's time to finish the socket
  // so another thread can read the next datagram.
  finish_socket(sinfo);
  
  if (_shutdown) {
    return;
  }
  
  datagram.set_connection(sinfo->_connection);
  datagram.set_address(NetAddress(socket->GetPeerName()));
  receive_datagram(datagram);
}

////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::thread_run
//       Access: Private
//  Description: This is the actual executing function for each
//               thread.
////////////////////////////////////////////////////////////////////
void ConnectionReader::
thread_run(int thread_index) {
  nassertv(!_polling);
  nassertv(_threads[thread_index] == Thread::get_current_thread());

  while (!_shutdown) {
    SocketInfo *sinfo =
      get_next_available_socket(false, thread_index);
    if (sinfo != (SocketInfo *)NULL) {
      process_incoming_data(sinfo);
    }
  }
}


////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::get_next_available_socket
//       Access: Private
//  Description: Polls the known connections for activity and returns
//               the next one known to have activity, or NULL if no
//               activity is detected within the timeout interval.
//
//               This function may block indefinitely if it is being
//               called by multiple threads; if there are no other
//               threads, it may block only if allow_block is true.
////////////////////////////////////////////////////////////////////
ConnectionReader::SocketInfo *ConnectionReader::
get_next_available_socket(bool allow_block, int current_thread_index) {
  // Go to sleep on the select() mutex.  This guarantees that only one
  // thread is in this function at a time.
  MutexHolder holder(_select_mutex);

  do {
    // First, check the result from the previous select call.  If
    // there are any sockets remaining there, process them first.
    while (!_shutdown && _num_results > 0) {
      nassertr(_next_index < (int)_selecting_sockets.size(), NULL);
      int i = _next_index;
      _next_index++;

      if (_fdset.IsSetFor(*_selecting_sockets[i]->get_socket())) {
        _num_results--;
        SocketInfo *sinfo = _selecting_sockets[i];

        // Some noise on this socket.
        sinfo->_busy = true;
        return sinfo;
      }
    }

    bool interrupted;
    do {
      interrupted = false;

      // Ok, no results from previous select calls.  Prepare to set up
      // for a new select.

      // First, report to anyone else who cares that we're the thread
      // about to do the poll.  That way, if any new sockets come
      // available while we're polling, we can service them.
      AtomicAdjust::set(_currently_polling_thread, current_thread_index);
      
      rebuild_select_list();

      // Now we can execute the select.
      _num_results = 0;
      _next_index = 0;

      if (!_shutdown) {
        PN_uint32 timeout = max_timeout_ms;
        if (!allow_block) {
          timeout = 0;
        }

        _num_results = _fdset.WaitForRead(false, timeout);
      }

      if (_num_results == 0 && allow_block) {
        // If we reached max_timeout_ms, go back and reconsider.  (We
        // never timeout indefinitely, so we can check the shutdown
        // flag every once in a while.)
        interrupted = true;

      } else if (_num_results < 0) {
        // If we had an error, just return.
        return (SocketInfo *)NULL;
      }
    } while (!_shutdown && interrupted);

    AtomicAdjust::set(_currently_polling_thread, current_thread_index);

    // Repeat the above until we (a) find a socket with actual noise
    // on it, or (b) return from PR_Poll() with no sockets available.
  } while (!_shutdown && _num_results > 0);

  return (SocketInfo *)NULL;
}


////////////////////////////////////////////////////////////////////
//     Function: ConnectionReader::rebuild_select_list
//       Access: Private
//  Description: Rebuilds the _fdset and _selecting_sockets arrays
//               based on the sockets that are currently available for
//               selecting.
////////////////////////////////////////////////////////////////////
void ConnectionReader::
rebuild_select_list() {
  _fdset.clear();
  _selecting_sockets.clear();

  MutexHolder holder(_sockets_mutex);
  Sockets::const_iterator si;
  for (si = _sockets.begin(); si != _sockets.end(); ++si) {
    SocketInfo *sinfo = (*si);
    if (!sinfo->_busy && !sinfo->_error) {
      _fdset.setForSocket(*sinfo->get_socket());
      _selecting_sockets.push_back(sinfo);
    }
  }

  // This is also a fine time to delete the contents of the
  // _removed_sockets list.
  if (!_removed_sockets.empty()) {
    Sockets still_busy_sockets;
    for (si = _removed_sockets.begin(); si != _removed_sockets.end(); ++si) {
      SocketInfo *sinfo = (*si);
      if (sinfo->_busy) {
        still_busy_sockets.push_back(sinfo);
      } else {
        delete sinfo;
      }
    }
    _removed_sockets.swap(still_busy_sockets);
  }
}
