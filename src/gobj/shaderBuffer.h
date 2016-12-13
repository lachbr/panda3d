/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file shaderBuffer.h
 * @author rdb
 * @date 2016-12-12
 */

#ifndef SHADERBUFFER_H
#define SHADERBUFFER_H

#include "pandabase.h"
#include "namable.h"
#include "geomEnums.h"

class BufferContext;
class PreparedGraphicsObjects;

/**
 * This is a generic buffer object that lives in graphics memory.
 */
class EXPCL_PANDA_GOBJ ShaderBuffer : public TypedWritableReferenceCount, public Namable, public GeomEnums {
private:
  INLINE ShaderBuffer() DEFAULT_CTOR;

PUBLISHED:
  INLINE ShaderBuffer(const string &name, uint64_t size, UsageHint usage_hint);
  INLINE ShaderBuffer(const string &name, pvector<unsigned char> initial_data, UsageHint usage_hint);

public:
  INLINE uint64_t get_data_size_bytes() const;
  INLINE UsageHint get_usage_hint() const;
  INLINE const unsigned char *get_initial_data() const;

  virtual void output(ostream &out) const;

PUBLISHED:
  MAKE_PROPERTY(data_size_bytes, get_data_size_bytes);
  MAKE_PROPERTY(usage_hint, get_usage_hint);

  void prepare(PreparedGraphicsObjects *prepared_objects);
  bool is_prepared(PreparedGraphicsObjects *prepared_objects) const;

  BufferContext *prepare_now(PreparedGraphicsObjects *prepared_objects,
                             GraphicsStateGuardianBase *gsg);
  bool release(PreparedGraphicsObjects *prepared_objects);
  int release_all();

private:
  uint64_t _data_size_bytes;
  UsageHint _usage_hint;
  pvector<unsigned char> _initial_data;

  typedef pmap<PreparedGraphicsObjects *, BufferContext *> Contexts;
  Contexts *_contexts;

public:
  static void register_with_read_factory();
  virtual void write_datagram(BamWriter *manager, Datagram &dg);

protected:
  static TypedWritable *make_from_bam(const FactoryParams &params);
  void fillin(DatagramIterator &scan, BamReader *manager);

public:
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    TypedWritableReferenceCount::init_type();
    Namable::init_type();
    register_type(_type_handle, "ShaderBuffer",
                  TypedWritableReferenceCount::get_class_type(),
                  Namable::get_class_type());
  }

private:
  static TypeHandle _type_handle;
};

INLINE ostream &operator << (ostream &out, const ShaderBuffer &m) {
  m.output(out);
  return out;
}

#include "shaderBuffer.I"

#endif
