// Filename: glgsg.h
// Created by:  drose (09Feb04)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001 - 2004, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://etc.cmu.edu/panda3d/docs/license/ .
//
// To contact the maintainers of this program write to
// panda3d-general@lists.sourceforge.net .
//
////////////////////////////////////////////////////////////////////

#ifndef GLGSG_H
#define GLGSG_H

// This is the actual header file to include if you want to pick up
// any or all of the header files in this directory as compiled to use
// the "true" GL library.

#include "pandabase.h"
#include "config_glgsg.h"

#define GLP(name) gl##name
#define GLUP(name) glu##name
#define CLP(name) GL##name
#define GLPREFIX_QUOTED "gl"
#define CLASSPREFIX_QUOTED "GL"
#define GLSYSTEM_NAME "OpenGL"
#define CONFIGOBJ config_glgsg
#define GLCAT glgsg_cat
#define EXPCL_GL EXPCL_PANDAGL
#define EXPTP_GL EXPTP_PANDAGL


#ifdef WIN32_VC
// Must include windows.h before gl.h on NT
#include <windows.h>
#endif

#ifdef  IS_OSX
#define __glext_h_
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#ifdef HAVE_GLU
#include <GL/glu.h>
#endif
#endif 

#include "glstuff_src.h"

#endif  // GLGSG_H
