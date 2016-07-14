/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file cppArrayType.cxx
 * @author drose
 * @date 1999-10-19
 */

#include "cppArrayType.h"
#include "cppExpression.h"

/**
 *
 */
CPPArrayType::
CPPArrayType(CPPType *element_type, CPPExpression *bounds) :
  CPPType(CPPFile()),
  _element_type(element_type),
  _bounds(bounds)
{
}

/**
 * Returns true if this declaration is an actual, factual declaration, or
 * false if some part of the declaration depends on a template parameter which
 * has not yet been instantiated.
 */
bool CPPArrayType::
is_fully_specified() const {
  return CPPType::is_fully_specified() &&
    _element_type->is_fully_specified();
}

/**
 * If this CPPType object is a forward reference or other nonspecified
 * reference to a type that might now be known a real type, returns the real
 * type.  Otherwise returns the type itself.
 */
CPPType *CPPArrayType::
resolve_type(CPPScope *current_scope, CPPScope *global_scope) {
  CPPType *ptype = _element_type->resolve_type(current_scope, global_scope);

  if (ptype != _element_type) {
    CPPArrayType *rep = new CPPArrayType(*this);
    rep->_element_type = ptype;
    return CPPType::new_type(rep);
  }
  return this;
}

/**
 * Returns true if the type, or any nested type within the type, is a
 * CPPTBDType and thus isn't fully determined right now.  In this case,
 * calling resolve_type() may or may not resolve the type.
 */
bool CPPArrayType::
is_tbd() const {
  return _element_type->is_tbd();
}

/**
 * Returns true if the type is considered a Plain Old Data (POD) type.
 */
bool CPPArrayType::
is_trivial() const {
  return _element_type->is_trivial();
}

/**
 * Returns true if the type is default-constructible.
 */
bool CPPArrayType::
is_default_constructible() const {
  return _element_type->is_default_constructible();
}

/**
 * Returns true if the type is copy-constructible.
 */
bool CPPArrayType::
is_copy_constructible() const {
  return false;
}

/**
 * This is a little more forgiving than is_equal(): it returns true if the
 * types appear to be referring to the same thing, even if they may have
 * different pointers or somewhat different definitions.  It's useful for
 * parameter matching, etc.
 */
bool CPPArrayType::
is_equivalent(const CPPType &other) const {
  const CPPArrayType *ot = ((CPPType *)&other)->as_array_type();
  if (ot == (CPPArrayType *)NULL) {
    return CPPType::is_equivalent(other);
  }

  return _element_type->is_equivalent(*ot->_element_type);
}

/**
 *
 */
CPPDeclaration *CPPArrayType::
substitute_decl(CPPDeclaration::SubstDecl &subst,
                CPPScope *current_scope, CPPScope *global_scope) {
  SubstDecl::const_iterator si = subst.find(this);
  if (si != subst.end()) {
    return (*si).second;
  }

  CPPArrayType *rep = new CPPArrayType(*this);
  rep->_element_type =
    _element_type->substitute_decl(subst, current_scope, global_scope)
    ->as_type();

  if (_bounds != NULL) {
    rep->_bounds =
      _bounds->substitute_decl(subst, current_scope, global_scope)
      ->as_expression();
  }

  if (rep->_element_type == _element_type &&
      rep->_bounds == _bounds) {
    delete rep;
    rep = this;
  }
  rep = CPPType::new_type(rep)->as_array_type();
  subst.insert(SubstDecl::value_type(this, rep));
  return rep;
}

/**
 *
 */
void CPPArrayType::
output(ostream &out, int indent_level, CPPScope *scope, bool complete) const {
  /*
  _element_type->output(out, indent_level, scope, complete);
  out << "[";
  if (_bounds != NULL) {
    out << *_bounds;
  }
  out << "]";
  */
  output_instance(out, indent_level, scope, complete, "", "");
}

/**
 * Formats a C++-looking line that defines an instance of the given type, with
 * the indicated name.  In most cases this will be "type name", but some types
 * have special exceptions.
 */
void CPPArrayType::
output_instance(ostream &out, int indent_level, CPPScope *scope,
                bool complete, const string &prename,
                const string &name) const {
  ostringstream brackets;
  brackets << "[";
  if (_bounds != NULL) {
    brackets << *_bounds;
  }
  brackets << "]";
  string bracketsstr = brackets.str();

  _element_type->output_instance(out, indent_level, scope, complete,
                                 prename, name + bracketsstr);
}

/**
 *
 */
CPPDeclaration::SubType CPPArrayType::
get_subtype() const {
  return ST_array;
}

/**
 *
 */
CPPArrayType *CPPArrayType::
as_array_type() {
  return this;
}

/**
 * Called by CPPDeclaration() to determine whether this type is equivalent to
 * another type of the same type.
 */
bool CPPArrayType::
is_equal(const CPPDeclaration *other) const {
  const CPPArrayType *ot = ((CPPDeclaration *)other)->as_array_type();
  assert(ot != NULL);

  if (_bounds != NULL && ot->_bounds != NULL) {
    if (*_bounds != *ot->_bounds) {
      return false;
    }
  } else if ((_bounds == NULL) != (ot->_bounds == NULL)) {
    return false;
  }

  return *_element_type == *ot->_element_type;
}


/**
 * Called by CPPDeclaration() to determine whether this type should be ordered
 * before another type of the same type, in an arbitrary but fixed ordering.
 */
bool CPPArrayType::
is_less(const CPPDeclaration *other) const {
  const CPPArrayType *ot = ((CPPDeclaration *)other)->as_array_type();
  assert(ot != NULL);

  if (_bounds != NULL && ot->_bounds != NULL) {
    if (*_bounds != *ot->_bounds) {
      return *_bounds < *ot->_bounds;
    }
  } else if ((_bounds == NULL) != (ot->_bounds == NULL)) {
    return _bounds < ot->_bounds;
  }

  if (*_element_type != *ot->_element_type) {
    return *_element_type < *ot->_element_type;
  }
  return false;
}