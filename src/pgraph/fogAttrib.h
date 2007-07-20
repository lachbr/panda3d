// Filename: fogAttrib.h
// Created by:  drose (14Mar02)
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

#ifndef FOGATTRIB_H
#define FOGATTRIB_H

#include "pandabase.h"

#include "renderAttrib.h"
#include "fog.h"

////////////////////////////////////////////////////////////////////
//       Class : FogAttrib
// Description : Applies a Fog to the geometry at and below this node.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA_PGRAPH FogAttrib : public RenderAttrib {
private:
  INLINE FogAttrib();

PUBLISHED:
  static CPT(RenderAttrib) make(Fog *fog);
  static CPT(RenderAttrib) make_off();

  INLINE bool is_off() const;
  INLINE Fog *get_fog() const;

public:
  virtual void output(ostream &out) const;
  virtual void store_into_slot(AttribSlots *slots) const;

protected:
  virtual int compare_to_impl(const RenderAttrib *other) const;
  virtual RenderAttrib *make_default_impl() const;

private:
  PT(Fog) _fog;

public:
  static void register_with_read_factory();
  virtual void write_datagram(BamWriter *manager, Datagram &dg);
  virtual int complete_pointers(TypedWritable **plist, BamReader *manager);

protected:
  static TypedWritable *make_from_bam(const FactoryParams &params);
  void fillin(DatagramIterator &scan, BamReader *manager);
  
public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    RenderAttrib::init_type();
    register_type(_type_handle, "FogAttrib",
                  RenderAttrib::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

#include "fogAttrib.I"

#endif

