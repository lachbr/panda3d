/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file config_postprocess.cxx
 * @author Brian Lach
 * @date 2020-09-20
 */

#include "config_postprocess.h"

#include "bloom.h"
#include "fxaa.h"
#include "hdr.h"
#include "postProcess.h"
#include "postProcessEffect.h"
#include "postProcessPass.h"
#include "ssao.h"

NotifyCategoryDef(postprocess, "")

ConfigureDef(config_postprocess)

ConfigureFn(config_postprocess) {
  init_libpostprocess();
}

void init_libpostprocess() {
  static bool initialized = false;
  if (initialized) {
    return;
  }

  initialized = true;

  PostProcess::init_type();
  PostProcessPass::init_type();
  PostProcessEffect::init_type();

  BloomEffect::init_type();
  FXAA_Effect::init_type();
  HDRPass::init_type();
  HDREffect::init_type();
  SSAO_Effect::init_type();
}
