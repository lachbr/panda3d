// Filename: glmisc_src.cxx
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

#include "pandaSystem.h"

ConfigVariableBool CLP(cheap_textures)
  ("gl-cheap-textures", false,
   PRC_DESC("Configure this true to GLP(Hint) the textures into the cheapest "
            "possible mode."));

ConfigVariableBool CLP(ignore_clamp)
  ("gl-ignore-clamp", false,
   PRC_DESC("Configure this true to disable texture clamp mode (all textures "
            "repeat, a little cheaper for software renderers)."));

ConfigVariableBool CLP(support_clamp_to_border)
  ("gl-support-clamp-to-border", true,
   PRC_DESC("Configure this true to enable the use of the clamp_to_border "
            "extension if the GL claims to support it, or false not to "
            "use it even if it appears to be available.  (On some OpenGL "
            "drivers, enabling this mode can force software rendering.)"));

ConfigVariableBool CLP(support_rescale_normal)
  ("gl-support-rescale-normal", true,
   PRC_DESC("Configure this true to enable the use of the rescale_normal "
            "extension if the GL claims to support it, or false not to use "
            "it even if it appears to be available.  (This appears to be "
            "buggy on some drivers.)"));

ConfigVariableBool CLP(ignore_filters)
  ("gl-ignore-filters", false,
   PRC_DESC("Configure this true to disable any texture filters at all (forcing "
            "point sampling)."));

ConfigVariableBool CLP(ignore_mipmaps)
  ("gl-ignore-mipmaps", false,
   PRC_DESC("Configure this true to disable mipmapping only."));

ConfigVariableBool CLP(force_mipmaps)
  ("gl-force-mipmaps", false,
   PRC_DESC("Configure this true to enable full trilinear mipmapping on every "
            "texture, whether it asks for it or not."));

ConfigVariableBool CLP(color_mask)
  ("gl-color-mask", true,
   PRC_DESC("Configure this false if your GL's implementation of glColorMask() "
            "is broken (some are).  This will force the use of a (presumably) "
            "more expensive blending operation instead."));

ConfigVariableBool CLP(compile_and_execute)
  ("gl-compile-and-execute", false,
   PRC_DESC("Configure this true if you know your GL's implementation of "
            "glNewList(n, GL_COMPILE_AND_EXECUTE) works.  It is "
            "false by default, since it is known to cause a crash with "
            "Intel 855GM driver 4.14.10.3889 at least.  Turning this on "
            "*may* reduce the chug you get for preparing display lists "
            "for the first time, by allowing the display list to be "
            "rendered at the same time it is being compiled."));

void CLP(init_classes)() {
  CLP(GraphicsStateGuardian)::init_type();
  CLP(TextureContext)::init_type();
  CLP(GeomContext)::init_type();
  CLP(VertexBufferContext)::init_type();
  CLP(IndexBufferContext)::init_type();
  CLP(GeomMunger)::init_type();
  CLP(ShaderContext)::init_type();

  PandaSystem *ps = PandaSystem::get_global_ptr();
  ps->add_system(GLSYSTEM_NAME);

  // We can't add any tags defining the available OpenGL capabilities,
  // since we won't know those until we create a graphics context (and
  // the answer may be different for different contexts).
}

