// Filename: eggVertex.h
// Created by:  drose (16Jan99)
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

#ifndef EGGVERTEX_H
#define EGGVERTEX_H

#include <pandabase.h>

#include "eggObject.h"
#include "eggAttributes.h"
#include "eggMorphList.h"

#include <referenceCount.h>
#include <luse.h>
#include "pset.h"

class EggVertexPool;
class EggGroup;
class EggPrimitive;


////////////////////////////////////////////////////////////////////
//       Class : EggVertex
// Description : Any one-, two-, three-, or four-component vertex,
//               possibly with attributes such as a normal.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAEGG EggVertex : public EggObject, public EggAttributes {
public:
  typedef pset<EggGroup *> GroupRef;
  typedef pmultiset<EggPrimitive *> PrimitiveRef;

  EggVertex();
  EggVertex(const EggVertex &copy);
  EggVertex &operator = (const EggVertex &copy);
  virtual ~EggVertex();

  INLINE EggVertexPool *get_pool() const;

  // The pos might have 1, 2, 3, or 4 dimensions.  That complicates
  // things a bit.
  INLINE void set_pos(double pos);
  INLINE void set_pos(const LPoint2d &pos);
  INLINE void set_pos(const LPoint3d &pos);
  INLINE void set_pos(const LPoint4d &pos);
  INLINE void set_pos4(const LPoint4d &pos);

  // get_pos[123] return the pos as the corresponding type.  It is an
  // error to call any of these without first verifying that
  // get_num_dimensions() matches the desired type.  However,
  // get_pos4() may always be called; it returns the pos as a
  // four-component point in homogeneous space (with a 1.0 in the last
  // position if the pos has fewer than four components).
  INLINE int get_num_dimensions() const;
  INLINE double get_pos1() const;
  INLINE LPoint2d get_pos2() const;
  INLINE Vertexd get_pos3() const;
  INLINE LPoint4d get_pos4() const;

  INLINE int get_index() const;

  void write(ostream &out, int indent_level) const;
  bool sorts_less_than(const EggVertex &other) const;

  int get_num_local_coord() const;
  int get_num_global_coord() const;

  void transform(const LMatrix4d &mat);

  INLINE GroupRef::const_iterator gref_begin() const;
  INLINE GroupRef::const_iterator gref_end() const;
  INLINE GroupRef::size_type gref_size() const;
  bool has_gref(const EggGroup *group) const;

  void copy_grefs_from(const EggVertex &other);
  void clear_grefs();

  INLINE PrimitiveRef::const_iterator pref_begin() const;
  INLINE PrimitiveRef::const_iterator pref_end() const;
  INLINE PrimitiveRef::size_type pref_size() const;
  int has_pref(const EggPrimitive *prim) const;

#ifndef NDEBUG
  void test_gref_integrity() const;
  void test_pref_integrity() const;
#else
  void test_gref_integrity() const { }
  void test_pref_integrity() const { }
#endif  // NDEBUG

  void output(ostream &out) const;

  EggMorphVertexList _dxyzs;

private:
  EggVertexPool *_pool;
  int _index;
  LPoint4d _pos;
  short _num_dimensions;
  GroupRef _gref;
  PrimitiveRef _pref;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    EggObject::init_type();
    EggAttributes::init_type();
    register_type(_type_handle, "EggVertex",
                  EggObject::get_class_type(),
                  EggAttributes::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;

  friend class EggVertexPool;
  friend class EggGroup;
  friend class EggPrimitive;
};

INLINE ostream &operator << (ostream &out, const EggVertex &vert) {
  vert.output(out);
  return out;
}

///////////////////////////////////////////////////////////////////
//       Class : UniqueEggVertices
// Description : An STL function object for sorting vertices into
//               order by properties.  Returns true if the two
//               referenced EggVertex pointers are in sorted order,
//               false otherwise.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAEGG UniqueEggVertices {
public:
  INLINE bool operator ()(const EggVertex *v1, const EggVertex *v2) const;
};

#include "eggVertex.I"

#endif



