// Filename: dcAtomicField.h
// Created by:  drose (05Oct00)
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

#ifndef DCATOMICFIELD_H
#define DCATOMICFIELD_H

#include <cmath.h>
#include "dcbase.h"
#include "dcField.h"
#include "dcSubatomicType.h"

////////////////////////////////////////////////////////////////////
//       Class : DCAtomicField
// Description : A single atomic field of a Distributed Class, as read
//               from a .dc file.  This defines an interface to the
//               Distributed Class, and is always implemented as a
//               remote procedure method.
////////////////////////////////////////////////////////////////////
class EXPCL_DIRECT DCAtomicField : public DCField {
PUBLISHED:
  virtual DCAtomicField *as_atomic_field();

  int get_num_elements() const;
  DCSubatomicType get_element_type(int n) const;
  string get_element_name(int n) const;
  int get_element_divisor(int n) const;
  string get_element_default(int n) const;
  bool has_element_default(int n) const;

  bool is_required() const;
  bool is_broadcast() const;
  bool is_p2p() const;
  bool is_ram() const;
  bool is_db() const;
  bool is_clsend() const;
  bool is_clrecv() const;
  bool is_ownsend() const;

public:
  DCAtomicField();
  virtual void write(ostream &out, int indent_level = 0) const;
  virtual void generate_hash(HashGenerator &hash) const;

public:
  // These members define the primary interface to the atomic field
  // definition as read from the file.
  class ElementType {
  public:
    ElementType();
    bool set_default_value(double num);
    bool set_default_value(const string &str);
    bool set_default_value_literal(const string &str);

    bool add_default_value(double num);
    bool add_default_value(const string &str);
    bool add_default_value_literal(const string &str);
    bool end_array();

    DCSubatomicType _type;
    string _name;
    int _divisor;
    string _default_value;
    bool _has_default_value;

  private:
    bool format_default_value(double num, string &formatted) const;
    bool format_default_value(const string &str, string &formatted) const;
  };

  typedef pvector<ElementType> Elements;
  Elements _elements;

  enum Flags {
    F_required        = 0x0001,
    F_broadcast       = 0x0002,
    F_p2p             = 0x0004,
    F_ram             = 0x0008,
    F_db              = 0x0010,
    F_clsend          = 0x0020,
    F_clrecv          = 0x0040,
    F_ownsend         = 0x0080,
  };

  int _flags;  // A bitmask union of any of the above values.
};

#endif
