// Filename: config_egg2sg.cxx
// Created by:  drose (01Oct99)
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

#include "config_egg2sg.h"
#include "loaderFileTypeEgg.h"

#include <dconfig.h>
#include <loaderFileTypeRegistry.h>
#include <get_config_path.h>

ConfigureDef(config_egg2sg);
NotifyCategoryDef(egg2sg, "");

bool egg_mesh = config_egg2sg.GetBool("egg-mesh", true);
bool egg_retesselate_coplanar = config_egg2sg.GetBool("egg-retesselate-coplanar", true);
bool egg_unroll_fans = config_egg2sg.GetBool("egg-unroll-fans", true);
bool egg_show_tstrips = config_egg2sg.GetBool("egg-show-tstrips", false);
bool egg_show_qsheets = config_egg2sg.GetBool("egg-show-qsheets", false);
bool egg_show_quads = config_egg2sg.GetBool("egg-show-quads", false);
bool egg_false_color = (egg_show_tstrips | egg_show_qsheets | egg_show_quads);
bool egg_show_normals = config_egg2sg.GetBool("egg-show-normals", false);
double egg_normal_scale = config_egg2sg.GetDouble("egg-normal-scale", 1.0);
bool egg_subdivide_polys = config_egg2sg.GetBool("egg-subdivide-polys", true);
bool egg_consider_fans = config_egg2sg.GetBool("egg-consider-fans", true);
double egg_max_tfan_angle = config_egg2sg.GetDouble("egg-max-tfan-angle", 40.0);
int egg_min_tfan_tris = config_egg2sg.GetInt("egg-min-tfan-tris", 4);
double egg_coplanar_threshold = config_egg2sg.GetDouble("egg-coplanar-threshold", 0.01);
bool egg_ignore_mipmaps = config_egg2sg.GetBool("egg-ignore-mipmaps", false);
bool egg_ignore_filters = config_egg2sg.GetBool("egg-ignore-filters", false);
bool egg_ignore_clamp = config_egg2sg.GetBool("egg-ignore-clamp", false);
bool egg_always_decal_textures = config_egg2sg.GetBool("egg-always-decal-textures", false);
bool egg_ignore_decals = config_egg2sg.GetBool("egg-ignore-decals", false);
bool egg_flatten = config_egg2sg.GetBool("egg-flatten", true);

// It is almost always a bad idea to set this true.
bool egg_flatten_siblings = config_egg2sg.GetBool("egg-flatten-siblings", false);

bool egg_show_collision_solids = config_egg2sg.GetBool("egg-show-collision-solids", false);

// When this is true, keep texture pathnames exactly the same as they
// appeared in the egg file, in particular leaving them as relative
// paths, rather than letting them reflect the full path at which they
// were found.  This is particularly useful when generating bam files.
// However, if the same texture is named by two different relative
// paths, these will still be collapsed into one texture (using one of
// the relative paths, chosen arbitrarily).
bool egg_keep_texture_pathnames = config_egg2sg.GetBool("egg-keep-texture-pathnames", false);

// When this is true, a <NurbsCurve> entry appearing in an egg file
// will load a ClassicNurbsCurve object instead of the default, a
// NurbsCurve object.  This only makes a difference when the NURBS++
// library is available, in which case the default, NurbsCurve, is
// actually a NurbsPPCurve object.
bool egg_load_classic_nurbs_curves = config_egg2sg.GetBool("egg-load-classic-nurbs-curves", false);

// When this is true, certain kinds of recoverable errors (not syntax
// errors) in an egg file will be allowed and ignored when an egg file
// is loaded.  When it is false, only perfectly pristine egg files may
// be loaded.
bool egg_accept_errors = config_egg2sg.GetBool("egg-accept-errors", true);

CoordinateSystem egg_coordinate_system;

ConfigureFn(config_egg2sg) {
  init_libegg2sg();
}

////////////////////////////////////////////////////////////////////
//     Function: init_libegg2sg
//  Description: Initializes the library.  This must be called at
//               least once before any of the functions or classes in
//               this library can be used.  Normally it will be
//               called by the static initializers and need not be
//               called explicitly, but special cases exist.
////////////////////////////////////////////////////////////////////
void
init_libegg2sg() {
  static bool initialized = false;
  if (initialized) {
    return;
  }
  initialized = true;

  LoaderFileTypeEgg::init_type();

  string csstr = config_egg2sg.GetString("egg-coordinate-system", "default");
  CoordinateSystem cs = parse_coordinate_system_string(csstr);

  if (cs == CS_invalid) {
    egg2sg_cat.error()
      << "Unexpected egg-coordinate-system string: " << csstr << "\n";
    cs = CS_default;
  }
  egg_coordinate_system = (cs == CS_default) ?
    default_coordinate_system : cs;

  LoaderFileTypeRegistry *reg = LoaderFileTypeRegistry::get_ptr();

  reg->register_type(new LoaderFileTypeEgg);
}
