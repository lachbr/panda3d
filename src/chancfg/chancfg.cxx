// Filename: chancfg.cxx
// Created by:  cary (02Feb99)
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

#include "chancfg.h"
#include "notify.h"
#include "displayRegion.h"
#include "graphicsChannel.h"
#include "hardwareChannel.h"
#include "camera.h"
#include "qpcamera.h"
#include "frustum.h"
#include "perspectiveLens.h"
#include "renderRelation.h"
#include "transformTransition.h"
#include "dSearchPath.h"
#include "dconfig.h"
#include "filename.h"
#include "pt_NamedNode.h"
#include "transformState.h"

#include <algorithm>


Configure(chanconfig);

ConfigureFn(chanconfig) {
}

static bool have_read = false;
NotifyCategoryDef(chancfg, "");

static void ReadChanConfigData(void) {
  if (have_read)
    return;

  ResetLayout();
  ResetSetup();
  ResetWindow();

  DSearchPath path(chanconfig.GetString("ETC_PATH", "."));

  string layoutdbfilename = chanconfig.GetString("layout-db-file","layout_db");
  string windowdbfilename = chanconfig.GetString("window-db-file","window_db");
  string setupdbfilename = chanconfig.GetString("setup-db-file","setup_db");

  Filename layoutfname = Filename::text_filename(layoutdbfilename);
  if (!layoutfname.resolve_filename(path)) {
    chancfg_cat->error() << "Could not find layoutdb file " << layoutfname << " in path " << path << "\n";
    exit(1);
  } else {
    ifstream ifs;
    if (layoutfname.open_read(ifs)) {
      if (chancfg_cat.is_debug())
        chancfg_cat->debug()
          << "Reading layout database " << layoutfname << endl;
      ParseLayout(ifs);
    } else {
      chancfg_cat->error()
        << "Unable to read layout database " << layoutfname << "\n";
      exit(1);
    }
  }

  Filename setupfname = Filename::text_filename(setupdbfilename);
  if (!setupfname.resolve_filename(path)) {
    chancfg_cat->error() << "Could not find setupdb file " << setupfname << " in path " << path << "\n"; 
    exit(1);
  } else {
    ifstream ifs;
    if (setupfname.open_read(ifs)) {
      if (chancfg_cat.is_debug())
        chancfg_cat->debug()
          << "Reading setup database " << setupfname << endl;
      ParseSetup(ifs);
    } else {
      chancfg_cat->error()
        << "Unable to read setup database " << setupfname << "\n";
      exit(1);
    }
  }

  Filename windowfname = Filename::text_filename(windowdbfilename);
  if (!windowfname.resolve_filename(path)) {
    chancfg_cat->error() << "Could not find windowdb file " << setupfname << " in path " << path << "\n"; 
    exit(1);
  } else {
    ifstream ifs;
    if (windowfname.open_read(ifs)) {
      if (chancfg_cat.is_debug())
        chancfg_cat->debug()
          << "Reading window database " << windowfname << endl;
      ParseWindow(ifs);
    } else {
      chancfg_cat->error()
        << "Unable to read window database " << windowfname << "\n";
      exit(1);
    }
  }
}

static const bool config_sanity_check =
  chanconfig.GetBool("chan-config-sanity-check", false);

ChanCfgOverrides ChanOverrideNone;

INLINE bool LayoutDefined(std::string sym) {
  return (LayoutDB != (LayoutType *)NULL) &&
    (LayoutDB->find(sym) != LayoutDB->end());
}

INLINE bool SetupDefined(std::string sym) {
  return (SetupDB != (SetupType *)NULL) &&
    (SetupDB->find(sym) != SetupDB->end());
}

INLINE bool ConfigDefined(std::string sym) {
  return (WindowDB != (WindowType *)NULL) &&
    (WindowDB->find(sym) != WindowDB->end());
}

bool ChanCheckLayouts(SetupSyms& S) {
  if (S.empty())
    return false;
  for (SetupSyms::iterator i=S.begin(); i!=S.end(); ++i)
    if (!LayoutDefined(*i)) {
      chancfg_cat.error() << "no layout called '" << *i << "'" << endl;
      return false;
    }
  return true;
}

