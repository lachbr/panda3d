/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file lightmap_palettes.cpp
 * @author Brian Lach
 * @date August 01, 2018
 */

#include "lightmap_palettes.h"
#include "bspfile.h"
#include "bsplevel.h"

#include <bitset>
#include <cstdio>

NotifyCategoryDef( lightmapPalettizer, "" );

// Currently we pack every single lightmap into one texture,
// no matter how big. The way to split the lightmap palettes
// is verrry slow atm.
//#define LMPALETTE_SPLIT

LightmapPalettizer::LightmapPalettizer( const BSPLevel *level ) :
        _level( level )
{
}

void blit_lightmap_bits(const BSPLevel *level, LightmapPalette::Entry *entry,
                        PNMImage &img, int lmnum = 0, bool bounced = false, int style = 0)
{
        const dface_t *face = level->get_bspdata()->dfaces + entry->facenum;
        int width = face->lightmap_size[0] + 1;
        int height = face->lightmap_size[1] + 1;
        int num_luxels = width * height;

        int luxel = 0;

        for ( int y = 0; y < height; y++ )
        {
                for ( int x = 0; x < width; x++ )
                {

                        colorrgbexp32_t *sample;
                        if ( !bounced )
                                sample = SampleLightmap( level->get_bspdata(), face, luxel, style, lmnum );
                        else
                                sample = SampleBouncedLightmap( level->get_bspdata(), face, luxel );

			// Luxel is in linear-space.
			LVector3 luxel_col;
			ColorRGBExp32ToVector( *sample, luxel_col );
			luxel_col /= 255.0f;

                        img.set_xel( x + entry->offset[0], y + entry->offset[1], luxel_col );

                        luxel++;
                }
        }
}

PT(LightmapPaletteDirectory) LightmapPalettizer::palettize_lightmaps()
{
        PT(LightmapPaletteDirectory) dir = new LightmapPaletteDirectory;
        dir->face_palette_entries.resize(_level->get_bspdata()->numfaces);

        // Put each face in one or more palettes
        for ( int facenum = 0; facenum < _level->get_bspdata()->numfaces; facenum++ )
        {
                dface_t *face = _level->get_bspdata()->dfaces + facenum;
                if ( face->lightofs == -1 )
                {
                        // Face does not have a lightmap.
                        continue;
                }

                PT(LightmapPalette::Entry) entry = new LightmapPalette::Entry;
                entry->facenum = facenum;

                // Find or create a palette to put this face's lightmap in

                bool added = false;
                size_t palcount = dir->palettes.size();
                for (size_t i = 0; i < palcount; i++)
                {
                        LightmapPalette *pal = dir->palettes[i];
                        if (pal->packer.add_block(face->lightmap_size[0] + 1, face->lightmap_size[1] + 1, &entry->offset[0], &entry->offset[1]))
                        {
                                pal->entries.push_back(entry);
                                entry->palette = pal;
                                added = true;
                                break;
                        }
                }

                if (!added)
                {
                        PT(LightmapPalette) pal = new LightmapPalette;
                        if (!pal->packer.add_block(face->lightmap_size[0] + 1, face->lightmap_size[1] + 1, &entry->offset[0], &entry->offset[1]))
                        {
                                lightmapPalettizer_cat.error()
                                        << "lightmap (" << face->lightmap_size[0] + 1 << "x" << face->lightmap_size[1] + 1
                                        << ") too big to fit in palette (" << max_palette << "x" << max_palette << ")\n";
                        }
                        pal->entries.push_back(entry);
                        entry->palette = pal;
                        dir->palettes.push_back(pal);
                }

                dir->face_palette_entries[facenum] = entry;
        }

        // We've found a palette for each lightmap to fit in. Now generate the actual textures
        // for each palette that can be applied to geometry.
        for ( size_t i = 0; i < dir->palettes.size(); i++ )
        {
                LightmapPalette *pal = dir->palettes[i];
                pal->packer.get_minimum_dimensions(&pal->size[0], &pal->size[1]);
                int width = pal->size[0];
                int height = pal->size[1];

                PNMImage images[NUM_LIGHTMAPS];
                for ( int n = 0; n < NUM_LIGHTMAPS; n++ )
                {
                        images[n] = PNMImage( width, height );
			images[n].set_color_space( ColorSpace::CS_linear );
			images[n].set_maxval( USHRT_MAX );
                        images[n].fill(0, 1, 0);
                }

                pal->texture = new Texture;
                pal->texture->setup_2d_texture_array( width, height, NUM_LIGHTMAPS, Texture::T_unsigned_short, Texture::F_rgb );
                pal->texture->set_minfilter( SamplerState::FT_linear_mipmap_linear );
                pal->texture->set_magfilter( SamplerState::FT_linear );

                for ( size_t j = 0; j < pal->entries.size(); j++ )
                {
                        LightmapPalette::Entry *entry = pal->entries[j];
                        const dface_t *face = _level->get_bspdata()->dfaces + entry->facenum;

                        // Bounced
                        blit_lightmap_bits(_level, entry, images[0], 0, true);

                        int direct_count = face->bumped_lightmap ? NUM_BUMP_VECTS + 1 : 1;

                        for ( int n = 0; n < direct_count; n++ )
                        {
                                blit_lightmap_bits(_level, entry, images[n + 1], n);
                        }
                }

                // load all palette images into our array texture
                for ( int n = 0; n < NUM_LIGHTMAPS; n++ )
                {
                        std::ostringstream ss;
                        ss << "palette_dump/palette_" << i << "_" << n << ".tga";
                        images[n].write(ss.str());
                        pal->texture->load(images[n], n, 0 );
                }
        }

        return dir;
}
