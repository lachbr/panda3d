// Filename: geomVertexArrayData.h
// Created by:  drose (17Mar05)
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

#ifndef GEOMVERTEXARRAYDATA_H
#define GEOMVERTEXARRAYDATA_H

#include "pandabase.h"
#include "typedWritableReferenceCount.h"
#include "geomVertexArrayFormat.h"
#include "geomEnums.h"
#include "pta_uchar.h"
#include "updateSeq.h"
#include "cycleData.h"
#include "cycleDataReader.h"
#include "cycleDataWriter.h"
#include "cycleDataStageReader.h"
#include "cycleDataStageWriter.h"
#include "pipelineCycler.h"
#include "pmap.h"

class PreparedGraphicsObjects;
class VertexBufferContext;
class GraphicsStateGuardianBase;

////////////////////////////////////////////////////////////////////
//       Class : GeomVertexArrayData
// Description : This is the data for one array of a GeomVertexData
//               structure.  Many GeomVertexData structures will only
//               define one array, with all data elements interleaved
//               (DirectX 8.0 and before insisted on this format);
//               some will define multiple arrays.  
//
//               DirectX calls this concept of one array a "stream".
//               It also closely correlates with the concept of a
//               vertex buffer.
//
//               This object is just a block of data.  In general, you
//               should not be directly messing with this object from
//               application code.  See GeomVertexData for the
//               organizing structure, and see
//               GeomVertexReader/Writer/Rewriter for high-level tools
//               to manipulate the actual vertex data.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA GeomVertexArrayData : public TypedWritableReferenceCount, public GeomEnums {
private:
  GeomVertexArrayData();

PUBLISHED:
  GeomVertexArrayData(const GeomVertexArrayFormat *array_format,
                      UsageHint usage_hint);
  GeomVertexArrayData(const GeomVertexArrayData &copy);
  void operator = (const GeomVertexArrayData &copy);
  virtual ~GeomVertexArrayData();
  ALLOC_DELETED_CHAIN(GeomVertexArrayData);

  INLINE const GeomVertexArrayFormat *get_array_format() const;

  INLINE UsageHint get_usage_hint() const;
  void set_usage_hint(UsageHint usage_hint);

  INLINE bool has_column(const InternalName *name) const;

  INLINE int get_num_rows() const;
  INLINE bool set_num_rows(int n);
  INLINE void clear_rows();

  INLINE int get_data_size_bytes() const;
  INLINE UpdateSeq get_modified() const;

  void output(ostream &out) const;
  void write(ostream &out, int indent_level = 0) const;

  INLINE CPTA_uchar get_data() const;
  INLINE PTA_uchar modify_data();
  INLINE void set_data(CPTA_uchar data);

public:
  void prepare(PreparedGraphicsObjects *prepared_objects);

  VertexBufferContext *prepare_now(PreparedGraphicsObjects *prepared_objects, 
                                   GraphicsStateGuardianBase *gsg);
  bool release(PreparedGraphicsObjects *prepared_objects);
  int release_all();

private:
  void clear_prepared(PreparedGraphicsObjects *prepared_objects);
  PTA_uchar reverse_data_endianness(const PTA_uchar &data);

  CPT(GeomVertexArrayFormat) _array_format;

  // A GeomVertexArrayData keeps a list (actually, a map) of all the
  // PreparedGraphicsObjects tables that it has been prepared into.
  // Each PGO conversely keeps a list (a set) of all the Geoms that
  // have been prepared there.  When either destructs, it removes
  // itself from the other's list.
  typedef pmap<PreparedGraphicsObjects *, VertexBufferContext *> Contexts;
  Contexts _contexts;

  // This is only used when reading from a bam file.  It is set true
  // to indicate the data must be endian-reversed in finalize().
  bool _endian_reversed;

  // This is the data that must be cycled between pipeline stages.
  class EXPCL_PANDA CData : public CycleData {
  public:
    INLINE CData();
    INLINE CData(const CData &copy);
    virtual ~CData();
    ALLOC_DELETED_CHAIN(CData);
    virtual CycleData *make_copy() const;
    virtual void write_datagram(BamWriter *manager, Datagram &dg,
                                void *extra_data) const;
    virtual void fillin(DatagramIterator &scan, BamReader *manager,
                        void *extra_data);
    virtual TypeHandle get_parent_type() const {
      return GeomVertexArrayData::get_class_type();
    }

    UsageHint _usage_hint;
    PTA_uchar _data;
    UpdateSeq _modified;

    friend class GeomVertexArrayData;
  };

  PipelineCycler<CData> _cycler;
  typedef CycleDataReader<CData> CDReader;
  typedef CycleDataWriter<CData> CDWriter;
  typedef CycleDataStageReader<CData> CDStageReader;
  typedef CycleDataStageWriter<CData> CDStageWriter;