bool ChanCheckSetups(SetupSyms& S) {
  if (S.empty())
    return false;
  for (SetupSyms::iterator i=S.begin(); i!=S.end(); ++i) {
    if (!SetupDefined(*i)) {
      chancfg_cat.error() << "no setup called '" << *i << "'" << endl;
      return false;
    }
  }
  return true;
}

INLINE ChanViewport ChanScaleViewport(const ChanViewport& parent,
                      const ChanViewport& child) {
  float dx = (parent.right() - parent.left());
  float dy = (parent.top() - parent.bottom());
  ChanViewport v(parent.left() + (dx * child.left()),
         parent.left() + (dx * child.right()),
         parent.bottom() + (dy * child.bottom()),
         parent.bottom() + (dy * child.top()));
  return v;
}

/*
int ChanFindNextHWChan(int offset, const HardwareChannel::HWChanMap& hw_chans) {
  int i = offset;
  while (hw_chans.find(i) != hw_chans.end())
    ++i;
  return i;
}
*/

SetupFOV ChanResolveFOV(SetupFOV& fov, float sizeX, float sizeY) {
  float horiz = 45.0f;
  float vert;
  SetupFOV ret;
  switch (fov.getType()) {
      case SetupFOV::Horizontal:
        horiz = fov.getHoriz();
        if (chancfg_cat.is_debug())
          chancfg_cat->debug() << "ChanResolveFOV:: setting default horiz = "
                   << horiz << endl;
      case SetupFOV::Default:
        horiz = chanconfig.GetFloat("fov", horiz);
        vert = 2.0f*rad_2_deg(atan((sizeY/sizeX)*tan(0.5f*deg_2_rad(horiz))));
        if (chancfg_cat.is_debug())
          chancfg_cat->debug() << "ChanResolveFOV:: setting horiz = " << horiz
                   << " and vert = " << vert << endl;
        break;
      case SetupFOV::Both:
        horiz = fov.getHoriz();
        vert = fov.getVert();
        if (chancfg_cat.is_debug())
          chancfg_cat->debug() << "ChanResolveFOV:: setting horiz = " << horiz
                   << " and vert = " << vert << endl;
        break;
    
      default:
        break;
  }

  ret.setFOV(horiz, vert);
  return ret;
}

