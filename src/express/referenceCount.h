// Filename: referenceCount.h
// Created by:  drose (23Oct98)
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

#ifndef REFERENCECOUNT_H
#define REFERENCECOUNT_H

#include <pandabase.h>

#include "typedObject.h"
#include "memoryUsage.h"
#include "config_express.h"

#include <stdlib.h>

#ifdef HAVE_RTTI
#include <typeinfo>
#endif

///////////////////////////////////////////////////////////////////
//       Class : ReferenceCount
// Description : A base class for all things that want to be
//               reference-counted.  ReferenceCount works in
//               conjunction with PointerTo to automatically delete
//               objects when the last pointer to them goes away.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAEXPRESS ReferenceCount {
protected:
  INLINE ReferenceCount();
  INLINE ReferenceCount(const ReferenceCount &);
  INLINE void operator = (const ReferenceCount &);
  INLINE ~ReferenceCount();

public:
  // These functions are not part of the normal API, but they have to
  // be public.  You shouldn't generally call these directly.
  INLINE void prepare_delete();
  INLINE bool unref_consider_delete();

PUBLISHED:
  INLINE int get_ref_count() const;
  INLINE void ref() const;
  INLINE void unref() const;

  INLINE void test_ref_count_integrity() const;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    register_type(_type_handle, "ReferenceCount");
  }

private:
  int _ref_count;
  static TypeHandle _type_handle;
};

template<class RefCountType>
INLINE void unref_delete(RefCountType *ptr);

///////////////////////////////////////////////////////////////////
//       Class : RefCountProxy
// Description : A "proxy" to use to make a reference-countable object
//               whenever the object cannot inherit from
//               ReferenceCount for some reason.  RefCountPr<MyClass>
//               can be treated as an instance of MyClass directly,
//               for the most part, except that it can be reference
//               counted.
//
//               If you want to declare a RefCountProxy to something
//               that does not have get_class_type(), you will have to
//               define a template specialization on
//               _get_type_handle() and _do_init_type(), as in
//               typedObject.h.
////////////////////////////////////////////////////////////////////
template<class Base>
class RefCountProxy : public ReferenceCount {
public:
  INLINE RefCountProxy();
  INLINE RefCountProxy(const Base &copy);

  INLINE operator Base &();
  INLINE operator const Base &() const;

  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type();

private:
  Base _base;
  static TypeHandle _type_handle;
};


///////////////////////////////////////////////////////////////////
//       Class : RefCountObj
// Description : Another kind of proxy, similar to RefCountProxy.
//               This one works by inheriting from the indicated base
//               type, giving it an is-a relation instead of a has-a
//               relation.  As such, it's a little more robust, but
//               only works when the base type is, in fact, a class.
////////////////////////////////////////////////////////////////////
template<class Base>
class RefCountObj : public ReferenceCount, public Base {
public:
  INLINE RefCountObj();
  INLINE RefCountObj(const Base &copy);

  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type();

private:
  static TypeHandle _type_handle;
};



#include "referenceCount.I"

#endif
