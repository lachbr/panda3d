// Filename: eggAttributes.cxx
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

#include "eggAttributes.h"
#include "eggParameters.h"
#include "eggMorph.h"
#include "eggMorphList.h"

#include <indent.h>
#include <math.h>

TypeHandle EggAttributes::_type_handle;


////////////////////////////////////////////////////////////////////
//     Function: EggAttributes::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
EggAttributes::
EggAttributes() {
  _flags = 0;
}

////////////////////////////////////////////////////////////////////
//     Function: EggAttributes::Copy constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
EggAttributes::
EggAttributes(const EggAttributes &copy) {
  (*this) = copy;
}

////////////////////////////////////////////////////////////////////
//     Function: EggAttributes::Copy assignment operator
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
EggAttributes &EggAttributes::
operator = (const EggAttributes &copy) {
  _flags = copy._flags;
  _normal = copy._normal;
  _uv = copy._uv;
  _color = copy._color;
  _dnormals = copy._dnormals;
  _duvs = copy._duvs;
  _drgbas = copy._drgbas;
  return *this;
}

////////////////////////////////////////////////////////////////////
//     Function: EggAttributes::Destructor
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
EggAttributes::
~EggAttributes() {
}


////////////////////////////////////////////////////////////////////
//     Function: EggAttributes::write
//       Access: Public
//  Description: Writes the attributes to the indicated output stream in
//               Egg format.
////////////////////////////////////////////////////////////////////
void EggAttributes::
write(ostream &out, int indent_level) const {
  if (has_normal()) {
    if (_dnormals.empty()) {
      indent(out, indent_level)
        << "<Normal> { " << get_normal() << " }\n";
    } else {
      indent(out, indent_level) << "<Normal> {\n";
      indent(out, indent_level+2) << get_normal() << "\n";
      _dnormals.write(out, indent_level+2);
      indent(out, indent_level) << "}\n";
    }
  }
  if (has_uv()) {
    if (_duvs.empty()) {
      indent(out, indent_level)
        << "<UV> { " << get_uv() << " }\n";
    } else {
      indent(out, indent_level) << "<UV> {\n";
      indent(out, indent_level+2) << get_uv() << "\n";
      _duvs.write(out, indent_level+2);
      indent(out, indent_level) << "}\n";
    }
  }
  if (has_color()) {
    if (_drgbas.empty()) {
      indent(out, indent_level)
        << "<RGBA> { " << get_color() << " }\n";
    } else {
      indent(out, indent_level) << "<RGBA> {\n";
      indent(out, indent_level+2) << get_color() << "\n";
      _drgbas.write(out, indent_level+2);
      indent(out, indent_level) << "}\n";
    }
  }
}


////////////////////////////////////////////////////////////////////
//     Function: EggAttributes::sorts_less_than
//       Access: Public
//  Description: An ordering operator to compare two vertices for
//               sorting order.  This imposes an arbitrary ordering
//               useful to identify unique vertices.
////////////////////////////////////////////////////////////////////
bool EggAttributes::
sorts_less_than(const EggAttributes &other) const {
  if (_flags != other._flags) {
    return _flags < other._flags;
  }

  if (has_normal()) {
    int compare =
      _normal.compare_to(other._normal, egg_parameters->_normal_threshold);
    if (compare != 0) {
      return compare < 0;
    }
    if (_dnormals != other._dnormals) {
      return _dnormals < other._dnormals;
    }
  }

  if (has_uv()) {
    int compare =
      _uv.compare_to(other._uv, egg_parameters->_uv_threshold);
    if (compare != 0) {
      return compare < 0;
    }
    if (_duvs != other._duvs) {
      return _duvs < other._duvs;
    }
  }

  if (has_color()) {
    int compare =
      _color.compare_to(other._color, egg_parameters->_color_threshold);
    if (compare != 0) {
      return compare < 0;
    }
    if (_drgbas != other._drgbas) {
      return _drgbas < other._drgbas;
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: EggAttributes::transform
//       Access: Public, Virtual
//  Description: Applies the indicated transformation matrix to the
//               attributes.
////////////////////////////////////////////////////////////////////
void EggAttributes::
transform(const LMatrix4d &mat) {
  if (has_normal()) {
    _normal = _normal * mat;
    LVector3d old_normal = _normal;
    _normal.normalize();

    EggMorphNormalList::iterator mi;
    for (mi = _dnormals.begin(); mi != _dnormals.end(); ++mi) {
      // We can safely cast the morph object to a non-const, because
      // we're not changing its name, which is the only thing the set
      // cares about preserving.
      EggMorphNormal &morph = (EggMorphNormal &)(*mi);

      // A bit of funny business to ensure the offset normal is
      // normalized after the transform.  This will break strange
      // normal morphs that want to change the length of the normal,
      // but what else can we do?
      LVector3d offset = (*mi).get_offset() * mat;
      LVector3d n = old_normal + offset;
      n.normalize();
      morph.set_offset(n - _normal);
    }
  }
}