public:
  static void register_with_read_factory();
  virtual void write_datagram(BamWriter *manager, Datagram &dg);
  void write_raw_data(BamWriter *manager, Datagram &dg, const PTA_uchar &data);
  PTA_uchar read_raw_data(BamReader *manager, DatagramIterator &source);
  virtual int complete_pointers(TypedWritable **plist, BamReader *manager);

  virtual void finalize(BamReader *manager);

protected:
  static TypedWritable *make_from_bam(const FactoryParams &params);
  void fillin(DatagramIterator &scan, BamReader *manager);

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    TypedWritableReferenceCount::init_type();
    register_type(_type_handle, "GeomVertexArrayData",
                  TypedWritableReferenceCount::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;

  friend class GeomCacheManager;
  friend class GeomVertexData;
  friend class PreparedGraphicsObjects;
  friend class GeomVertexArrayDataPipelineBase;
  friend class GeomVertexArrayDataPipelineReader;
  friend class GeomVertexArrayDataPipelineWriter;
};

////////////////////////////////////////////////////////////////////
//       Class : GeomVertexArrayDataPipelineBase
// Description : The common code from
//               GeomVertexArrayDataPipelineReader and
//               GeomVertexArrayDataPipelineWriter.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA GeomVertexArrayDataPipelineBase : public GeomEnums {
protected:
  INLINE GeomVertexArrayDataPipelineBase(GeomVertexArrayData *object, 
                                         Thread *current_thread,
                                         GeomVertexArrayData::CData *cdata);
public:
  INLINE ~GeomVertexArrayDataPipelineBase();

  INLINE Thread *get_current_thread() const;

  INLINE const GeomVertexArrayFormat *get_array_format() const;

  INLINE UsageHint get_usage_hint() const;
  INLINE CPTA_uchar get_data() const;
  INLINE int get_num_rows() const;
  INLINE int get_data_size_bytes() const;
  INLINE UpdateSeq get_modified() const;

protected:
  GeomVertexArrayData *_object;
  Thread *_current_thread;
  GeomVertexArrayData::CData *_cdata;
};

////////////////////////////////////////////////////////////////////
//       Class : GeomVertexArrayDataPipelineReader
// Description : Encapsulates the data from a GeomVertexArrayData,
//               pre-fetched for one stage of the pipeline.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA GeomVertexArrayDataPipelineReader : public GeomVertexArrayDataPipelineBase {
public:
  INLINE GeomVertexArrayDataPipelineReader(const GeomVertexArrayData *object, Thread *current_thread);
private:
  INLINE GeomVertexArrayDataPipelineReader(const GeomVertexArrayDataPipelineReader &copy);
  INLINE void operator = (const GeomVertexArrayDataPipelineReader &copy);

public:
  INLINE ~GeomVertexArrayDataPipelineReader();
  ALLOC_DELETED_CHAIN(GeomVertexArrayDataPipelineReader);

  INLINE const GeomVertexArrayData *get_object() const;
};

////////////////////////////////////////////////////////////////////
//       Class : GeomVertexArrayDataPipelineWriter
// Description : Encapsulates the data from a GeomVertexArrayData,
//               pre-fetched for one stage of the pipeline.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA GeomVertexArrayDataPipelineWriter : public GeomVertexArrayDataPipelineBase {
public:
  INLINE GeomVertexArrayDataPipelineWriter(GeomVertexArrayData *object, bool force_to_0, Thread *current_thread);
private:
  INLINE GeomVertexArrayDataPipelineWriter(const GeomVertexArrayDataPipelineWriter &copy);
  INLINE void operator = (const GeomVertexArrayDataPipelineWriter &copy);

public:
  INLINE ~GeomVertexArrayDataPipelineWriter();
  ALLOC_DELETED_CHAIN(GeomVertexArrayDataPipelineWriter);

  bool set_num_rows(int n);

  INLINE GeomVertexArrayData *get_object() const;
  PTA_uchar modify_data();
  void set_data(CPTA_uchar data);
};

INLINE ostream &operator << (ostream &out, const GeomVertexArrayData &obj);

#include "geomVertexArrayData.I"

#endif
