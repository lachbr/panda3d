// Filename: bamWriter.cxx
// Created by:  jason (08Jun00)
//

#include <pandabase.h>
#include <notify.h>

#include "config_util.h"
#include "bam.h"
#include "bamWriter.h"

////////////////////////////////////////////////////////////////////
//     Function: BamWriter::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
BamWriter::
BamWriter(DatagramSink *sink) : 
  _target(sink)
{
}

////////////////////////////////////////////////////////////////////
//     Function: BamWriter::Destructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
BamWriter::
~BamWriter() {
}

////////////////////////////////////////////////////////////////////
//     Function: BamWriter::init
//       Access: Public
//  Description: Initializes the BamWriter prior to writing any
//               objects to its output stream.  This includes writing
//               out the Bam header.
//
//               This returns true if the BamWriter successfully
//               initialized, false otherwise.
////////////////////////////////////////////////////////////////////
bool BamWriter::
init() {
  // Initialize the next object and PTA ID's.  These start counting at
  // 1, since 0 is reserved for NULL.
  _next_object_id = 1;
  _next_pta_id = 1;

  // Write out the current major and minor BAM file version numbers.
  Datagram header;

  header.add_uint16(_bam_major_ver);
  header.add_uint16(_bam_minor_ver);

  if (!_target->put_datagram(header)) {
    util_cat.error()
      << "Unable to write Bam header.\n";
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: BamWriter::write_object
//       Access: Public
//  Description: Writes a single object to the Bam file, so that the
//               BamReader::read_object() can later correctly restore
//               the object and all its pointers.
//
//               This implicitly also writes any additional objects
//               this object references (if they haven't already been
//               written), so that pointers may be fully resolved.
//
//               This may be called repeatedly to write a sequence of
//               objects to the Bam file, but typically (especially
//               for scene graph files, indicated with the .bam
//               extension), only one object is written directly from
//               the Bam file: the root of the scene graph.  The
//               remaining objects will all be written recursively by
//               the first object.
//
//               Returns true if the object is successfully written,
//               false otherwise.
////////////////////////////////////////////////////////////////////
bool BamWriter::
write_object(TypedWritable *object) {
  nassertr(_object_queue.empty(), false);

  int object_id = enqueue_object(object);
  nassertr(object_id != 0, false);

  // Now we write out all the objects in the queue, in order.  The
  // first one on the queue will, of course, be this object we just
  // queued up, but each object we write may append more to the queue.
  while (!_object_queue.empty()) {
    object = _object_queue.front();
    _object_queue.pop_front();

    // Look up the object in the map.  It had better be there!
    StateMap::iterator si = _state_map.find(object);
    nassertr(si != _state_map.end(), false);

    int object_id = (*si).second._object_id;
    bool already_written = (*si).second._written;

    Datagram dg;

    if (!already_written) {
      // The first time we write a particular object, we do so by
      // writing its TypeHandle (which had better not be
      // TypeHandle::none(), since that's our code for a
      // previously-written object), followed by the object ID number,
      // followed by the object definition.

      TypeHandle type = object->get_type();
      nassertr(type != TypeHandle::none(), false);

      write_handle(dg, type);
      dg.add_uint16(object_id);

      object->write_datagram(this, dg);
      (*si).second._written = true;

    } else {
      // On subsequent times when we write a particular object, we
      // write simply TypeHandle::none(), followed by the object ID.
      // The occurrence of TypeHandle::none() is an indicator to the
      // BamReader that this is a previously-written object.

      write_handle(dg, TypeHandle::none());
      dg.add_uint16(object_id);
    }

    if (!_target->put_datagram(dg)) {
      util_cat.error() 
	<< "Unable to write datagram to file.\n";
      return false;
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: BamWriter::write_pointer
//       Access: Public
//  Description: The interface for writing a pointer to another object
//               to a Bam file.  This is intended to be called by the
//               various objects that write themselves to the Bam
//               file, within the write_datagram() method.
//
//               This writes the pointer out in such a way that the
//               BamReader will be able to restore the pointer later.
//               If the pointer is to an object that has not yet
//               itself been written to the Bam file, that object will
//               automatically be written.
////////////////////////////////////////////////////////////////////
void BamWriter::
write_pointer(Datagram &packet, TypedWritable *object) {
  // If the pointer is NULL, we always simply write a zero for an
  // object ID and leave it at that.
  if (object == (TypedWritable *)NULL) {
    packet.add_uint16(0);

  } else {
    StateMap::iterator si = _state_map.find(object);
    if (si == _state_map.end()) {
      // We have not written this pointer out yet.  This means we must
      // queue the object definition up for later.
      int object_id = enqueue_object(object);
      packet.add_uint16(object_id);

    } else {
      // We have already assigned this pointer an ID; thus, we can
      // simply write out the ID.
      packet.add_uint16((*si).second._object_id);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BamWriter::register_pta
//       Access: Public
//  Description: Prepares to write a PointerToArray to the Bam file,
//               unifying references to the same pointer across the
//               Bam file.
//
//               The writing object should call this prior to writing
//               out a PointerToArray.  It will return true if the
//               same pointer has been previously, in which case the
//               writing object need do nothing further; or it will
//               return false if this particular pointer has not yet
//               been written, in which case the writing object must
//               then write out the contents of the array.
//
//               Also see the WRITE_PTA() macro, which consolidates
//               the work that must be done to write a PTA.
////////////////////////////////////////////////////////////////////
bool BamWriter::
register_pta(Datagram &packet, void *ptr) {
  if (ptr == (void *)NULL) {
    // A zero for the PTA ID indicates a NULL pointer.  This is a
    // special case.
    packet.add_uint16(0);

    // We return false to indicate the user must now write out the
    // "definition" of the NULL pointer.  This is necessary because of
    // a quirk in the BamReader's design, which forces callers to read
    // the definition of every NULL pointer.  Presumably, the caller
    // will be able to write the definition in a concise way that will
    // clearly indicate a NULL pointer; in the case of a
    // PointerToArray, this will generally be simply a zero element
    // count.
    return false;
  }  

  PTAMap::iterator pi = _pta_map.find(ptr);
  if (pi == _pta_map.end()) {
    // We have not encountered this pointer before.
    int pta_id = _next_pta_id;
    _next_pta_id++;
    
    // Make sure our PTA ID will fit within the PN_uint16 we have
    // allocated for it.
    nassertr(pta_id <= 65535, 0);
    
    bool inserted = _pta_map.insert(PTAMap::value_type(ptr, pta_id)).second;
    nassertr(inserted, false);
    
    packet.add_uint16(pta_id);
    
    // Return false to indicate the caller must now write out the
    // array definition.
    return false;
    
  } else {
    // We have encountered this pointer before.
    int pta_id = (*pi).second;
    packet.add_uint16(pta_id);
    
    // Return true to indicate the caller need do nothing further.
    return true;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BamWriter::write_handle
//       Access: Public
//  Description: Writes a TypeHandle to the file in such a way that
//               the BamReader can read the same TypeHandle later via
//               read_handle().
////////////////////////////////////////////////////////////////////
void BamWriter::
write_handle(Datagram &packet, TypeHandle type) {
  // We encode TypeHandles within the Bam file by writing a unique
  // index number for each one to the file.  When we write a
  // particular TypeHandle for the first time, we assign it a new
  // index number and then immediately follow it by its definition;
  // when we write the same TypeHandle on subsequent times we only
  // write the index number.

  // The unique number we choose is actually the internal index number
  // of the TypeHandle.  Why not?
  int index = type.get_index();

  // Also make sure the index number fits within a PN_uint16.
  nassertv(index <= 65535);

  packet.add_uint16(index);

  if (index != 0) {
    bool inserted = _types_written.insert(index).second;

    if (inserted) {
      // This is the first time this TypeHandle has been written, so
      // also write out its definition.
      packet.add_string(type.get_name());
      
      // We also need to write the derivation of the TypeHandle, in case
      // the program reading this file later has never heard of this
      // type before.
      int num_parent_classes = type.get_num_parent_classes();
      nassertv(num_parent_classes <= 255);  // Good grief!
      packet.add_uint8(num_parent_classes);
      for (int i = 0; i < num_parent_classes; i++) {
	write_handle(packet, type.get_parent_class(i));
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: BamWriter::enqueue_object
//       Access: Private
//  Description: Assigns an object ID to the object and queues it up
//               for later writing to the Bam file.  
//
//               The return value is the object ID, or 0 if there is
//               an error.
////////////////////////////////////////////////////////////////////
int BamWriter::
enqueue_object(TypedWritable *object) {
  Datagram dg;

  nassertr(object != TypedWritable::Null, 0);

  // No object should ever be written out that is not registered as a
  // child of TypedWritable.  The only way this can happen is if
  // someone failed to initialize their type correctly in init_type().
  nassertr(object->is_of_type(TypedWritable::get_class_type()), 0);

  // We need to assign a unique index number to every object we write
  // out.  Has this object been assigned a number yet?
  int object_id;
  bool already_written;

  StateMap::iterator si = _state_map.find(object);
  if (si == _state_map.end()) {
    // No, it hasn't, so assign it the next number in sequence
    // arbitrarily.
    object_id = _next_object_id;
    already_written = false;
    
    // Make sure our object ID will fit within the PN_uint16 we have
    // allocated for it.
    nassertr(object_id <= 65535, 0);

    bool inserted = 
      _state_map.insert(StateMap::value_type(object, StoreState(_next_object_id))).second;
    nassertr(inserted, false);
    _next_object_id++;

  } else {
    // Yes, it has; get the object ID.
    object_id = (*si).second._object_id;
    already_written = (*si).second._written;
  }

  _object_queue.push_back(object);
  return object_id;
}
