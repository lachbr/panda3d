// Filename: cullFaceAttrib.h
// Created by:  drose (27Feb02)
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

#ifndef CULLFACEATTRIB_H
#define CULLFACEATTRIB_H

#include "pandabase.h"

#include "renderAttrib.h"

class FactoryParams;

////////////////////////////////////////////////////////////////////
//       Class : CullFaceAttrib
// Description : Indicates which faces should be culled based on their
//               vertex ordering.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA_PGRAPH CullFaceAttrib : public RenderAttrib {
PUBLISHED:
  enum Mode {
    M_cull_none,                // Cull no polygons
    M_cull_clockwise,           // Cull clockwise-oriented polygons
    M_cull_counter_clockwise,   // Cull counter-clockwise-oriented polygons
    M_cull_unchanged,           // Do not change existing cull behavior
  };

private:
  INLINE CullFaceAttrib(Mode mode, bool reverse);

PUBLISHED:
  static CPT(RenderAttrib) make(Mode mode = M_cull_clockwise);
  static CPT(RenderAttrib) make_reverse();

  INLINE Mode get_actual_mode() const;
  INLINE bool get_reverse() const;
  Mode get_effective_mode() const;

public:
  virtual void output(ostream &out) const;
  virtual void store_into_slot(AttribSlots *slots) const;

protected:
  virtual int compare_to_impl(const RenderAttrib *other) const;
  virtual CPT(RenderAttrib) compose_impl(const RenderAttrib *other) const;
  virtual CPT(RenderAttrib) invert_compose_impl(const RenderAttrib *other) const;
  virtual RenderAttrib *make_default_impl() const;

private:
  Mode _mode;
  bool _reverse;

public:
  static void register_with_read_factory();
  virtual void write_datagram(BamWriter *manager, Datagram &dg);

protected:
  static TypedWritable *make_from_bam(const FactoryParams &params);
  void fillin(DatagramIterator &scan, BamReader *manager);
  
public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    RenderAttrib::init_type();
    register_type(_type_handle, "CullFaceAttrib",
                  RenderAttrib::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

#include "cullFaceAttrib.I"

#endif

