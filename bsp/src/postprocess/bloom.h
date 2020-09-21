/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file bloom.h
 * @author Brian Lach
 * @date July 23, 2019
 */

#ifndef BLOOM_H
#define BLOOM_H

#include "postProcessEffect.h"

class Texture;
class PostProcess;

class EXPCL_BSP_POSTPROCESS BloomEffect : public PostProcessEffect
{
	DECLARE_CLASS( BloomEffect, PostProcessEffect );

PUBLISHED:
	BloomEffect( PostProcess *pp );

	virtual Texture *get_final_texture();
};

#endif
