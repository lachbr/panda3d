// Filename: dcAtomicField.cxx
// Created by:  drose (05Oct00)
// 
////////////////////////////////////////////////////////////////////

#include "dcAtomicField.h"
#include "hashGenerator.h"
#include "dcindent.h"

#include <math.h>

ostream &
operator << (ostream &out, const DCAtomicField::ElementType &et) {
  out << et._type;
  if (et._divisor != 1) {
    out << " / " << et._divisor;
  }
  if (!et._name.empty()) {
    out << " " << et._name;
  }
  if (et._has_default_value) {
    out << " = <" << hex;
    string::const_iterator si;
    for (si = et._default_value.begin(); si != et._default_value.end(); ++si) {
      out << setw(2) << setfill('0') << (int)(unsigned char)(*si);
    }
    out << dec << ">";
  }
  return out;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::ElementType::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
DCAtomicField::ElementType::
ElementType() {
  _type = ST_invalid;
  _divisor = 1;
  _has_default_value = false;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::ElementType::set_default_value
//       Access: Public
//  Description: Stores the indicated value as the default value for
//               this element.
//
//               Returns true if the element type reasonably accepts a
//               default value of numeric type, false otherwise.
////////////////////////////////////////////////////////////////////
bool DCAtomicField::ElementType::
set_default_value(double num) {
  switch (_type) {
  case ST_int16array:
  case ST_uint16array:
  case ST_int32array:
  case ST_uint32array:
  case ST_blob:
    // These array types don't take numbers.
    return false;
  default:
    break;
  }

  string formatted;
  if (!format_default_value(num, formatted)) {
    return false;
  }

  _default_value = formatted;
  _has_default_value = true;
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::ElementType::set_default_value
//       Access: Public
//  Description: Stores the indicated value as the default value for
//               this element.
//
//               Returns true if the element type reasonably accepts a
//               default value of string type, false otherwise.
////////////////////////////////////////////////////////////////////
bool DCAtomicField::ElementType::
set_default_value(const string &str) {
  switch (_type) {
  case ST_int16array:
  case ST_uint16array:
  case ST_int32array:
  case ST_uint32array:
    // These array types don't take strings.
    return false;
  default:
    break;
  }

  string formatted;
  if (!format_default_value(str, formatted)) {
    return false;
  }

  _default_value = formatted;
  _has_default_value = true;
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::ElementType::set_default_value_literal
//       Access: Public
//  Description: Explicitly sets the default value to the given
//               pre-formatted string.
////////////////////////////////////////////////////////////////////
void DCAtomicField::ElementType::
set_default_value_literal(const string &str) {
  _default_value = str;
  _has_default_value = true;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::ElementType::add_default_value
//       Access: Public
//  Description: Appends the indicated value as the next array element
//               value for the default value for this type.
//
//               Returns true if the element type reasonably accepts a
//               default value of numeric type, false otherwise.
////////////////////////////////////////////////////////////////////
bool DCAtomicField::ElementType::
add_default_value(double num) {
  string formatted;
  if (!format_default_value(num, formatted)) {
    return false;
  }

  _default_value += formatted;
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::ElementType::end_array
//       Access: Public
//  Description: Called by the parser after a number of calls to
//               add_default_value(), to indicate the array has been
//               completed.
////////////////////////////////////////////////////////////////////
bool DCAtomicField::ElementType::
end_array() {
  switch (_type) {
  case ST_int16array:
  case ST_uint16array:
  case ST_int32array:
  case ST_uint32array:
  case ST_blob:
    {
      // We've accumulated all the elements of the array; now we must
      // prepend the array length.
      int length = _default_value.length();
      _default_value = 
	string(1, (char)(length & 0xff)) +
	string(1, (char)((length >> 8) & 0xff)) +
	_default_value;
    }
    return true;
    
  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::ElementType::format_default_value
//       Access: Private
//  Description: Formats the indicated default value to a sequence of
//               bytes, according to the element type.  Returns true
//               if the element type reasonably accepts a number,
//               false otherwise.
////////////////////////////////////////////////////////////////////
bool DCAtomicField::ElementType::
format_default_value(double num, string &formatted) const {
  double real_value = num * _divisor;
  int int_value = (int)floor(real_value + 0.5);

  switch (_type) {
  case ST_int8:
  case ST_uint8:
  case ST_blob:
    formatted = string(1, (char)(int_value & 0xff));
    break;

  case ST_int16:
  case ST_uint16:
  case ST_int16array:
  case ST_uint16array:
    formatted = 
      string(1, (char)(int_value & 0xff)) +
      string(1, (char)((int_value >> 8) & 0xff));
    break;

  case ST_int32:
  case ST_uint32:
  case ST_int32array:
  case ST_uint32array:
    formatted = 
      string(1, (char)(int_value & 0xff)) +
      string(1, (char)((int_value >> 8) & 0xff)) +
      string(1, (char)((int_value >> 16) & 0xff)) +
      string(1, (char)((int_value >> 24) & 0xff));
    break;

  case ST_int64:
    // We don't fully support default values for int64.  The
    // high-order 32 bits cannot be specified.
    formatted = 
      string(1, (char)(int_value & 0xff)) +
      string(1, (char)((int_value >> 8) & 0xff)) +
      string(1, (char)((int_value >> 16) & 0xff)) +
      string(1, (char)((int_value >> 24) & 0xff)) +
      ((int_value & 0x80000000) != 0 ? string(4, '\xff') : string(4, '\0'));
    break;

  case ST_uint64:
    // We don't fully support default values for int64.  The
    // high-order 32 bits cannot be specified.
    formatted = 
      string(1, (char)(int_value & 0xff)) +
      string(1, (char)((int_value >> 8) & 0xff)) +
      string(1, (char)((int_value >> 16) & 0xff)) +
      string(1, (char)((int_value >> 24) & 0xff)) +
      string(4, '\0');
    break;

  case ST_float64:
    // This may not be fully portable.
    formatted = string((char *)&real_value, 8);
#ifdef IS_BIG_ENDIAN
    {
      // Reverse the byte ordering for big-endian machines.
      string str;
      str = reserve(8);

      for (size_t i = 0; i < 8; i++) {
	str += formatted[length - 1 - i];
      }
      formatted = str;
    }
#endif
    break;

  case ST_string:
    // It doesn't make sense to assign a numeric default value to a
    // string.
    return false;

  case ST_invalid:
    break;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::ElementType::format_default_value
//       Access: Private
//  Description: Formats the indicated default value to a sequence of
//               bytes, according to the element type.  Returns true
//               if the element type reasonably accepts a string,
//               false otherwise.
////////////////////////////////////////////////////////////////////
bool DCAtomicField::ElementType::
format_default_value(const string &str, string &formatted) const {
  switch (_type) {
  case ST_string:
  case ST_blob:
    {
      int length = str.length();
      formatted = 
	string(1, (char)(length & 0xff)) +
	string(1, (char)((length >> 8) & 0xff)) +
	str;
    }
    break;

  default:
    // It doesn't make sense to assign a string default to a number.
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::as_atomic_field
//       Access: Public, Virtual
//  Description: Returns the same field pointer converted to an atomic
//               field pointer, if this is in fact an atomic field;
//               otherwise, returns NULL.
////////////////////////////////////////////////////////////////////
DCAtomicField *DCAtomicField::
as_atomic_field() {
  return this;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::get_num_elements
//       Access: Public
//  Description: Returns the number of elements of the atomic field.
////////////////////////////////////////////////////////////////////
int DCAtomicField::
get_num_elements() const {
  return _elements.size();
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::get_element_type
//       Access: Public
//  Description: Returns the numeric type of the nth element of the
//               field.
////////////////////////////////////////////////////////////////////
DCSubatomicType DCAtomicField::
get_element_type(int n) const {
  nassertr(n >= 0 && n < (int)_elements.size(), ST_invalid);
  return _elements[n]._type;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::get_element_name
//       Access: Public
//  Description: Returns the name of the nth element of the field.
//               This name is strictly for documentary purposes; it
//               does not generally affect operation.  If a name is
//               not specified, this will be the empty string.
////////////////////////////////////////////////////////////////////
string DCAtomicField::
get_element_name(int n) const {
  nassertr(n >= 0 && n < (int)_elements.size(), string());
  return _elements[n]._name;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::get_element_divisor
//       Access: Public
//  Description: Returns the divisor associated with the nth element
//               of the field.  This implements an implicit
//               fixed-point system; floating-point values are to be
//               multiplied by this value before encoding into a
//               packet, and divided by this number after decoding.
////////////////////////////////////////////////////////////////////
int DCAtomicField::
get_element_divisor(int n) const {
  nassertr(n >= 0 && n < (int)_elements.size(), 1);
  return _elements[n]._divisor;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::get_element_default
//       Access: Public
//  Description: Returns the pre-formatted default value associated
//               with the nth element of the field.  This is only
//               valid if has_element_default() returns true, in which
//               case this string represents the bytes that should be
//               assigned to the field as a default value.
////////////////////////////////////////////////////////////////////
string DCAtomicField::
get_element_default(int n) const {
  nassertr(has_element_default(n), string());
  nassertr(n >= 0 && n < (int)_elements.size(), string());
  return _elements[n]._default_value;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::has_element_default
//       Access: Public
//  Description: Returns true if the nth element of the field has a
//               default value specified, false otherwise.
////////////////////////////////////////////////////////////////////
bool DCAtomicField::
has_element_default(int n) const {
  nassertr(n >= 0 && n < (int)_elements.size(), false);
  return _elements[n]._has_default_value;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::is_required
//       Access: Public
//  Description: Returns true if the "required" flag is set for this
//               field, false otherwise.
////////////////////////////////////////////////////////////////////
bool DCAtomicField::
is_required() const {
  return (_flags & F_required) != 0;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::is_broadcast
//       Access: Public
//  Description: Returns true if the "broadcast" flag is set for this
//               field, false otherwise.
////////////////////////////////////////////////////////////////////
bool DCAtomicField::
is_broadcast() const {
  return (_flags & F_broadcast) != 0;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::is_p2p
//       Access: Public
//  Description: Returns true if the "p2p" flag is set for this
//               field, false otherwise.
////////////////////////////////////////////////////////////////////
bool DCAtomicField::
is_p2p() const {
  return (_flags & F_p2p) != 0;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::is_ram
//       Access: Public
//  Description: Returns true if the "ram" flag is set for this
//               field, false otherwise.
////////////////////////////////////////////////////////////////////
bool DCAtomicField::
is_ram() const {
  return (_flags & F_ram) != 0;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::is_db
//       Access: Public
//  Description: Returns true if the "db" flag is set for this
//               field, false otherwise.
////////////////////////////////////////////////////////////////////
bool DCAtomicField::
is_db() const {
  return (_flags & F_db) != 0;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::is_clsend
//       Access: Public
//  Description: Returns true if the "clsend" flag is set for this
//               field, false otherwise.
////////////////////////////////////////////////////////////////////
bool DCAtomicField::
is_clsend() const {
  return (_flags & F_clsend) != 0;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::is_clrecv
//       Access: Public
//  Description: Returns true if the "clrecv" flag is set for this
//               field, false otherwise.
////////////////////////////////////////////////////////////////////
bool DCAtomicField::
is_clrecv() const {
  return (_flags & F_clrecv) != 0;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::is_ownsend
//       Access: Public
//  Description: Returns true if the "ownsend" flag is set for this
//               field, false otherwise.
////////////////////////////////////////////////////////////////////
bool DCAtomicField::
is_ownsend() const {
  return (_flags & F_ownsend) != 0;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
DCAtomicField::
DCAtomicField() {
  _number = 0;
  _flags = 0;
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::write
//       Access: Public, Virtual
//  Description: Generates a parseable description of the object to
//               the indicated output stream.
////////////////////////////////////////////////////////////////////
void DCAtomicField::
write(ostream &out, int indent_level) const {
  indent(out, indent_level)
    << _name << "(";

  if (!_elements.empty()) {
    Elements::const_iterator ei = _elements.begin();
    out << (*ei);
    ++ei;
    while (ei != _elements.end()) {
      out << ", " << (*ei);
      ++ei;
    }
  }
  out << ")";

  if ((_flags & F_required) != 0) {
    out << " required";
  }
  if ((_flags & F_broadcast) != 0) {
    out << " broadcast";
  }
  if ((_flags & F_p2p) != 0) {
    out << " p2p";
  }
  if ((_flags & F_ram) != 0) {
    out << " ram";
  }
  if ((_flags & F_db) != 0) {
    out << " db";
  }
  if ((_flags & F_clsend) != 0) {
    out << " clsend";
  }
  if ((_flags & F_clrecv) != 0) {
    out << " clrecv";
  }
  if ((_flags & F_ownsend) != 0) {
    out << " ownsend";
  }

  out << ";  // field " << _number << "\n";
}

////////////////////////////////////////////////////////////////////
//     Function: DCAtomicField::generate_hash
//       Access: Public, Virtual
//  Description: Accumulates the properties of this field into the
//               hash.
////////////////////////////////////////////////////////////////////
void DCAtomicField::
generate_hash(HashGenerator &hash) const {
  DCField::generate_hash(hash);

  hash.add_int(_elements.size());
  Elements::const_iterator ei;
  for (ei = _elements.begin(); ei != _elements.end(); ++ei) {
    const ElementType &element = (*ei);
    hash.add_int(element._type);
    hash.add_int(element._divisor);
  }
  hash.add_int(_flags);
}
