// Filename: config_pnmimagetypes.cxx
// Created by:  drose (17Jun00)
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

#include "config_pnmimagetypes.h"
#include "pnmFileTypePNM.h"
#include "pnmFileTypeSGI.h"
#include "pnmFileTypeAlias.h"
#include "pnmFileTypeRadiance.h"
#include "pnmFileTypeTIFF.h"
#include "pnmFileTypeTGA.h"
#include "pnmFileTypeYUV.h"
#include "pnmFileTypeIMG.h"
#include "pnmFileTypeSoftImage.h"
#include "pnmFileTypeBMP.h"
#ifdef HAVE_JPEG
  #include "pnmFileTypeJPG.h"
#endif
#include "sgi.h"

#include <config_pnmimage.h>
#include <pnmFileTypeRegistry.h>
#include <string_utils.h>
#include <dconfig.h>

Configure(config_pnmimagetypes);
NotifyCategoryDef(pnmimage_pnm, pnmimage_cat);
NotifyCategoryDef(pnmimage_sgi, pnmimage_cat);
NotifyCategoryDef(pnmimage_alias, pnmimage_cat);
NotifyCategoryDef(pnmimage_radiance, pnmimage_cat);
NotifyCategoryDef(pnmimage_tiff, pnmimage_cat);
NotifyCategoryDef(pnmimage_tga, pnmimage_cat);
NotifyCategoryDef(pnmimage_yuv, pnmimage_cat);
NotifyCategoryDef(pnmimage_img, pnmimage_cat);
NotifyCategoryDef(pnmimage_soft, pnmimage_cat);
NotifyCategoryDef(pnmimage_bmp, pnmimage_cat);
NotifyCategoryDef(pnmimage_jpg, pnmimage_cat);

int sgi_storage_type = STORAGE_RLE;
const string sgi_imagename = config_pnmimagetypes.GetString("sgi-imagename", "");
const double radiance_gamma_correction = config_pnmimagetypes.GetDouble("radiance-gamma-correction", 2.2);
const int radiance_brightness_adjustment = config_pnmimagetypes.GetInt("radiance-brightness-adjustment", 0);

// YUV format doesn't include an image size specification, so the
// image size must be specified externally.  The defaults here are
// likely candidates, since this is the Abekas native size; the ysize
// is automatically adjusted down to account for a short file.
const int yuv_xsize = config_pnmimagetypes.GetInt("yuv-xsize", 720);
const int yuv_ysize = config_pnmimagetypes.GetInt("yuv-ysize", 486);

// TGA supports RLE compression, as well as colormapping and/or
// grayscale images.  Set these true to enable these features, if
// possible, or false to disable them.  Some programs (like xv) have
// difficulty reading these advanced TGA files.
const bool tga_rle = config_pnmimagetypes.GetBool("tga-rle", false);
const bool tga_colormap = config_pnmimagetypes.GetBool("tga-colormap", false);
const bool tga_grayscale = config_pnmimagetypes.GetBool("tga-grayscale", false);

// IMG format is just a sequential string of r, g, b bytes.  However,
// it may or may not include a "header" which consists of the xsize
// and the ysize of the image, either as shorts or as longs.
IMGHeaderType img_header_type;
// The following are only used if header_type is 'none'.
const int img_xsize = config_pnmimagetypes.GetInt("img-xsize", 0);
const int img_ysize = config_pnmimagetypes.GetInt("img-ysize", 0);

// Set this to the quality percentage for writing JPEG files.  95 is
// the highest useful value (values greater than 95 do not lead to
// significantly better quality, but do lead to significantly greater
// size).
const int jpeg_quality = config_pnmimagetypes.GetInt("jpeg-quality", 95);

// These control the scaling that is automatically performed on a JPEG
// file for decompression.  You might specify to scale down by a
// fraction, e.g. 1/8, by specifying jpeg_scale_num = 1 and
// jpeg_scale_denom = 8.  This will reduce decompression time
// correspondingly.  Attempting to use this to scale up, or to scale
// by any fraction other than an even power of two, may not be
// supported.
const int jpeg_scale_num = config_pnmimagetypes.GetInt("jpeg-scale-num", 1);
const int jpeg_scale_denom = config_pnmimagetypes.GetInt("jpeg-scale-denom", 1);