void ChanConfig::chan_eval(GraphicsWindow* win, WindowItem& W, LayoutItem& L, 
                           SVec& S,
                           ChanViewport& V, int hw_offset, int xsize, int ysize,
                           Node *render, bool want_cameras) {
  int i = min(L.GetNumRegions(), int(S.size()));
  int j;
  SVec::iterator k;
  std::vector<PT_NamedNode>camera(W.getNumCameraGroups());
  //first camera is special cased to name "camera" for older code
  camera[0] = new NamedNode("camera");
  for(int icam=1;icam<W.getNumCameraGroups();icam++) {
    char dummy[10];//if more than 10^11 groups, you've got bigger problems
    sprintf(dummy,"%d",icam);
    std::string nodeName = "camera";
    nodeName.append(dummy);
    camera[icam] = new NamedNode(nodeName.c_str());
  }
  for (j=0, k=S.begin(); j<i; ++j, ++k) {
   ChanViewport v(ChanScaleViewport(V, L[j]));
   PT(GraphicsChannel) chan;
   if ((*k).getHWChan() && W.getHWChans()) {
     if ((*k).getChan() == -1) {
       chan = win->get_channel(hw_offset);
     } else
       chan = win->get_channel((*k).getChan());
       // HW channels always start with the full area of the channel
       v = ChanViewport(0.0f, 1.0f, 0.0f, 1.0f);
   } else {
     chan = win->get_channel(0);
   }
   ChanViewport v2(ChanScaleViewport(v, (*k).getViewport()));
   PT(GraphicsLayer) layer = chan->make_layer();
   PT(DisplayRegion) dr = 
     layer->make_display_region(v2.left(), v2.right(),
                                v2.bottom(), v2.top());
   if (want_cameras && camera[0] != (Node *)NULL) {
     // now make a camera for it
     PT(Camera) cam = new Camera;
     dr->set_camera(cam);
     _display_region.push_back(dr);
     SetupFOV fov = (*k).getFOV();
     // The distinction between ConsoleSize and DisplaySize
     // is to handle display regions with orientations that
     // are rotated 90 degrees left or right.  For example,
     // the model shop cave (LAIR) uses projectors on their
     // sides, so that what's horizontal on the console is 
     // vertical in the cave (Display).
     float xConsoleSize = xsize*(v2.right()-v2.left());
     float yConsoleSize = ysize*(v2.top()-v2.bottom());
     float xDisplaySize, yDisplaySize;
     if ( (*k).getOrientation() == SetupItem::Left ||
          (*k).getOrientation() == SetupItem::Right ) {
        xDisplaySize = yConsoleSize;
        yDisplaySize = xConsoleSize;
     } else {
        xDisplaySize = xConsoleSize;
        yDisplaySize = yConsoleSize;
     }
     fov = ChanResolveFOV(fov, xDisplaySize, yDisplaySize);
     if (chancfg_cat->is_debug()) {
       chancfg_cat->debug() << "ChanEval:: FOVhoriz = " << fov.getHoriz()
          << "  FOVvert = " << fov.getVert() << endl;
       chancfg_cat->debug() << "ChanEval:: xsize = " << xsize
          << "  ysize = " << ysize << endl;
     }

     // take care of the orientation
     PT(TransformTransition) orient;
     float hFov, vFov;
   
     switch ((*k).getOrientation()) {
       case SetupItem::Up:
         hFov = fov.getHoriz(); vFov = fov.getVert();
         break;
       case SetupItem::Down:
         hFov = fov.getHoriz(); vFov = fov.getVert();
         orient = new TransformTransition(
           LMatrix4f::rotate_mat_normaxis(180.0f, LVector3f::forward()));
         break;
       case SetupItem::Left:
         // vertical and horizontal FOV are being switched
         hFov = fov.getVert(); vFov = fov.getHoriz();
         orient = new TransformTransition(
           LMatrix4f::rotate_mat_normaxis(90.0f, LVector3f::forward()));
         break;
       case SetupItem::Right:
         // vertical and horizontal FOV are being switched
         hFov = fov.getVert(); vFov = fov.getHoriz();
         orient = new TransformTransition(
           LMatrix4f::rotate_mat_normaxis(-90.0f, LVector3f::forward()));
         break;
     }

     PT(Lens) lens = new PerspectiveLens;
     lens->set_fov(hFov, vFov);
     lens->set_near_far(1.0f, 10000.0f);
     cam->set_lens(lens);

     // hfov and vfov for camera are switched from what was specified
     // if the orientation is sideways.
     if (chancfg_cat->is_debug())
       chancfg_cat->debug() << "ChanEval:: camera hfov = "
         << lens->get_hfov() << "  vfov = "
         << lens->get_vfov() << endl;
     cam->set_scene(render);

     RenderRelation *tocam = new RenderRelation(camera[W.getCameraGroup(j)], cam);
     if (orient != (TransformTransition *)NULL) {
       tocam->set_transition(orient);
     }
   }
  }
  _group_node = camera;
  return;
}

