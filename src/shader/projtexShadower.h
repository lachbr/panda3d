// Filename: projtexShadower.h
// Created by:  mike (09Jan97)
//
////////////////////////////////////////////////////////////////////
//
#ifndef PROJTEXSHADOWER_H
#define PROJTEXSHADOWER_H
//
////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////
#include <pandabase.h>

#include "casterShader.h"
#include "projtexShader.h"

#include <pt_Node.h>

////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//       Class : ProjtexShadower
// Description : Creates the shadow of a casting object(s) and projects
//               it onto a receiving object(s) from a given light
//               frustum
////////////////////////////////////////////////////////////////////
class EXPCL_SHADER ProjtexShadower : public CasterShader
{
public:
  
  ProjtexShadower(int size = 256);
  
  virtual void pre_apply(Node *node, const AllAttributesWrapper &init_state,
                         const AllTransitionsWrapper &net_trans,
                         GraphicsStateGuardian *gsg);
  virtual void apply(Node *node, const AllAttributesWrapper &init_state,
                     const AllTransitionsWrapper &net_trans,
                     GraphicsStateGuardian *gsg);
  
  // MPG - should we make this check for powers of 2?
  INLINE void set_size(int size) { _size = size; }
  INLINE int get_size(void) { return _size; }
  
  //Override base class shader's set_priority to allow
  //us to set the priority of the contained projtex_shader at 
  //the same time
  virtual void set_priority(int priority);
  virtual void set_multipass(bool on);
  
protected:
  
  PT(ProjtexShader)             _projtex_shader;
  int                           _size;
  
public:
  
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    CasterShader::init_type();
    register_type(_type_handle, "ProjtexShadower",
                  CasterShader::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}
  
private:
  
  static TypeHandle _type_handle;
};

#endif
