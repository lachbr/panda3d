// Filename: dcMolecularField.h
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

#ifndef DCMOLECULARFIELD_H
#define DCMOLECULARFIELD_H

#include "dcbase.h"
#include "dcField.h"

class DCAtomicField;

////////////////////////////////////////////////////////////////////
//       Class : DCMolecularField
// Description : A single molecular field of a Distributed Class, as
//               read from a .dc file.  This represents a combination
//               of two or more related atomic fields, that will often
//               be treated as a unit.
////////////////////////////////////////////////////////////////////
class EXPCL_DIRECT DCMolecularField : public DCField {
PUBLISHED:
  virtual DCMolecularField *as_molecular_field();

  int get_num_atomics() const;
  DCAtomicField *get_atomic(int n) const;

public:
  DCMolecularField();
  virtual void write(ostream &out, int indent_level = 0) const;
  virtual void generate_hash(HashGenerator &hash) const;

public:
  // These members define the primary interface to the molecular field
  // definition as read from the file.
  typedef pvector<DCAtomicField *> Fields;
  Fields _fields;
};

#endif