ChanConfig::ChanConfig(GraphicsPipe* pipe, std::string cfg, Node *render,
                       ChanCfgOverrides& overrides) {
  ReadChanConfigData();
  // check to make sure we know everything we need to
  if (!ConfigDefined(cfg)) {
    chancfg_cat.error()
      << "no window configuration called '" << cfg << "'" << endl;
    _graphics_window = (GraphicsWindow*)0;
    return;
  }
  WindowItem W = (*WindowDB)[cfg];

  std::string l = W.getLayout();
  if (!LayoutDefined(l)) {
    chancfg_cat.error()
      << "No layout called '" << l << "'" << endl;
    _graphics_window = (GraphicsWindow*)0;
    return;
  }
  LayoutItem L = (*LayoutDB)[l];

  SetupSyms s = W.getSetups();
  if (!ChanCheckSetups(s)) {
    chancfg_cat.error() << "Setup failure" << endl;
    _graphics_window = (GraphicsWindow*)0;
    return;
  }

  SVec S;
  S.reserve(s.size());
  for (SetupSyms::iterator i=s.begin(); i!=s.end(); ++i)
    S.push_back((*SetupDB)[(*i)]);

  // get the window data
  int sizeX = chanconfig.GetInt("win-width", -1);
  int sizeY = chanconfig.GetInt("win-height", -1);
  if (overrides.defined(ChanCfgOverrides::SizeX))
    sizeX = overrides.getInt(ChanCfgOverrides::SizeX);
  if (overrides.defined(ChanCfgOverrides::SizeY))
    sizeY = overrides.getInt(ChanCfgOverrides::SizeY);
  if (sizeX < 0) {
    if (sizeY < 0) {
      if(chancfg_cat.is_debug())
          chancfg_cat.debug() << "Using default chan-window size\n";
      // take the default size
      sizeX = W.getSizeX();
      sizeY = W.getSizeY();
    } else {
      // vertical size is defined, compute horizontal keeping the aspect from
      // the default
      sizeX = (W.getSizeX() * sizeY) / W.getSizeY();
    }
  } else if (sizeY < 0) {
    // horizontal size is defined, compute vertical keeping the aspect from the
    // default
    sizeY = (W.getSizeY() * sizeX) / W.getSizeX();
  }

  int origX = chanconfig.GetInt("win-origin-x");
  int origY = chanconfig.GetInt("win-origin-y");
  origX = overrides.defined(ChanCfgOverrides::OrigX) ?
            overrides.getInt(ChanCfgOverrides::OrigX) : origX;
  origY = overrides.defined(ChanCfgOverrides::OrigY) ?
            overrides.getInt(ChanCfgOverrides::OrigY) : origY;

  bool border = !chanconfig.GetBool("no-border", !W.getBorder());
  bool fullscreen = chanconfig.GetBool("fullscreen", false);
  bool use_cursor = chanconfig.GetBool("cursor-visible", W.getCursor());
  int want_depth_bits = chanconfig.GetInt("want-depth-bits", 1);
  int want_color_bits = chanconfig.GetInt("want-color-bits", 1);

  // visual?  nope, that's handled with the mode.
  uint mask = 0x0;  // ?!  this really should come from the win config
  mask = overrides.defined(ChanCfgOverrides::Mask) ?
           overrides.getUInt(ChanCfgOverrides::Mask) : mask;
  std::string title = cfg;
  title = overrides.defined(ChanCfgOverrides::Title) ?
            overrides.getString(ChanCfgOverrides::Title) : title;

  GraphicsWindow::Properties props;
  props._xorg = origX;
  props._yorg = origY;
  props._xsize = sizeX;
  props._ysize = sizeY;
  props._title = title;
  props._mask = mask;
  props._border = border;
  props._fullscreen = fullscreen;
  props._want_depth_bits = want_depth_bits;
  props._want_color_bits = want_color_bits;
  props._bCursorIsVisible = use_cursor;

  // stereo prep?
  // DVR prep?

  bool want_cameras = overrides.defined(ChanCfgOverrides::Cameras) ?
                        overrides.getBool(ChanCfgOverrides::Cameras) : true;

  // open that sucker
  PT(GraphicsWindow) win = pipe->make_window(props);
  if(win == (GraphicsWindow *)NULL) {
    chancfg_cat.error() << "Could not create window" << endl;
    _graphics_window = (GraphicsWindow *)NULL;
    return;
  }

  // make channels and display regions
  ChanViewport V(0.0f, 1.0f, 0.0f, 1.0f);
  chan_eval(win, W, L, S, V, W.getChanOffset()+1, sizeX, sizeY, 
            render, want_cameras);
  for(size_t dr_index=0; dr_index<_display_region.size(); dr_index++)
    _group_membership.push_back(W.getCameraGroup(dr_index));

  // sanity check
  if (config_sanity_check) {
    nout << "ChanConfig Sanity check:" << endl
         << "window - " << (void*)win << endl
         << "  width = " << win->get_width() << "  height = " << win->get_height() << endl
         << "  xorig = " << win->get_xorg() << "  yorig = " << win->get_yorg()<< endl;

    int max_channel_index = win->get_max_channel_index();
    for (int c = 0; c < max_channel_index; c++) {
        if (win->is_channel_defined(c)) {
          GraphicsChannel *chan = win->get_channel(c);
          nout << "  Chan - " << (void*)chan << endl
               << "    window = " << (void*)(chan->get_window()) << endl
               << "    active = " << chan->is_active() << endl;
        
          int num_layers = chan->get_num_layers();
          for (int l = 0; l < num_layers; l++) {
            GraphicsLayer *layer = chan->get_layer(l);
            nout << "    Layer - " << (void*)layer << endl
                 << "      channel = " << (void*)(layer->get_channel()) << endl
                 << "      active = " << layer->is_active() << endl;
        
            int num_drs = layer->get_num_drs();
            for (int d = 0; d < num_drs; d++) {
              DisplayRegion *dr = layer->get_dr(d);
              nout << "      DR - " << (void*)dr << endl
                   << "        layer = " << (void*)(dr->get_layer()) << endl;
              float ll, rr, bb, tt;
              dr->get_dimensions(ll, rr, bb, tt);
              nout << "        (" << ll << " " << rr << " " << bb << " " << tt << ")" << endl
                   << "        camera = " << (void*)(dr->get_camera()) << endl;
              Camera* cmm = dr->get_camera();
              if (cmm != (Camera*)0L) {
                  nout << "          active = " << cmm->is_active() << endl;
                  int num_cam_drs = cmm->get_num_drs();
                  for (int cd = 0; cd < num_cam_drs; cd++) 
                      nout << "          dr = " << (void*)cmm->get_dr(cd) << endl;
              }
              nout << "      active = " << dr->is_active() << endl;
            }
          }
        }
    }
  }
  _graphics_window = win;
  return;
}

