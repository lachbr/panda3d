// Filename: config_downloader.h
// Created by:  mike (19Mar00)
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

#ifndef CONFIG_DOWNLOADER_H
#define CONFIG_DOWNLOADER_H

#include <pandabase.h>
#include <notifyCategoryProxy.h>

NotifyCategoryDecl(downloader, EXPCL_PANDAEXPRESS, EXPTP_PANDAEXPRESS);

extern const int downloader_disk_write_frequency;
extern const int downloader_byte_rate;
extern const float downloader_frequency;
extern const int downloader_timeout;
extern const int downloader_timeout_retries;

extern const int decompressor_buffer_size;
extern const float decompressor_frequency;

extern const int extractor_buffer_size;
extern const float extractor_frequency;

extern const int patcher_buffer_size;

#endif
