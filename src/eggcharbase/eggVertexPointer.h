// Filename: eggVertexPointer.h
// Created by:  drose (26Feb01)
// 
////////////////////////////////////////////////////////////////////

#ifndef EGGVERTEXPOINTER_H
#define EGGVERTEXPOINTER_H

#include <pandatoolbase.h>

#include "eggBackPointer.h"

#include <eggGroup.h>
#include <pointerTo.h>

////////////////////////////////////////////////////////////////////
// 	 Class : EggVertexPointer
// Description : This stores a pointer back to a <Vertex>, or to a
//               particular pritimive like a <Polygon>, representing a
//               morph offset.
////////////////////////////////////////////////////////////////////
class EggVertexPointer : public EggBackPointer {
public:
  EggVertexPointer();

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    EggBackPointer::init_type();
    register_type(_type_handle, "EggVertexPointer",
		  EggBackPointer::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}
 
private:
  static TypeHandle _type_handle;
};

#endif