void qpChanConfig::chan_eval(GraphicsWindow* win, WindowItem& W, LayoutItem& L, 
                           SVec& S,
                           ChanViewport& V, int hw_offset, int xsize, int ysize,
                           const qpNodePath &render, bool want_cameras) {
  int i = min(L.GetNumRegions(), int(S.size()));
  int j;
  SVec::iterator k;
  std::vector< PT(PandaNode) >camera(W.getNumCameraGroups());
  //first camera is special cased to name "camera" for older code
  camera[0] = new PandaNode("camera");
  for(int icam=1;icam<W.getNumCameraGroups();icam++) {
    char dummy[10];//if more than 10^11 groups, you've got bigger problems
    sprintf(dummy,"%d",icam);
    std::string nodeName = "camera";
    nodeName.append(dummy);
    camera[icam] = new PandaNode(nodeName);
  }
  for (j=0, k=S.begin(); j<i; ++j, ++k) {
   ChanViewport v(ChanScaleViewport(V, L[j]));
   PT(GraphicsChannel) chan;
   if ((*k).getHWChan() && W.getHWChans()) {
     if ((*k).getChan() == -1) {
       chan = win->get_channel(hw_offset);
     } else
       chan = win->get_channel((*k).getChan());
       // HW channels always start with the full area of the channel
       v = ChanViewport(0.0f, 1.0f, 0.0f, 1.0f);
   } else {
     chan = win->get_channel(0);
   }
   ChanViewport v2(ChanScaleViewport(v, (*k).getViewport()));
   PT(GraphicsLayer) layer = chan->make_layer();
   PT(DisplayRegion) dr = 
     layer->make_display_region(v2.left(), v2.right(),
                                v2.bottom(), v2.top());
   if (want_cameras && camera[0] != (PandaNode *)NULL) {
     // now make a camera for it
     PT(qpCamera) cam = new qpCamera("");
     dr->set_qpcamera(qpNodePath(cam));
     _display_region.push_back(dr);
     SetupFOV fov = (*k).getFOV();
     // The distinction between ConsoleSize and DisplaySize
     // is to handle display regions with orientations that
     // are rotated 90 degrees left or right.  For example,
     // the model shop cave (LAIR) uses projectors on their
     // sides, so that what's horizontal on the console is 
     // vertical in the cave (Display).
     float xConsoleSize = xsize*(v2.right()-v2.left());
     float yConsoleSize = ysize*(v2.top()-v2.bottom());
     float xDisplaySize, yDisplaySize;
     if ( (*k).getOrientation() == SetupItem::Left ||
          (*k).getOrientation() == SetupItem::Right ) {
        xDisplaySize = yConsoleSize;
        yDisplaySize = xConsoleSize;
     } else {
        xDisplaySize = xConsoleSize;
        yDisplaySize = yConsoleSize;
     }
     fov = ChanResolveFOV(fov, xDisplaySize, yDisplaySize);
     if (chancfg_cat->is_debug()) {
       chancfg_cat->debug() << "ChanEval:: FOVhoriz = " << fov.getHoriz()
          << "  FOVvert = " << fov.getVert() << endl;
       chancfg_cat->debug() << "ChanEval:: xsize = " << xsize
          << "  ysize = " << ysize << endl;
     }

     // take care of the orientation
     CPT(TransformState) orient;
     float hFov, vFov;
   
     switch ((*k).getOrientation()) {
       case SetupItem::Up:
         hFov = fov.getHoriz(); vFov = fov.getVert();
         break;
       case SetupItem::Down:
         hFov = fov.getHoriz(); vFov = fov.getVert();
         orient = TransformState::make_mat
           (LMatrix4f::rotate_mat_normaxis(180.0f, LVector3f::forward()));
         break;
       case SetupItem::Left:
         // vertical and horizontal FOV are being switched
         hFov = fov.getVert(); vFov = fov.getHoriz();
         orient = TransformState::make_mat
           (LMatrix4f::rotate_mat_normaxis(90.0f, LVector3f::forward()));
         break;
       case SetupItem::Right:
         // vertical and horizontal FOV are being switched
         hFov = fov.getVert(); vFov = fov.getHoriz();
         orient = TransformState::make_mat
           (LMatrix4f::rotate_mat_normaxis(-90.0f, LVector3f::forward()));
         break;
     }

     PT(Lens) lens = new PerspectiveLens;
     lens->set_fov(hFov, vFov);
     lens->set_near_far(1.0f, 10000.0f);
     cam->set_lens(lens);

     // hfov and vfov for camera are switched from what was specified
     // if the orientation is sideways.
     if (chancfg_cat->is_debug())
       chancfg_cat->debug() << "ChanEval:: camera hfov = "
         << lens->get_hfov() << "  vfov = "
         << lens->get_vfov() << endl;
     cam->set_scene(render);

     camera[W.getCameraGroup(j)]->add_child(cam);
     if (orient != (TransformState *)NULL) {
       cam->set_transform(orient);
     }
   }
  }
  _group_node = camera;
  return;
}

