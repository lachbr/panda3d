// Filename: config_express.cxx
// Created by:  cary (04Jan00)
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


#include "config_express.h"
#include "datagram.h"
#include "referenceCount.h"
#include "typedObject.h"
#include "typedReferenceCount.h"
#include "virtualFile.h"
#include "virtualFileComposite.h"
#include "virtualFileMount.h"
#include "virtualFileMountMultifile.h"
#include "virtualFileMountSystem.h"
#include "virtualFileSimple.h"

#include <dconfig.h>

ConfigureDef(config_express);
NotifyCategoryDef(express, "");

extern void init_system_type_handles();

ConfigureFn(config_express) {
  Datagram::init_type();
  ReferenceCount::init_type();
  TypedObject::init_type();
  TypedReferenceCount::init_type();
  VirtualFileComposite::init_type();
  VirtualFile::init_type();
  VirtualFileMount::init_type();
  VirtualFileMountMultifile::init_type();
  VirtualFileMountSystem::init_type();
  VirtualFileSimple::init_type();

  init_system_type_handles();
}


// Set leak-memory true to disable the actual deletion of
// ReferenceCount-derived objects.  This is sometimes useful to track
// a reference counting bug, since the formerly deleted objects will
// still remain (with a reference count of -100) without being
// overwritten with a newly-allocated object, and the assertion tests
// in ReferenceCount may more accurately detect the first instance of
// an error.
bool
get_leak_memory() {
  static bool got_leak_memory = false;
  static bool leak_memory;

  if (!got_leak_memory) {
    leak_memory = config_express.GetBool("leak-memory", false);
    got_leak_memory = true;
  }

  return leak_memory;
}

// never-destruct is similar to leak-memory, above, except that not
// only will memory not be freed, but the destructor will not even be
// called (on ReferenceCount objects, at least).  This will leak gobs
// of memory, but ensures that every pointer to a ReferenceCount
// object will always be valid, and may be useful for tracking down
// certain kinds of errors.

// never-destruct is only respected if leak-memory, above, is true.
bool
get_never_destruct() {
  static bool got_never_destruct = false;
  static bool never_destruct;

  if (!got_never_destruct) {
    never_destruct = config_express.GetBool("never-destruct", false);
    got_never_destruct = true;
  }

  return never_destruct;
}

//const bool track_memory_usage = config_express.GetBool("track-memory-usage", false);

// Set this to false to avoid using the high-precision clock, even if
// it is available.
bool
get_use_high_res_clock() {
  return config_express.GetBool("use-high-res-clock", true);
}

// Set this to true to double-check the results of the high-resolution
// clock against the system clock.  This has no effect if NDEBUG is
// defined.
bool
get_paranoid_clock() {
  return config_express.GetBool("paranoid-clock", false);
}

// Set this to true to double-check the test for inheritance of
// TypeHandles, e.g. via is_of_type().  This has no effect if NDEBUG
// is defined.
bool
get_paranoid_inheritance() {
  return config_express.GetBool("paranoid-inheritance", true);
}

// Set this to true to verify that every attempted DCAST operation in
// fact references the correct type, or false otherwise.  This has no
// effect if NDEBUG is defined, in which case it is never tested.
bool
get_verify_dcast() {
  static bool got_verify_dcast = false;
  static bool verify_dcast;

  if (!got_verify_dcast) {
    verify_dcast = config_express.GetBool("verify-dcast", true);
    got_verify_dcast = true;
  }

  return verify_dcast;
}

const int patchfile_window_size =
        config_express.GetInt("patchfile-window-size", 16);

const int patchfile_increment_size =
        config_express.GetInt("patchfile-increment-size", 8);

const int patchfile_buffer_size =
        config_express.GetInt("patchfile-buffer-size", 4096);

const int patchfile_zone_size =
        config_express.GetInt("patchfile-zone-size", 10000);


// Returns the configure object for accessing config variables from a
// scripting language.
ConfigExpress &
get_config_express() {
  return config_express;
}

