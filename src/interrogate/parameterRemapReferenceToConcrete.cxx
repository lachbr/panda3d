// Filename: parameterRemapReferenceToConcrete.C
// Created by:  drose (04Aug00)
// 
////////////////////////////////////////////////////////////////////

#include "parameterRemapReferenceToConcrete.h"
#include "typeManager.h"

#include <cppType.h>
#include <cppDeclaration.h>
#include <cppConstType.h>
#include <cppPointerType.h>
#include <cppReferenceType.h>

////////////////////////////////////////////////////////////////////
//     Function: ParameterRemapReferenceToConcrete::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
ParameterRemapReferenceToConcrete::
ParameterRemapReferenceToConcrete(CPPType *orig_type) :
  ParameterRemap(orig_type)
{
  _new_type = TypeManager::unwrap_const_reference(orig_type);
}

////////////////////////////////////////////////////////////////////
//     Function: ParameterRemapReferenceToConcrete::pass_parameter
//       Access: Public, Virtual
//  Description: Outputs an expression that converts the indicated
//               variable from the new type to the original type, for
//               passing into the actual C++ function.
////////////////////////////////////////////////////////////////////
void ParameterRemapReferenceToConcrete::
pass_parameter(ostream &out, const string &variable_name) {
  out << variable_name;
}

////////////////////////////////////////////////////////////////////
//     Function: ParameterRemapReferenceToConcrete::get_return_expr
//       Access: Public, Virtual
//  Description: Returns an expression that evalutes to the
//               appropriate value type for returning from the
//               function, given an expression of the original type.
////////////////////////////////////////////////////////////////////
string ParameterRemapReferenceToConcrete::
get_return_expr(const string &expression) {
  return expression;
}

