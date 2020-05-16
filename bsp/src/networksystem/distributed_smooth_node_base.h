/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file distributed_smooth_node_base.h
 * @author Brian Lach
 * @date March 27, 2018
 *
 * @desc Modified version of cDistributedSmoothNodeBase from direct/src/distributed
 *       for use in the new network system. Original file authored by drose on
 *       September 03, 2004.
 */

#ifndef DISTRIBUTED_SMOOTH_NODE_BASE_H
#define DISTRIBUTED_SMOOTH_NODE_BASE_H

#include "directbase.h"
#include "nodePath.h"
#include "dcbase.h"
#include "dcPacker.h"
#include "clockObject.h"
#include "dcClass.h"

class NetworkSystem;

/**
 * This class defines some basic methods of DistributedSmoothNodeBase which
 * have been moved into C++ as a performance optimization.
 */
class CDistributedSmoothNodeBase {
PUBLISHED:
  CDistributedSmoothNodeBase();
  ~CDistributedSmoothNodeBase();

  INLINE void
    set_repository(NetworkSystem *repository,
                   bool is_ai, CHANNEL_TYPE ai_id);

#ifdef HAVE_PYTHON
  INLINE void
    set_clock_delta(PyObject *clock_delta);
#endif

  void initialize(const NodePath &node_path, DCClass *dclass,
                  CHANNEL_TYPE do_id);

  void send_everything();

  void broadcast_pos_hpr_full();
  void broadcast_pos_hpr_xyh();
  void broadcast_pos_hpr_xy();

  void set_curr_l(uint64_t l);
  void print_curr_l();

private:
  INLINE static bool only_changed(int flags, int compare);

  INLINE void d_setSmStop();
  INLINE void d_setSmH(PN_stdfloat h);
  INLINE void d_setSmZ(PN_stdfloat z);
  INLINE void d_setSmXY(PN_stdfloat x, PN_stdfloat y);
  INLINE void d_setSmXZ(PN_stdfloat x, PN_stdfloat z);
  INLINE void d_setSmPos(PN_stdfloat x, PN_stdfloat y, PN_stdfloat z);
  INLINE void d_setSmHpr(PN_stdfloat h, PN_stdfloat p, PN_stdfloat r);
  INLINE void d_setSmXYH(PN_stdfloat x, PN_stdfloat y, PN_stdfloat h);
  INLINE void d_setSmXYZH(PN_stdfloat x, PN_stdfloat y, PN_stdfloat z, PN_stdfloat h);
  INLINE void d_setSmPosHpr(PN_stdfloat x, PN_stdfloat y, PN_stdfloat z, PN_stdfloat h, PN_stdfloat p, PN_stdfloat r);
  INLINE void d_setSmPosHprL(PN_stdfloat x, PN_stdfloat y, PN_stdfloat z, PN_stdfloat h, PN_stdfloat p, PN_stdfloat r, uint64_t l);

  void begin_send_update(DCPacker &packer, const std::string &field_name);
  void finish_send_update(DCPacker &packer);

  enum Flags {
    F_new_x = 0x01,
    F_new_y = 0x02,
    F_new_z = 0x04,
    F_new_h = 0x08,
    F_new_p = 0x10,
    F_new_r = 0x20,
  };

  NodePath _node_path;
  DCClass *_dclass;
  CHANNEL_TYPE _do_id;

  NetworkSystem *_repository;
  bool _is_ai;
  CHANNEL_TYPE _ai_id;
#ifdef HAVE_PYTHON
  PyObject *_clock_delta;
#endif

  LPoint3 _store_xyz;
  LVecBase3 _store_hpr;
  bool _store_stop;
  // contains most recently sent location info as index 0, index 1 contains
  // most recently set location info
  uint64_t _currL[2];
};

/**
 * Tells the C++ instance definition about the AI or Client repository, used
 * for sending datagrams.
 */
INLINE void CDistributedSmoothNodeBase::
set_repository(NetworkSystem *repository,
               bool is_ai, CHANNEL_TYPE ai_id) {
  _repository = repository;
  _is_ai = is_ai;
  _ai_id = ai_id;
}

#ifdef HAVE_PYTHON
/**
 * Tells the C++ instance definition about the global ClockDelta object.
 */
INLINE void CDistributedSmoothNodeBase::
set_clock_delta(PyObject *clock_delta) {
  _clock_delta = clock_delta;
}
#endif  // HAVE_PYTHON

