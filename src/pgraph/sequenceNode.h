// Filename: sequenceNode.h
// Created by:  drose (06Mar02)
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

#ifndef SEQUENCENODE_H
#define SEQUENCENODE_H

#include "pandabase.h"

#include "selectiveChildNode.h"
#include "clockObject.h"

////////////////////////////////////////////////////////////////////
//       Class : SequenceNode
// Description : A node that automatically cycles through rendering
//               each one of its children according to its frame rate.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA SequenceNode : public SelectiveChildNode {
PUBLISHED:
  INLINE SequenceNode(float cycle_rate, const string &name);

public:
  SequenceNode(const SequenceNode &copy);

  virtual PandaNode *make_copy() const;
  virtual bool safe_to_combine() const;

  virtual bool has_cull_callback() const;
  virtual bool cull_callback(CullTraverser *trav, CullTraverserData &data);

PUBLISHED:
  INLINE void set_cycle_rate(float cycle_rate);
  INLINE float get_cycle_rate() const;

  INLINE void set_visible_child(int index);
  INLINE int get_visible_child() const;

private:
  INLINE float calc_frame(float now) const;
  INLINE float calc_frame() const;

  class EXPCL_PANDA CData : public CycleData {
  public:
    INLINE CData();
    INLINE CData(const CData &copy);
    virtual CycleData *make_copy() const;
    virtual void write_datagram(BamWriter *manager, Datagram &dg) const;
    virtual void fillin(DatagramIterator &scan, BamReader *manager);

    float _cycle_rate;
    float _frame_offset;
    float _start_time;
  };

  PipelineCycler<CData> _cycler;
  typedef CycleDataReader<CData> CDReader;
  typedef CycleDataWriter<CData> CDWriter;

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
    SelectiveChildNode::init_type();
    register_type(_type_handle, "SequenceNode",
                  SelectiveChildNode::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

#include "sequenceNode.I"

#endif
