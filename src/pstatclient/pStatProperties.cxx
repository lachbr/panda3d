// Filename: pStatProperties.cxx
// Created by:  drose (17May01)
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

#include "pStatProperties.h"
#include "pStatCollectorDef.h"
#include "pStatClient.h"
#include "config_pstats.h"

#include <ctype.h>

static const int current_pstat_major_version = 2;
static const int current_pstat_minor_version = 1;
// Initialized at 2.0 on 5/18/01, when version numbers were first added.
// Incremented to 2.1 on 5/21/01 to add support for TCP frame data.

////////////////////////////////////////////////////////////////////
//     Function: get_current_pstat_major_version
//  Description: Returns the current major version number of the
//               PStats protocol.  This is the version number that
//               will be reported by clients running this code, and
//               that will be expected by servers running this code.
//
//               The major version numbers must match exactly in order
//               for a communication to be successful.
////////////////////////////////////////////////////////////////////
int
get_current_pstat_major_version() {
  return current_pstat_major_version;
}

////////////////////////////////////////////////////////////////////
//     Function: get_current_pstat_minor_version
//  Description: Returns the current minor version number of the
//               PStats protocol.  This is the version number that
//               will be reported by clients running this code, and
//               that will be expected by servers running this code.
//
//               The minor version numbers need not match exactly, but
//               the server must be >= the client.
////////////////////////////////////////////////////////////////////
int
get_current_pstat_minor_version() {
  return current_pstat_minor_version;
}


#ifdef DO_PSTATS

////////////////////////////////////////////////////////////////////
//
// The rest of this file defines the predefined properties (color,
// sort, etc.) for the various PStatCollectors that may be defined
// within Panda or even elsewhere.
//
// It is a little strange to defined these properties here instead of
// where the collectors are actually declared, but it's handy to have
// them all in one place, so we can easily see which colors are
// available, etc.  It also makes the declarations a lot simpler,
// since there are quite a few esoteric parameters to specify.
//
// We could define these in some external data file that is read in at
// runtime, so that you could extend this list without having to
// relink panda, but then there are the usual problems with ensuring
// that the file is available to you at runtime.  The heck with it.
//
// At least, no other file depends on this file, so it may be modified
// without forcing anything else to be recompiled.
//
////////////////////////////////////////////////////////////////////

struct ColorDef {
  float r, g, b;
};

struct TimeCollectorProperties {
  bool is_active;
  const char *name;
  ColorDef color;
  float suggested_scale;
};

struct LevelCollectorProperties {
  bool is_active;
  const char *name;
  ColorDef color;
  const char *units;
  float suggested_scale;
  float inv_factor;
};

static TimeCollectorProperties time_properties[] = {
  { 1, "App",                              { 0.0, 1.0, 1.0 },  1.0 / 30.0 },
  { 1, "App:Animation",                    { 1.0, 0.0, 1.0 } },
  { 1, "App:Collisions",                   { 1.0, 0.5, 0.0 } },
  { 0, "App:Data graph",                   { 0.5, 0.8, 0.4 } },
  { 1, "App:Show code",                    { 0.8, 0.2, 1.0 } },
  { 1, "Cull",                             { 0.0, 1.0, 0.0 },  1.0 / 30.0 },
  { 0, "Cull:Traverse",                    { 0.0, 1.0, 1.0 } },
  { 0, "Cull:Geom node",                   { 1.0, 0.0, 1.0 } },
  { 0, "Cull:Direct node",                 { 1.0, 0.5, 0.0 } },
  { 0, "Cull:Apply initial",               { 0.2, 1.0, 0.8 } },
  { 0, "Cull:Draw",                        { 1.0, 1.0, 0.0 } },
  { 0, "Cull:Clean",                       { 0.0, 0.0, 1.0 } },
  { 0, "Cull:Bins",                        { 0.8, 1.0, 0.8 } },
  { 0, "Cull:Bins:BTF",                    { 1.0, 0.5, 0.5 } },
  { 0, "Cull:Bins:Unsorted",               { 0.5, 0.5, 1.0 } },
  { 0, "Cull:Bins:Fixed",                  { 0.5, 1.0, 0.5 } },
  { 1, "Draw",                             { 1.0, 0.0, 0.0 },  1.0 / 30.0 },
  { 1, "Draw:Swap buffers",                { 0.5, 1.0, 0.8 } },
  { 1, "Draw:Clear",                       { 0.5, 0.7, 0.7 } },
  { 1, "Draw:Show fps",                    { 0.5, 0.8, 1.0 } },
  { 1, "Draw:Make current",                { 1.0, 0.6, 0.3 } },
  { 0, "WRT",                              { 0.0, 0.0, 1.0 } },
  { 0, "WRT:Subtree",                      { 0.3, 1.0, 0.3 } },
  { 0, NULL }
};

