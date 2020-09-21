/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file fxaa.h
 * @author Brian Lach
 * @date December 07, 2019
 */

#ifndef FXAA_H_
#define FXAA_H_

#include "postProcessEffect.h"

class Texture;
class PostProcess;

class EXPCL_BSP_POSTPROCESS FXAA_Effect : public PostProcessEffect
{
	DECLARE_CLASS( FXAA_Effect, PostProcessEffect );

PUBLISHED:
	FXAA_Effect( PostProcess *pp );

	virtual Texture *get_final_texture();
};

#endif // FXAA_H_
