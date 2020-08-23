#pragma once

#include "extension.h"
#include "solid.h"
#include "py_panda.h"

template<>
class Extension<CSolid> : public ExtensionBase<CSolid> {
public:
  PyObject *split(const CPlane &plane);
};

INLINE PyObject *Extension<CSolid>::
split(const CPlane &plane) {
  PT(CSolid) front, back;
  bool ret = _this->split(plane, &front, &back);

  PyObject *pyret, *pyfront, *pyback;

  if (front) {
    pyfront = DTool_CreatePyInstance<CSolid>(front, true);
  } else {
    pyfront = Py_None;
  }

  if (back) {
    pyback = DTool_CreatePyInstance<CSolid>(back, true);
  } else {
    pyback = Py_None;
  }

  pyret = ret ? Py_True : Py_False;

  PyObject *tuple = PyTuple_Pack(3, pyret, pyfront, pyback);
  return tuple;
}
