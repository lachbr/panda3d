// Filename: staticTextFont.cxx
// Created by:  drose (03May01)
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

#include "staticTextFont.h"
#include "config_text.h"

#include "geom.h"
#include "geomPoint.h"
#include "qpgeomNode.h"
#include "renderState.h"

#include "geomNode.h"
#include "namedNode.h"
#include "renderRelation.h"

TypeHandle StaticTextFont::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: StaticTextFont::Constructor
//       Access: Published
//  Description: The constructor expects the root node to a model
//               generated via egg-mkfont, which consists of a set of
//               models, one per each character in the font.
////////////////////////////////////////////////////////////////////
StaticTextFont::
StaticTextFont(Node *font_def) {
  nassertv(font_def != (Node *)NULL);
  _font = font_def;
  _glyphs.clear();

  find_characters(font_def);
  _is_valid = !_glyphs.empty();

  if (_font->is_of_type(NamedNode::get_class_type())) {
    NamedNode *named_node = DCAST(NamedNode, _font);
    set_name(named_node->get_name());
  }
}

////////////////////////////////////////////////////////////////////
//     Function: StaticTextFont::Constructor
//       Access: Published
//  Description: The constructor expects the root node to a model
//               generated via egg-mkfont, which consists of a set of
//               models, one per each character in the font.
////////////////////////////////////////////////////////////////////
StaticTextFont::
StaticTextFont(PandaNode *font_def) {
  nassertv(font_def != (PandaNode *)NULL);
  _qpfont = font_def;
  _glyphs.clear();

  find_characters(font_def, RenderState::make_empty());
  _is_valid = !_glyphs.empty();

  set_name(font_def->get_name());
}

