// Filename: eventQueue.h
// Created by:  drose (08Feb99)
//
////////////////////////////////////////////////////////////////////

#ifndef EVENTQUEUE_H
#define EVENTQUEUE_H

#include <pandabase.h>

#include "event.h"
#include "pt_Event.h"

#include <circBuffer.h>

#ifdef HAVE_IPC
#include <ipc_mutex.h>
#endif

////////////////////////////////////////////////////////////////////
// 	 Class : EventQueue
// Description : A queue of pending events.  As events are thrown,
//               they are added to this queue; eventually, they will
//               be extracted out again by an EventHandler and
//               processed.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAEXPRESS EventQueue {
public:
  enum { max_events = 500 };

  EventQueue();
  ~EventQueue();

PUBLISHED:
  void queue_event(CPT_Event event);

  bool is_queue_empty() const;
  CPT_Event dequeue_event();

  INLINE static EventQueue *
  get_global_event_queue();

protected:
  CircBuffer<CPT_Event, max_events> _queue;
 
  static void make_global_event_queue();
  static EventQueue *_global_event_queue;

#ifdef HAVE_IPC
  mutex _lock;
#endif
};

#include "eventQueue.I"

#endif