static LevelCollectorProperties level_properties[] = {
  { 1, "Texture usage",                    { 1.0, 0.0, 0.5 },  "MB", 12, 1048576 },
  { 1, "Texture usage:Active",             { 0.5, 1.0, 0.8 } },
  { 1, "Texture memory",                   { 0.0, 0.0, 1.0 },  "MB", 12, 1048576 },
  { 1, "Texture memory:In use",            { 0.0, 1.0, 1.0 } },
  { 1, "Texture manager",                  { 1.0, 0.0, 0.0 },  "MB", 12, 1048576 },
  { 1, "Texture manager:Resident",         { 1.0, 1.0, 0.0 } },
  { 1, "Vertices",                         { 0.5, 0.2, 0.0 },  "K", 10, 1000 },
  { 1, "Vertices:Other",                   { 0.2, 0.2, 0.2 } },
  { 1, "Vertices:Triangles",               { 0.8, 0.8, 0.8 } },
  { 1, "Vertices:Triangle fans",           { 0.8, 0.5, 0.2 } },
  { 1, "Vertices:Triangle strips",         { 0.2, 0.5, 0.8 } },
  { 1, "Nodes",                            { 0.4, 0.2, 0.8 },  "", 500.0 },
  { 1, "Nodes:GeomNodes",                  { 0.8, 0.2, 0.0 } },
  { 1, "State changes",                    { 1.0, 0.5, 0.2 },  "", 500.0 },
  { 1, "State changes:Transforms",         { 0.2, 0.2, 0.8 }, },
  { 1, "State changes:Textures",           { 0.8, 0.2, 0.2 }, },
  { 1, "Panda memory usage",               { 0.8, 0.2, 0.5 },  "MB", 64, 1048576 },
  { 0, NULL }
};


////////////////////////////////////////////////////////////////////
//     Function: initialize_collector_def_from_table
//  Description: Looks up the collector in the compiled-in table
//               defined above, and sets its properties appropriately
//               if it is found.
////////////////////////////////////////////////////////////////////
static void
initialize_collector_def_from_table(const string &fullname, PStatCollectorDef *def) {
  int i;

  for (i = 0;
       time_properties[i].name != (const char *)NULL;
       i++) {
    const TimeCollectorProperties &tp = time_properties[i];
    if (fullname == tp.name) {
      def->_sort = i;
      if (!def->_active_explicitly_set) {
        def->_is_active = tp.is_active;
      }
      def->_suggested_color.set(tp.color.r, tp.color.g, tp.color.b);
      if (tp.suggested_scale != 0.0) {
        def->_suggested_scale = tp.suggested_scale;
      }
      return;
    }
  }

  for (i = 0;
       level_properties[i].name != (const char *)NULL;
       i++) {
    const LevelCollectorProperties &lp = level_properties[i];
    if (fullname == lp.name) {
      def->_sort = i;
      if (!def->_active_explicitly_set) {
        def->_is_active = lp.is_active;
      }
      def->_suggested_color.set(lp.color.r, lp.color.g, lp.color.b);
      if (lp.suggested_scale != 0.0) {
        def->_suggested_scale = lp.suggested_scale;
      }
      if (lp.units != (const char *)NULL) {
        def->_level_units = lp.units;
      }
      if (lp.inv_factor != 0.0) {
        def->_factor = 1.0 / lp.inv_factor;
      }
      return;
    }
  }
}


////////////////////////////////////////////////////////////////////
//     Function: initialize_collector_def
//  Description: This is the only accessor function into this table.
//               The PStatCollectorDef constructor calls it when a new
//               PStatCollectorDef is created.  It should look up in
//               the table and find a matching definition for this def
//               by name; if one is found, the properties are applied.
////////////////////////////////////////////////////////////////////
void
initialize_collector_def(PStatClient *client, PStatCollectorDef *def) {
  string fullname;

  if (def->_index == 0) {
    fullname = def->_name;
  } else {
    fullname = client->get_collector_fullname(def->_index);
  }

  // First, check the compiled-in defaults.
  initialize_collector_def_from_table(fullname, def);

  // Then, look to Config for more advice.  To do this, we first
  // change the name to something more like a Config variable name.
  // We replace colons and spaces with hyphens, eliminate other
  // punctuation, and make all letters lowercase.

  string config_name;
  string::const_iterator ni;
  for (ni = fullname.begin(); ni != fullname.end(); ++ni) {
    switch (*ni) {
    case ':':
    case ' ':
    case '\n':
    case '\r':
    case '\t':
      config_name += '-';
      break;

    default:
      if (isalnum(*ni)) {
        config_name += tolower(*ni);
      }
    }
  }

  if (!config_pstats.GetString("pstats-active-" + config_name, "").empty()) {
    def->_is_active =
      config_pstats.GetBool("pstats-active-" + config_name, true);
    def->_active_explicitly_set = true;
  }

  def->_sort =
    config_pstats.GetInt("pstats-sort-" + config_name, def->_sort);
  def->_suggested_scale =
    config_pstats.GetFloat("pstats-scale-" + config_name, def->_suggested_scale);
  def->_level_units =
    config_pstats.GetString("pstats-units-" + config_name, def->_level_units);
  def->_level_units =
    config_pstats.GetString("pstats-units-" + config_name, def->_level_units);
  if (!config_pstats.GetString("pstats-factor-" + config_name, "").empty()) {
    def->_factor =
      1.0/config_pstats.GetFloat("pstats-factor-" + config_name, 1.0);
  }

  // Get and decode the color string.  We allow any three
  // floating-point numbers, with any kind of non-digit characters
  // between them.
  string color_str =
    config_pstats.GetString("pstats-color-" + config_name, "");
  if (!color_str.empty()) {
    const char *cstr = color_str.c_str();
    const char *p = cstr;

    int i = 0;
    while (i < 3 && *p != '\0') {
      while (*p != '\0' && !(isdigit(*p) || *p == '.')) {
        p++;
      }
      char *q;
      def->_suggested_color[i] = strtod(p, &q);
      p = q;
      i++;
    }
  }
}

#endif // DO_PSTATS
