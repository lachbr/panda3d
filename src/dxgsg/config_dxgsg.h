// Filename: config_dxgsg.h
// Created by:  drose (06Oct99)
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

#ifndef CONFIG_DXGSG_H
#define CONFIG_DXGSG_H

#include <pandabase.h>
#include <notifyCategoryProxy.h>

NotifyCategoryDecl(dxgsg, EXPCL_PANDADX, EXPTP_PANDADX);

extern bool dx_full_screen;
extern bool dx_sync_video;
extern bool dx_cull_traversal;
extern bool dx_show_fps_meter;
extern bool dx_no_vertex_fog;
extern bool dx_full_screen_antialiasing;
extern float dx_fps_meter_update_interval;
extern bool dx_auto_normalize_lighting;
extern bool dx_use_rangebased_fog;
extern bool dx_allow_software_renderer;
extern bool dx_force_software_renderer;
extern const bool link_tristrips;

// debug flags we might want to use in full optimized build
extern bool dx_ignore_mipmaps;
extern bool dx_mipmap_everything;
extern bool dx_show_transforms;
extern bool dx_force_16bpptextures;
extern bool dx_no_dithering;

#ifndef NDEBUG
extern int dx_force_backface_culling;
#endif

#ifdef _DEBUG
extern float dx_global_miplevel_bias;
extern bool dx_debug_view_mipmaps;
extern bool dx_force_anisotropic_filtering;
#endif

// Ways to implement decals.
enum DXDecalType {
  GDT_mask,   // GL 1.0 style, involving three steps and double-draw of polygon
  GDT_blend,  // As above, but slower; use blending to disable colorbuffer writes
  GDT_offset  // The fastest, using GL 1.1 style glPolygonOffset
};
extern DXDecalType dx_decal_type;

extern EXPCL_PANDADX void init_libdxgsg();

#endif
