// Filename: loaderFileType.h
// Created by:  drose (20Jun00)
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

#ifndef LOADERFILETYPE_H
#define LOADERFILETYPE_H

#include "pandabase.h"

#include "typedObject.h"
#include "filename.h"
#include "pandaNode.h"
#include "pointerTo.h"

////////////////////////////////////////////////////////////////////
//       Class : LoaderFileType
// Description : This is the base class for a family of scene-graph
//               file types that the Loader supports.  Each kind of
//               loader that's available should define a corresponding
//               LoaderFileType object and register itself.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA LoaderFileType : public TypedObject {
protected:
  LoaderFileType();

public:
  virtual ~LoaderFileType();

  virtual string get_name() const=0;
  virtual string get_extension() const=0;

  virtual void resolve_filename(Filename &path) const;
  virtual PT(PandaNode) load_file(const Filename &path, bool report_errors) const;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    TypedObject::init_type();
    register_type(_type_handle, "LoaderFileType",
                  TypedObject::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

#endif

