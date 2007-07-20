// Filename: config_mathutil.h
// Created by:  drose (01Oct99)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001 - 2004, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://etc.cmu.edu/panda3d/docs/license/ .
//
// To contact the maintainers of this program write to
// panda3d-general@lists.sourceforge.net .
//
////////////////////////////////////////////////////////////////////

#ifndef CONFIG_MATHUTIL_H
#define CONFIG_MATHUTIL_H

#include "pandabase.h"
#include "notifyCategoryProxy.h"
#include "configVariableDouble.h"
#include "configVariableEnum.h"
#include "boundingVolume.h"

NotifyCategoryDecl(mathutil, EXPCL_PANDA_MATHUTIL, EXPTP_PANDA_MATHUTIL);

extern ConfigVariableDouble fft_offset;
extern ConfigVariableDouble fft_factor;
extern ConfigVariableDouble fft_exponent;
extern ConfigVariableDouble fft_error_threshold;
extern EXPCL_PANDA_MATHUTIL ConfigVariableEnum<BoundingVolume::BoundsType> bounds_type;

#endif


