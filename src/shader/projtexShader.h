// Filename: projtexShader.h
// Created by:  mike (09Jan97)
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
#ifndef PROJTEXSHADER_H
#define PROJTEXSHADER_H
//
////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////
#include <pandabase.h>

#include "shader.h"
#include <texture.h>
#include <colorBlendProperty.h>

////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//       Class : ProjtexShader
// Description :
////////////////////////////////////////////////////////////////////
class EXPCL_SHADER ProjtexShader : public FrustumShader
{
public:

  ProjtexShader(Texture* texture = NULL,
                ColorBlendProperty::Mode mode = ColorBlendProperty::M_multiply);
  ~ProjtexShader() { }

  virtual void config(void);
  virtual void apply(Node *node, const AllAttributesWrapper &init_state,
                     const AllTransitionsWrapper &net_trans,
                     GraphicsStateGuardian *gsg);

  INLINE void set_texture(Texture* texture) {
    _texture = texture;
    make_dirty();
  }

  INLINE Texture* get_texture(void) { return _texture; }

protected:

  PT(Texture) _texture;
  ColorBlendProperty::Mode _blend;

public:

  static TypeHandle get_class_type() {
    return _type_handle;
    }
  static void init_type() {
    FrustumShader::init_type();
    register_type(_type_handle, "ProjtexShader",
                  FrustumShader::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
    virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:

  static TypeHandle _type_handle;
};

#endif
