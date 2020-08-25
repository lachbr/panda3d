#ifndef SCRIPLIB_H__
#define SCRIPLIB_H__

#if _MSC_VER >= 1000
#pragma once
#endif

#include "cmdlib.h"

#define	MAXTOKEN 4096

extern _BSPEXPORT char     g_TXcommand;

#define MAX_WAD_PATHS   42
extern _BSPEXPORT char         g_szWadPaths[MAX_WAD_PATHS][_MAX_PATH];
extern _BSPEXPORT int          g_iNumWadPaths;

#endif //**/ SCRIPLIB_H__
