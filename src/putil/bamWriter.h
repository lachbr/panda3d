// Filename: bamWriter.h
// Created by:  jason (08Jun00)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://www.panda3d.org/license.txt .
//
// To contact the maintainers of this program write to
// panda3d@yahoogroups.com .
//
////////////////////////////////////////////////////////////////////

#ifndef __BAM_WRITER_
#define __BAM_WRITER_

#include <pandabase.h>
#include <notify.h>

#include "typedWritable.h"
#include "datagramSink.h"
#include "pdeque.h"

// A handy macro for writing PointerToArrays.
#define WRITE_PTA(Manager, dest, Write_func, array)  \
  if (!Manager->register_pta(dest, array.p()))       \
  {                                                  \
    Write_func(dest, array);                         \
  }                                                  \


////////////////////////////////////////////////////////////////////
//       Class : BamWriter
// Description : This is the fundamental interface for writing binary
//               objects to a Bam file, to be extracted later by a
//               BamReader.
//
//               A Bam file can be thought of as a linear collection
//               of objects.  Each object is an instance of a class
//               that inherits, directly or indirectly, from
//               TypedWritable.  The objects may include pointers to
//               other objects; the BamWriter automatically manages
//               these (with help from code within each class) and
//               writes all referenced objects to the file in such a
//               way that the pointers may be correctly restored
//               later.
//
//               This is the abstract interface and does not
//               specifically deal with disk files, but rather with a
//               DatagramSink of some kind, which simply accepts a
//               linear stream of Datagrams.  It is probably written
//               to a disk file, but it might conceivably be streamed
//               directly to a network or some such nonsense.
//
//               Bam files are most often used to store scene graphs
//               or subgraphs, and by convention they are given
//               filenames ending in the extension ".bam" when they
//               are used for this purpose.  However, a Bam file may
//               store any arbitrary list of TypedWritable objects;
//               in this more general usage, they are given filenames
//               ending in ".boo" to differentiate them from the more
//               common scene graph files.
//
//               See also BamFile, which defines a higher-level
//               interface to read and write Bam files on disk.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA BamWriter{
public:
  BamWriter(DatagramSink *sink);
  ~BamWriter();

  // The primary interface for a caller.

  bool init();
  bool write_object(const TypedWritable *obj);

public:
  // Functions to support classes that write themselves to the Bam.

  void write_pointer(Datagram &packet, const TypedWritable *dest);
  bool register_pta(Datagram &packet, const void *ptr);
  void write_handle(Datagram &packet, TypeHandle type);

private:
  int enqueue_object(const TypedWritable *object);


  // This is the set of all TypeHandles already written.
  pset<int> _types_written;

  // This keeps track of all of the objects we have written out
  // already (or are about to write out), and associates a unique
  // object ID number to each one.
  class StoreState {
  public:
    int _object_id;
    bool _written;

    StoreState(int object_id) : _object_id(object_id), _written(false) {}
  };
  typedef pmap<const TypedWritable *, StoreState> StateMap;
  StateMap _state_map;

  // This is the next object ID that will be assigned to a new object.
  int _next_object_id;

  // This is the queue of objects that need to be written when the
  // current object is finished.
  typedef pdeque<const TypedWritable *> ObjectQueue;
  ObjectQueue _object_queue;

  // These are used by register_pta() to unify multiple references to
  // the same PointerToArray.
  typedef pmap<const void *, int> PTAMap;
  PTAMap _pta_map;
  int _next_pta_id;

  // The destination to write all the output to.
  DatagramSink *_target;
};

#include "bamWriter.I"

#endif

