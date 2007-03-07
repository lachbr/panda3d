// Filename: queuedReturn.h
// Created by:  drose (25Feb00)
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

#ifndef QUEUEDRETURN_H
#define QUEUEDRETURN_H

#include "pandabase.h"

#include "connectionListener.h"
#include "connection.h"
#include "netAddress.h"
#include "pmutex.h"
#include "pdeque.h"
#include "config_net.h"
#include "mutexHolder.h"

#include <algorithm>

////////////////////////////////////////////////////////////////////
//       Class : QueuedReturn
// Description : This is the implementation of a family of things that
//               queue up their return values for later retrieval by
//               client code, like QueuedConnectionReader,
//               QueuedConnectionListener, QueuedConnectionManager.
////////////////////////////////////////////////////////////////////
template<class Thing>
class QueuedReturn {
PUBLISHED:
  void set_max_queue_size(int max_size);
  int get_max_queue_size() const;
  int get_current_queue_size() const;

  bool get_overflow_flag() const;
  void reset_overflow_flag();

protected:
  QueuedReturn();
  ~QueuedReturn();

  INLINE bool thing_available() const;
  bool get_thing(Thing &thing);

  bool enqueue_thing(const Thing &thing);
  bool enqueue_unique_thing(const Thing &thing);

private:
  Mutex _mutex;
  pdeque<Thing> _things;
  bool _available;
  int _max_queue_size;
  bool _overflow_flag;
};

#include "queuedReturn.I"

#endif