qpChanConfig::qpChanConfig(GraphicsPipe* pipe, std::string cfg, const qpNodePath &render,
                       ChanCfgOverrides& overrides) {
  ReadChanConfigData();
  // check to make sure we know everything we need to
  if (!ConfigDefined(cfg)) {
    chancfg_cat.error()
      << "no window configuration called '" << cfg << "'" << endl;
    _graphics_window = (GraphicsWindow*)0;
    return;
  }
  WindowItem W = (*WindowDB)[cfg];

  std::string l = W.getLayout();
  if (!LayoutDefined(l)) {
    chancfg_cat.error()
      << "No layout called '" << l << "'" << endl;
    _graphics_window = (GraphicsWindow*)0;
    return;
  }
  LayoutItem L = (*LayoutDB)[l];

  SetupSyms s = W.getSetups();
  if (!ChanCheckSetups(s)) {
    chancfg_cat.error() << "Setup failure" << endl;
    _graphics_window = (GraphicsWindow*)0;
    return;
  }

  SVec S;
  S.reserve(s.size());
  for (SetupSyms::iterator i=s.begin(); i!=s.end(); ++i)
    S.push_back((*SetupDB)[(*i)]);

  // get the window data
  int sizeX = chanconfig.GetInt("win-width", -1);
  int sizeY = chanconfig.GetInt("win-height", -1);
  if (overrides.defined(ChanCfgOverrides::SizeX))
    sizeX = overrides.getInt(ChanCfgOverrides::SizeX);
  if (overrides.defined(ChanCfgOverrides::SizeY))
    sizeY = overrides.getInt(ChanCfgOverrides::SizeY);
  if (sizeX < 0) {
    if (sizeY < 0) {
      if(chancfg_cat.is_debug())
          chancfg_cat.debug() << "Using default chan-window size\n";
      // take the default size
      sizeX = W.getSizeX();
      sizeY = W.getSizeY();
    } else {
      // vertical size is defined, compute horizontal keeping the aspect from
      // the default
      sizeX = (W.getSizeX() * sizeY) / W.getSizeY();
    }
  } else if (sizeY < 0) {
    // horizontal size is defined, compute vertical keeping the aspect from the
    // default
    sizeY = (W.getSizeY() * sizeX) / W.getSizeX();
  }

  int origX = chanconfig.GetInt("win-origin-x");
  int origY = chanconfig.GetInt("win-origin-y");
  origX = overrides.defined(ChanCfgOverrides::OrigX) ?
            overrides.getInt(ChanCfgOverrides::OrigX) : origX;
  origY = overrides.defined(ChanCfgOverrides::OrigY) ?
            overrides.getInt(ChanCfgOverrides::OrigY) : origY;

  bool border = !chanconfig.GetBool("no-border", !W.getBorder());
  bool fullscreen = chanconfig.GetBool("fullscreen", false);
  bool use_cursor = chanconfig.GetBool("cursor-visible", W.getCursor());
  int want_depth_bits = chanconfig.GetInt("want-depth-bits", 1);
  int want_color_bits = chanconfig.GetInt("want-color-bits", 1);

  // visual?  nope, that's handled with the mode.
  uint mask = 0x0;  // ?!  this really should come from the win config
  mask = overrides.defined(ChanCfgOverrides::Mask) ?
           overrides.getUInt(ChanCfgOverrides::Mask) : mask;
  std::string title = cfg;
  title = overrides.defined(ChanCfgOverrides::Title) ?
            overrides.getString(ChanCfgOverrides::Title) : title;

  GraphicsWindow::Properties props;
  props._xorg = origX;
  props._yorg = origY;
  props._xsize = sizeX;
  props._ysize = sizeY;
  props._title = title;
  props._mask = mask;
  props._border = border;
  props._fullscreen = fullscreen;
  props._want_depth_bits = want_depth_bits;
  props._want_color_bits = want_color_bits;
  props._bCursorIsVisible = use_cursor;

  // stereo prep?
  // DVR prep?

  bool want_cameras = overrides.defined(ChanCfgOverrides::Cameras) ?
                        overrides.getBool(ChanCfgOverrides::Cameras) : true;

  // open that sucker
  PT(GraphicsWindow) win = pipe->make_window(props);
  if(win == (GraphicsWindow *)NULL) {
    chancfg_cat.error() << "Could not create window" << endl;
    _graphics_window = (GraphicsWindow *)NULL;
    return;
  }

  // make channels and display regions
  ChanViewport V(0.0f, 1.0f, 0.0f, 1.0f);
  chan_eval(win, W, L, S, V, W.getChanOffset()+1, sizeX, sizeY, 
            render, want_cameras);
  for(size_t dr_index=0; dr_index<_display_region.size(); dr_index++)
    _group_membership.push_back(W.getCameraGroup(dr_index));

  // sanity check
  if (config_sanity_check) {
    nout << "ChanConfig Sanity check:" << endl
         << "window - " << (void*)win << endl
         << "  width = " << win->get_width() << "  height = " << win->get_height() << endl
         << "  xorig = " << win->get_xorg() << "  yorig = " << win->get_yorg()<< endl;

    int max_channel_index = win->get_max_channel_index();
    for (int c = 0; c < max_channel_index; c++) {
        if (win->is_channel_defined(c)) {
          GraphicsChannel *chan = win->get_channel(c);
          nout << "  Chan - " << (void*)chan << endl
               << "    window = " << (void*)(chan->get_window()) << endl
               << "    active = " << chan->is_active() << endl;
        
          int num_layers = chan->get_num_layers();
          for (int l = 0; l < num_layers; l++) {
            GraphicsLayer *layer = chan->get_layer(l);
            nout << "    Layer - " << (void*)layer << endl
                 << "      channel = " << (void*)(layer->get_channel()) << endl
                 << "      active = " << layer->is_active() << endl;
        
            int num_drs = layer->get_num_drs();
            for (int d = 0; d < num_drs; d++) {
              DisplayRegion *dr = layer->get_dr(d);
              nout << "      DR - " << (void*)dr << endl
                   << "        layer = " << (void*)(dr->get_layer()) << endl;
              float ll, rr, bb, tt;
              dr->get_dimensions(ll, rr, bb, tt);
              nout << "        (" << ll << " " << rr << " " << bb << " " << tt << ")" << endl
                   << "        camera = " << (void*)(dr->get_camera()) << endl;
              Camera* cmm = dr->get_camera();
              if (cmm != (Camera*)0L) {
                  nout << "          active = " << cmm->is_active() << endl;
                  int num_cam_drs = cmm->get_num_drs();
                  for (int cd = 0; cd < num_cam_drs; cd++) 
                      nout << "          dr = " << (void*)cmm->get_dr(cd) << endl;
              }
              nout << "      active = " << dr->is_active() << endl;
            }
          }
        }
    }
  }
  _graphics_window = win;
  return;
}





