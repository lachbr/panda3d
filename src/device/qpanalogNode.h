// Filename: qpanalogNode.h
// Created by:  drose (12Mar02)
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

#ifndef qpANALOGNODE_H
#define qpANALOGNODE_H

#include "pandabase.h"

#include "clientBase.h"
#include "clientAnalogDevice.h"
#include "qpdataNode.h"
#include "linmath_events.h"


////////////////////////////////////////////////////////////////////
//       Class : qpAnalogNode
// Description : This is the primary interface to analog controls like
//               sliders and joysticks associated with a ClientBase.
//               This creates a node that connects to the named analog
//               device, if it exists, and provides hooks to the user
//               to read the state of any of the sequentially numbered
//               controls associated with that device.
//
//               Each control can return a value ranging from -1 to 1,
//               reflecting the current position of the control within
//               its total range of motion.
//
//               The user may choose up to two analog controls to
//               place on the data graph as the two channels of an
//               xy datagram, similarly to the way a mouse places its
//               position data.  In this way, an AnalogNode may be
//               used in place of a mouse.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA qpAnalogNode : public qpDataNode {
PUBLISHED:
  qpAnalogNode(ClientBase *client, const string &device_name);
  virtual ~qpAnalogNode();

  INLINE bool is_valid() const;

  INLINE int get_num_controls() const;

  INLINE double get_control_state(int index) const;
  INLINE bool is_control_known(int index) const;

  INLINE void set_output(int channel, int index, bool flip);
  INLINE void clear_output(int channel);
  INLINE int get_output(int channel) const;
  INLINE bool is_output_flipped(int channel) const;

public:
  virtual void write(ostream &out, int indent_level = 0) const;

private:
  class OutputData {
  public:
    INLINE OutputData();
    int _index;
    bool _flip;
  };

  enum { max_outputs = 2 };
  OutputData _outputs[max_outputs];

  PT(ClientAnalogDevice) _analog;

protected:
  // Inherited from DataNode
  virtual void do_transmit_data(const DataNodeTransmit &input,
                                DataNodeTransmit &output);

private:
  // outputs
  int _xy_output;

  PT(EventStoreVec2) _xy;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    qpDataNode::init_type();
    register_type(_type_handle, "qpAnalogNode",
                  qpDataNode::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

#include "qpanalogNode.I"

#endif