/**
 * Returns true if at least some of the bits of compare are set in flags, but
 * no bits outside of compare are set.  That is to say, that the only things
 * that are changed are the bits indicated in compare.
 */
INLINE bool CDistributedSmoothNodeBase::
only_changed(int flags, int compare) {
  return (flags & compare) != 0 && (flags & ~compare) == 0;
}

/**
 *
 */
INLINE void CDistributedSmoothNodeBase::
d_setSmStop() {
  DCPacker packer;
  begin_send_update(packer, "setSmStop");
  finish_send_update(packer);
}

/**
 *
 */
INLINE void CDistributedSmoothNodeBase::
d_setSmH(PN_stdfloat h) {
  DCPacker packer;
  begin_send_update(packer, "setSmH");
  packer.pack_double(h);
  finish_send_update(packer);
}

/**
 *
 */
INLINE void CDistributedSmoothNodeBase::
d_setSmZ(PN_stdfloat z) {
  DCPacker packer;
  begin_send_update(packer, "setSmZ");
  packer.pack_double(z);
  finish_send_update(packer);
}

/**
 *
 */
INLINE void CDistributedSmoothNodeBase::
d_setSmXY(PN_stdfloat x, PN_stdfloat y) {
  DCPacker packer;
  begin_send_update(packer, "setSmXY");
  packer.pack_double(x);
  packer.pack_double(y);
  finish_send_update(packer);
}

/**
 *
 */
INLINE void CDistributedSmoothNodeBase::
d_setSmXZ(PN_stdfloat x, PN_stdfloat z) {
  DCPacker packer;
  begin_send_update(packer, "setSmXZ");
  packer.pack_double(x);
  packer.pack_double(z);
  finish_send_update(packer);
}

/**
 *
 */
INLINE void CDistributedSmoothNodeBase::
d_setSmPos(PN_stdfloat x, PN_stdfloat y, PN_stdfloat z) {
  DCPacker packer;
  begin_send_update(packer, "setSmPos");
  packer.pack_double(x);
  packer.pack_double(y);
  packer.pack_double(z);
  finish_send_update(packer);
}

/**
 *
 */
INLINE void CDistributedSmoothNodeBase::
d_setSmHpr(PN_stdfloat h, PN_stdfloat p, PN_stdfloat r) {
  DCPacker packer;
  begin_send_update(packer, "setSmHpr");
  packer.pack_double(h);
  packer.pack_double(p);
  packer.pack_double(r);
  finish_send_update(packer);
}

/**
 *
 */
INLINE void CDistributedSmoothNodeBase::
d_setSmXYH(PN_stdfloat x, PN_stdfloat y, PN_stdfloat h) {
  DCPacker packer;
  begin_send_update(packer, "setSmXYH");
  packer.pack_double(x);
  packer.pack_double(y);
  packer.pack_double(h);
  finish_send_update(packer);
}

/**
 *
 */
INLINE void CDistributedSmoothNodeBase::
d_setSmXYZH(PN_stdfloat x, PN_stdfloat y, PN_stdfloat z, PN_stdfloat h) {
  DCPacker packer;
  begin_send_update(packer, "setSmXYZH");
  packer.pack_double(x);
  packer.pack_double(y);
  packer.pack_double(z);
  packer.pack_double(h);
  finish_send_update(packer);
}

/**
 *
 */
INLINE void CDistributedSmoothNodeBase::
d_setSmPosHpr(PN_stdfloat x, PN_stdfloat y, PN_stdfloat z, PN_stdfloat h, PN_stdfloat p, PN_stdfloat r) {
  DCPacker packer;
  begin_send_update(packer, "setSmPosHpr");
  packer.pack_double(x);
  packer.pack_double(y);
  packer.pack_double(z);
  packer.pack_double(h);
  packer.pack_double(p);
  packer.pack_double(r);
  finish_send_update(packer);
}

/**
 *
 */
INLINE void CDistributedSmoothNodeBase::
d_setSmPosHprL(PN_stdfloat x, PN_stdfloat y, PN_stdfloat z, PN_stdfloat h, PN_stdfloat p, PN_stdfloat r, uint64_t l) {
  DCPacker packer;
  begin_send_update(packer, "setSmPosHprL");
  packer.pack_uint64(_currL[0]);
  packer.pack_double(x);
  packer.pack_double(y);
  packer.pack_double(z);
  packer.pack_double(h);
  packer.pack_double(p);
  packer.pack_double(r);
  finish_send_update(packer);
}


#endif  // DISTRIBUTED_SMOOTH_NODE_BASE_H