ConfigureFn(config_pnmimagetypes) {
  init_libpnmimagetypes();
}

////////////////////////////////////////////////////////////////////
//     Function: init_libpnmimagetypes
//  Description: Initializes the library.  This must be called at
//               least once before any of the functions or classes in
//               this library can be used.  Normally it will be
//               called by the static initializers and need not be
//               called explicitly, but special cases exist.
////////////////////////////////////////////////////////////////////
void
init_libpnmimagetypes() {
  static bool initialized = false;
  if (initialized) {
    return;
  }
  initialized = true;

  init_libpnmimage();
  PNMFileTypePNM::init_type();
  PNMFileTypeSGI::init_type();
  PNMFileTypeAlias::init_type();
  PNMFileTypeRadiance::init_type();
  PNMFileTypeTIFF::init_type();
  PNMFileTypeTGA::init_type();
  PNMFileTypeYUV::init_type();
  PNMFileTypeIMG::init_type();
  PNMFileTypeSoftImage::init_type();
  PNMFileTypeBMP::init_type();
#ifdef HAVE_JPEG
  PNMFileTypeJPG::init_type();
#endif

  string sgi_storage_type_str =
    config_pnmimagetypes.GetString("sgi-storage-type", "rle");
  if (cmp_nocase(sgi_storage_type_str, "rle") == 0) {
    sgi_storage_type = STORAGE_RLE;
  } else if (cmp_nocase(sgi_storage_type_str, "verbatim") == 0) {
    sgi_storage_type = STORAGE_VERBATIM;
  } else {
    pnmimage_sgi_cat->error()
      << "Invalid sgi-storage-type: " << sgi_storage_type_str << "\n";
  }

  string img_header_type_str =
    config_pnmimagetypes.GetString("img-header-type", "long");
  if (cmp_nocase(img_header_type_str, "none") == 0) {
    img_header_type = IHT_none;
  } else if (cmp_nocase(img_header_type_str, "short") == 0) {
    img_header_type = IHT_short;
  } else if (cmp_nocase(img_header_type_str, "long") == 0) {
    img_header_type = IHT_long;
  } else {
    pnmimage_img_cat->error()
      << "Invalid img-header-type: " << img_header_type_str << "\n";
  }

  // Register each type with the PNMFileTypeRegistry.
  PNMFileTypeRegistry *tr = PNMFileTypeRegistry::get_ptr();

  tr->register_type(new PNMFileTypePNM);
  tr->register_type(new PNMFileTypeSGI);
  tr->register_type(new PNMFileTypeAlias);
  tr->register_type(new PNMFileTypeRadiance);
  tr->register_type(new PNMFileTypeTIFF);
  tr->register_type(new PNMFileTypeTGA);
  tr->register_type(new PNMFileTypeYUV);
  tr->register_type(new PNMFileTypeIMG);
  tr->register_type(new PNMFileTypeSoftImage);
  tr->register_type(new PNMFileTypeBMP);
#ifdef HAVE_JPEG
  tr->register_type(new PNMFileTypeJPG);
#endif

  // Also register with the Bam reader.
  PNMFileTypePNM::register_with_read_factory();
  PNMFileTypeSGI::register_with_read_factory();
  PNMFileTypeAlias::register_with_read_factory();
  PNMFileTypeRadiance::register_with_read_factory();
  PNMFileTypeTIFF::register_with_read_factory();
  PNMFileTypeTGA::register_with_read_factory();
  PNMFileTypeYUV::register_with_read_factory();
  PNMFileTypeIMG::register_with_read_factory();
  PNMFileTypeSoftImage::register_with_read_factory();
  PNMFileTypeBMP::register_with_read_factory();
#ifdef HAVE_JPEG
  PNMFileTypeJPG::register_with_read_factory();
#endif
}