////////////////////////////////////////////////////////////////////
//     Function: StaticTextFont::write
//       Access: Published, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
void StaticTextFont::
write(ostream &out, int indent_level) const {
  indent(out, indent_level)
    << "StaticTextFont " << get_name() << "; "
    << _glyphs.size() << " characters available in font:\n";
  Glyphs::const_iterator gi;
  
  // Figure out which symbols we have.  We collect lowercase letters,
  // uppercase letters, and digits together for the user's
  // convenience.
  static const int num_letters = 26;
  static const int num_digits = 10;
  bool lowercase[num_letters];
  bool uppercase[num_letters];
  bool digits[num_digits];

  memset(lowercase, 0, sizeof(bool) * num_letters);
  memset(uppercase, 0, sizeof(bool) * num_letters);
  memset(digits, 0, sizeof(bool) * num_digits);

  int count_lowercase = 0;
  int count_uppercase = 0;
  int count_digits = 0;

  for (gi = _glyphs.begin(); gi != _glyphs.end(); ++gi) {
    int ch = (*gi).first;
    if (ch < 128) {
      if (islower(ch)) {
        count_lowercase++;
        lowercase[ch - 'a'] = true;
        
      } else if (isupper(ch)) {
        count_uppercase++;
        uppercase[ch - 'A'] = true;
        
      } else if (isdigit(ch)) {
        count_digits++;
        digits[ch - '0'] = true;
      }
    }
  }

  if (count_lowercase == num_letters) {
    indent(out, indent_level + 2)
      << "All lowercase letters\n";

  } else if (count_lowercase > 0) {
    indent(out, indent_level + 2)
      << "Some lowercase letters: ";
    for (int i = 0; i < num_letters; i++) {
      if (lowercase[i]) {
        out << (char)(i + 'a');
      }
    }
    out << "\n";
  }

  if (count_uppercase == num_letters) {
    indent(out, indent_level + 2)
      << "All uppercase letters\n";

  } else if (count_uppercase > 0) {
    indent(out, indent_level + 2)
      << "Some uppercase letters: ";
    for (int i = 0; i < num_letters; i++) {
      if (uppercase[i]) {
        out << (char)(i + 'A');
      }
    }
    out << "\n";
  }

  if (count_digits == num_digits) {
    indent(out, indent_level + 2)
      << "All digits\n";

  } else if (count_digits > 0) {
    indent(out, indent_level + 2)
      << "Some digits: ";
    for (int i = 0; i < num_digits; i++) {
      if (digits[i]) {
        out << (char)(i + '0');
      }
    }
    out << "\n";
  }

  for (gi = _glyphs.begin(); gi != _glyphs.end(); ++gi) {
    int ch = (*gi).first;
    if (ch >= 128 || !isalnum(ch)) {
      indent(out, indent_level + 2)
        << ch;
      if (ch < isprint(ch)) {
        out << " = '" << (char)ch << "'\n";
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: StaticTextFont::get_glyph
//       Access: Public, Virtual
//  Description: Gets the glyph associated with the given character
//               code, as well as an optional scaling parameter that
//               should be applied to the glyph's geometry and advance
//               parameters.  Returns true if the glyph exists, false
//               if it does not.  Even if the return value is false,
//               the value for glyph might be filled in with a
//               printable glyph.
////////////////////////////////////////////////////////////////////
bool StaticTextFont::
get_glyph(int character, const TextGlyph *&glyph, float &glyph_scale) {
  glyph_scale = 1.0f;

  Glyphs::const_iterator gi = _glyphs.find(character);
  if (gi == _glyphs.end()) {
    // No definition for this character.
    glyph = (TextGlyph *)NULL;
    return false;
  }

  glyph = (*gi).second;
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: StaticTextFont::find_character_gsets
//       Access: Private
//  Description: Given that 'root' is a Node containing at least a
//               polygon and a point which define the character's
//               appearance and kern position, respectively,
//               recursively walk the hierarchy and root and locate
//               those two Geoms.
////////////////////////////////////////////////////////////////////
bool StaticTextFont::
find_character_gsets(Node *root, Geom *&ch, GeomPoint *&dot,
                     AllTransitionsWrapper &trans) {
  if (root->is_of_type(GeomNode::get_class_type())) {
    GeomNode *geode = (GeomNode *)root;

    bool found = false;
    for (int i = 0; i < geode->get_num_geoms(); i++) {
      dDrawable *geom = geode->get_geom(i);
      if (geom->is_of_type(GeomPoint::get_class_type())) {
        dot = DCAST(GeomPoint, geom);

      } else if (geom->is_of_type(Geom::get_class_type())) {
        ch = DCAST(Geom, geom);
        found = true;
      }
    }
    return found;

  } else {
    TypeHandle graph_type = RenderRelation::get_class_type();
    int num_children = root->get_num_children(graph_type);
    for (int i = 0; i < num_children; i++) {
      NodeRelation *child_arc = root->get_child(graph_type, i);
      if (find_character_gsets(child_arc->get_child(), ch, dot, trans)) {
        trans.extract_from(child_arc);
      }
    }
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: StaticTextFont::find_characters
//       Access: Private
//  Description: Walk the hierarchy beginning at the indicated root
//               and locate any nodes whose names are just integers.
//               These are taken to be characters, and their
//               definitions and kern informations are retrieved.
////////////////////////////////////////////////////////////////////
void StaticTextFont::
find_characters(Node *root) {
  string name;
  if (root->is_of_type(NamedNode::get_class_type())) {
    name = DCAST(NamedNode, root)->get_name();
  }

  bool all_digits = !name.empty();
  const char *p = name.c_str();
  while (all_digits && *p != '\0') {
    // VC++ complains if we treat an int as a bool, so we have to do
    // this != 0 comparsion on the int isdigit() function to shut it
    // up.
    all_digits = (isdigit(*p) != 0);
    p++;
  }

  if (all_digits) {
    int character = atoi(name.c_str());
    Geom *ch = NULL;
    GeomPoint *dot = NULL;
    AllTransitionsWrapper trans;
    find_character_gsets(root, ch, dot, trans);
    if (dot != NULL) {
      // Get the first vertex from the "dot" geoset.  This will be the
      // origin of the next character.
      PTA_Vertexf alist;
      PTA_ushort ilist;
      float width;
      dot->get_coords(alist, ilist);
      if (ilist.empty()) {
        width = alist[0][0];
      } else {
        width = alist[ilist[0]][0];
      }

      _glyphs[character] = new TextGlyph(ch, trans, width);
    }

  } else if (name == "ds") {
    // The group "ds" is a special node that indicate's the font's
    // design size, or line height.

    Geom *ch = NULL;
    GeomPoint *dot = NULL;
    AllTransitionsWrapper trans;
    find_character_gsets(root, ch, dot, trans);
    if (dot != NULL) {
      // Get the first vertex from the "dot" geoset.  This will be the
      // design size indicator.
      PTA_Vertexf alist;
      PTA_ushort ilist;
      dot->get_coords(alist, ilist);
      if (ilist.empty()) {
        _line_height = alist[0][2];
      } else {
        _line_height = alist[ilist[0]][2];
      }
      _space_advance = 0.25f * _line_height;
    }

  } else {
    TypeHandle graph_type = RenderRelation::get_class_type();
    int num_children = root->get_num_children(graph_type);
    for (int i = 0; i < num_children; i++) {
      NodeRelation *child_arc = root->get_child(graph_type, i);
      find_characters(child_arc->get_child());
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: StaticTextFont::find_character_gsets
//       Access: Private
//  Description: Given that 'root' is a PandaNode containing at least
//               a polygon and a point which define the character's
//               appearance and kern position, respectively,
//               recursively walk the hierarchy and root and locate
//               those two Geoms.
////////////////////////////////////////////////////////////////////
void StaticTextFont::
find_character_gsets(PandaNode *root, Geom *&ch, GeomPoint *&dot,
                     const RenderState *&state, const RenderState *net_state) {
  CPT(RenderState) next_net_state = net_state->compose(root->get_state());

  if (root->is_geom_node()) {
    qpGeomNode *geode = DCAST(qpGeomNode, root);

    for (int i = 0; i < geode->get_num_geoms(); i++) {
      dDrawable *geom = geode->get_geom(i);
      if (geom->is_of_type(GeomPoint::get_class_type())) {
        dot = DCAST(GeomPoint, geom);

      } else if (geom->is_of_type(Geom::get_class_type())) {
        ch = DCAST(Geom, geom);
        state = next_net_state->compose(geode->get_geom_state(i));
      }
    }

  } else {
    PandaNode::Children cr = root->get_children();
    int num_children = cr.get_num_children();
    for (int i = 0; i < num_children; i++) {
      find_character_gsets(cr.get_child(i), ch, dot, state, next_net_state);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: StaticTextFont::find_characters
//       Access: Private
//  Description: Walk the hierarchy beginning at the indicated root
//               and locate any nodes whose names are just integers.
//               These are taken to be characters, and their
//               definitions and kern informations are retrieved.
////////////////////////////////////////////////////////////////////
void StaticTextFont::
find_characters(PandaNode *root, const RenderState *net_state) {
  CPT(RenderState) next_net_state = net_state->compose(root->get_state());
  string name = root->get_name();

  bool all_digits = !name.empty();
  const char *p = name.c_str();
  while (all_digits && *p != '\0') {
    // VC++ complains if we treat an int as a bool, so we have to do
    // this != 0 comparison on the int isdigit() function to shut it
    // up.
    all_digits = (isdigit(*p) != 0);
    p++;
  }

  if (all_digits) {
    int character = atoi(name.c_str());
    Geom *ch = NULL;
    GeomPoint *dot = NULL;
    const RenderState *state = NULL;
    find_character_gsets(root, ch, dot, state, next_net_state);
    if (dot != NULL) {
      // Get the first vertex from the "dot" geoset.  This will be the
      // origin of the next character.
      PTA_Vertexf alist;
      PTA_ushort ilist;
      float width;
      dot->get_coords(alist, ilist);
      if (ilist.empty()) {
        width = alist[0][0];
      } else {
        width = alist[ilist[0]][0];
      }

      _glyphs[character] = new TextGlyph(ch, state, width);
    }

  } else if (name == "ds") {
    // The group "ds" is a special node that indicate's the font's
    // design size, or line height.

    Geom *ch = NULL;
    GeomPoint *dot = NULL;
    const RenderState *state = NULL;
    find_character_gsets(root, ch, dot, state, next_net_state);
    if (dot != NULL) {
      // Get the first vertex from the "dot" geoset.  This will be the
      // design size indicator.
      PTA_Vertexf alist;
      PTA_ushort ilist;
      dot->get_coords(alist, ilist);
      if (ilist.empty()) {
        _line_height = alist[0][2];
      } else {
        _line_height = alist[ilist[0]][2];
      }
      _space_advance = 0.25f * _line_height;
    }

  } else {
    PandaNode::Children cr = root->get_children();
    int num_children = cr.get_num_children();
    for (int i = 0; i < num_children; i++) {
      find_characters(cr.get_child(i), next_net_state);
    }
  }
}
