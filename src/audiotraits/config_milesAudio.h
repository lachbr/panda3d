// Filename: config_milesAudio.h
// Created by:  skyler
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

#ifndef CONFIG_MILESAUDIO_H
#define CONFIG_MILESAUDIO_H

#include "pandabase.h"

#ifdef HAVE_RAD_MSS //[
#include "notifyCategoryProxy.h"
#include "dconfig.h"

ConfigureDecl(config_milesAudio, EXPCL_MILES_AUDIO, EXPTP_MILES_AUDIO);
NotifyCategoryDecl(milesAudio, EXPCL_MILES_AUDIO, EXPTP_MILES_AUDIO);

extern EXPCL_MILES_AUDIO void init_libMilesAudio();

#endif //]

#endif // CONFIG_MILESAUDIO_H