ChanCfgOverrides::ChanCfgOverrides(void) {}

ChanCfgOverrides::ChanCfgOverrides(const ChanCfgOverrides& c) :
  _fields(c._fields) {}

ChanCfgOverrides::~ChanCfgOverrides(void) {}

ChanCfgOverrides& ChanCfgOverrides::operator=(const ChanCfgOverrides& c)
{
  _fields = c._fields;
  return *this;
}

void ChanCfgOverrides::setField(const Field f, const bool v) {
  Types t;
  t.setBool(v);
  _fields[f] = t;
}

void ChanCfgOverrides::setField(const Field f, const int v) {
  Types t;
  t.setInt(v);
  _fields[f] = t;
}

void ChanCfgOverrides::setField(const Field f, const unsigned int v) {
  Types t;
  t.setUInt(v);
  _fields[f] = t;
}

void ChanCfgOverrides::setField(const Field f, const float v) {
  Types t;
  t.setFloat(v);
  _fields[f] = t;
}

void ChanCfgOverrides::setField(const Field f, const double v) {
  Types t;
  t.setDouble(v);
  _fields[f] = t;
}

void ChanCfgOverrides::setField(const Field f, const std::string& v) {
  Types t;
  t.setString(v);
  _fields[f] = t;
}

void ChanCfgOverrides::setField(const Field f, const char* v) {
  Types t;
  t.setString(v);
  _fields[f] = t;
}

bool ChanCfgOverrides::defined(const Field f) const {
  return (_fields.find(f) != _fields.end());
}

bool ChanCfgOverrides::getBool(const Field f) const {
  Fields::const_iterator i = _fields.find(f);
  const Types t((*i).second);
  return t.getBool();
}

int ChanCfgOverrides::getInt(const Field f) const {
  Fields::const_iterator i = _fields.find(f);
  const Types t((*i).second);
  return t.getInt();
}

unsigned int ChanCfgOverrides::getUInt(const Field f) const {
  Fields::const_iterator i = _fields.find(f);
  const Types t((*i).second);
  return t.getUInt();
}

float ChanCfgOverrides::getFloat(const Field f) const {
  Fields::const_iterator i = _fields.find(f);
  const Types t((*i).second);
  return t.getFloat();
}

double ChanCfgOverrides::getDouble(const Field f) const {
  Fields::const_iterator i = _fields.find(f);
  const Types t((*i).second);
  return t.getDouble();
}

std::string ChanCfgOverrides::getString(const Field f) const {
  Fields::const_iterator i = _fields.find(f);
  const Types t((*i).second);
  return t.getString();
}
