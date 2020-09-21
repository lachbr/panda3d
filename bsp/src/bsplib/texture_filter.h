#pragma once

#include "config_bsplib.h"
#include "texturePoolFilter.h"

/**
 * Converts all textures to SRGB space when loaded.
 */
class EXPCL_PANDABSP BSPTextureFilter : public TexturePoolFilter {
  DECLARE_CLASS(BSPTextureFilter, TexturePoolFilter);

public:
  virtual PT(Texture) post_load(Texture *tex);
};
