// Filename: namable.h
// Created by:  drose (15Jan99)
//

#ifndef NAMABLE_H
#define NAMABLE_H

#include <pandabase.h>

#include "typedObject.h"
#include <string>

///////////////////////////////////////////////////////////////////
//       Class : Namable
// Description : A base class for all things which can have a name.
//               The name is either empty or nonempty, but it is never
//               NULL.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAEXPRESS Namable {
PUBLISHED:
  INLINE Namable(const string &initial_name = "");
  INLINE Namable(const Namable &copy);
  INLINE Namable &operator = (const Namable &other);

  INLINE void set_name(const string &name);
  INLINE void clear_name();
  INLINE bool has_name() const;
  INLINE const string &get_name() const;

  // In the absence of any definition to the contrary, outputting a
  // Namable will write out its name.
  INLINE void output(ostream &out) const;

private:
  string _name;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    register_type(_type_handle, "Namable");
  }

private:
  static TypeHandle _type_handle;
};

INLINE ostream &operator << (ostream &out, const Namable &n);

///////////////////////////////////////////////////////////////////
//       Class : NamableOrderByName
// Description : An STL function object for sorting an array of
//               pointers to Namables into order by name.  Returns
//               true if the objects are in sorted order, false
//               otherwise.
////////////////////////////////////////////////////////////////////
class NamableOrderByName {
public:
  INLINE bool operator ()(const Namable *n1, const Namable *n2) const;
};

#include "namable.I"

#endif

 
