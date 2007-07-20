// Filename: materialAttrib.h
// Created by:  drose (04Mar02)
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

#ifndef MATERIALATTRIB_H
#define MATERIALATTRIB_H

#include "pandabase.h"

#include "renderAttrib.h"
#include "material.h"

////////////////////////////////////////////////////////////////////
//       Class : MaterialAttrib
// Description : Indicates which, if any, material should be applied
//               to geometry.  The material is used primarily to
//               control lighting effects, and isn't necessary (or
//               useful) in the absence of lighting.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA_PGRAPH MaterialAttrib : public RenderAttrib {
private:
  INLINE MaterialAttrib();

PUBLISHED:
  static CPT(RenderAttrib) make(Material *material);
  static CPT(RenderAttrib) make_off();

  INLINE bool is_off() const;
  INLINE Material *get_material() const;

public:
  virtual void output(ostream &out) const;
  virtual void store_into_slot(AttribSlots *slots) const;

protected:
  virtual int compare_to_impl(const RenderAttrib *other) const;
  virtual RenderAttrib *make_default_impl() const;

private:
  PT(Material) _material;

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
    register_type(_type_handle, "MaterialAttrib",
                  RenderAttrib::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

#include "materialAttrib.I"

#endif

