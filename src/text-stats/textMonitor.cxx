// Filename: textMonitor.cxx
// Created by:  drose (12Jul00)
// 
////////////////////////////////////////////////////////////////////

#include "textMonitor.h"

#include <indent.h>

////////////////////////////////////////////////////////////////////
//     Function: TextMonitor::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
TextMonitor::
TextMonitor() {
}

////////////////////////////////////////////////////////////////////
//     Function: TextMonitor::get_monitor_name
//       Access: Public, Virtual
//  Description: Should be redefined to return a descriptive name for
//               the type of PStatsMonitor this is.
////////////////////////////////////////////////////////////////////
string TextMonitor::
get_monitor_name() {
  return "Text Stats";
}

////////////////////////////////////////////////////////////////////
//     Function: TextMonitor::got_hello
//       Access: Public, Virtual
//  Description: Called when the "hello" message has been received
//               from the client.  At this time, the client's hostname
//               and program name will be known.
////////////////////////////////////////////////////////////////////
void TextMonitor::
got_hello() {
  nout << "Now connected to " << get_client_progname() << " on host " 
       << get_client_hostname() << "\n";
}

////////////////////////////////////////////////////////////////////
//     Function: TextMonitor::got_bad_version
//       Access: Public, Virtual
//  Description: Like got_hello(), this is called when the "hello"
//               message has been received from the client.  At this
//               time, the client's hostname and program name will be
//               known.  However, the client appears to be an
//               incompatible version and the connection will be
//               terminated; the monitor should issue a message to
//               that effect.
////////////////////////////////////////////////////////////////////
void TextMonitor::
got_bad_version(int client_major, int client_minor,
                int server_major, int server_minor) {
  nout
    << "Rejected connection by " << get_client_progname()
    << " from " << get_client_hostname()
    << ".  Client uses PStats version "
    << client_major << "." << client_minor
    << ", while server expects PStats version "
    << server_major << "." << server_minor << ".\n";
}

////////////////////////////////////////////////////////////////////
//     Function: TextMonitor::new_data
//       Access: Public, Virtual
//  Description: Called as each frame's data is made available.  There
//               is no gurantee the frames will arrive in order, or
//               that all of them will arrive at all.  The monitor
//               should be prepared to accept frames received
//               out-of-order or missing.
////////////////////////////////////////////////////////////////////
void TextMonitor::
new_data(int thread_index, int frame_number) {
  PStatView &view = get_view(thread_index);
  const PStatThreadData *thread_data = view.get_thread_data();

  if (frame_number == thread_data->get_latest_frame_number()) {
    view.set_to_frame(frame_number);
    
    if (view.all_collectors_known()) {
      nout << "\rThread " 
           << get_client_data()->get_thread_name(thread_index)
           << " frame " << frame_number << ", "
           << view.get_net_value() * 1000.0 << " ms ("
           << thread_data->get_frame_rate() << " Hz):\n";
      const PStatViewLevel *level = view.get_top_level();
      int num_children = level->get_num_children();
      for (int i = 0; i < num_children; i++) {
        show_level(level->get_child(i), 2);
      }
    }
  }
}
  

////////////////////////////////////////////////////////////////////
//     Function: TextMonitor::lost_connection
//       Access: Public, Virtual
//  Description: Called whenever the connection to the client has been
//               lost.  This is a permanent state change.  The monitor
//               should update its display to represent this, and may
//               choose to close down automatically.
////////////////////////////////////////////////////////////////////
void TextMonitor::
lost_connection() {
  nout << "Lost connection.\n";
}

////////////////////////////////////////////////////////////////////
//     Function: TextMonitor::is_thread_safe
//       Access: Public, Virtual
//  Description: Should be redefined to return true if this monitor
//               class can handle running in a sub-thread.
//
//               This is not related to the question of whether it can
//               handle multiple different PStatThreadDatas; this is
//               strictly a question of whether or not the monitor
//               itself wants to run in a sub-thread.
////////////////////////////////////////////////////////////////////
bool TextMonitor::
is_thread_safe() {
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: TextMonitor::show_level
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
void TextMonitor::
show_level(const PStatViewLevel *level, int indent_level) {
  int collector_index = level->get_collector();

  indent(nout, indent_level)
    << get_client_data()->get_collector_name(collector_index)
    << " = " << level->get_net_value() * 1000.0 << " ms\n";

  int num_children = level->get_num_children();
  for (int i = 0; i < num_children; i++) {
    show_level(level->get_child(i), indent_level + 2);
  }
}
