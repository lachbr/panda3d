/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * Image-space motion blur
 *
 * @file motion_blur.h
 * @author Brian Lach
 * @date September 3, 2020
 */

#pragma once

#include "config_bsp.h"
#include "referenceCount.h"
#include "nodePath.h"
#include "configVariableBool.h"
#include "configVariableDouble.h"
#include "pta_float.h"
#include "pta_LVecBase4.h"
#include "displayRegion.h"
#include "luse.h"

extern ConfigVariableBool mat_motion_blur_enabled;
extern ConfigVariableBool mat_motion_blur_forward_enabled;
extern ConfigVariableDouble mat_motion_blur_falling_min;
extern ConfigVariableDouble mat_motion_blur_falling_max;
extern ConfigVariableDouble mat_motion_blur_falling_intensity;
extern ConfigVariableDouble mat_motion_blur_rotation_intensity;
extern ConfigVariableDouble mat_motion_blur_strength;
extern ConfigVariableDouble mat_motion_blur_percent_of_screen_max;

class GraphicsOutput;
class Texture;
class CallbackData;

/**
 * Draws a screen-space quad over the scene and performs image-space
 * motion blur.
 */
class EXPCL_PANDABSP MotionBlur : public ReferenceCount {
PUBLISHED:
  MotionBlur(GraphicsOutput *output);

  void setup();
  void update();
  void shutdown();

  void set_scene_camera(const NodePath &camera);

  const NodePath &get_camera() const;

public:
  void draw(CallbackData *data);

private:
  GraphicsOutput *_output;
  PT(Texture) _framebuffer_texture;
  NodePath _scene_camera;
  CPT(Geom) _quad;
  CPT(RenderState) _quad_state;

  NodePath _np;

  NodePath _camera;

  // Params to shader
  PTA_LVecBase4f _motion_blur_params;
  PTA_LVecBase4f _consts;

  // Previous frame data
  float _last_time_update;
  float _previous_pitch;
  float _previous_yaw;
  LVector3 _previous_position;
  float _no_rotational_motion_blur_until;
};
