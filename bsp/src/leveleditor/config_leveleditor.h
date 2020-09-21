#pragma once

#include "dconfig.h"
#include "dtoolbase.h"
#include "notifyCategoryProxy.h"

#ifdef BUILDING_EDITOR
#define EXPCL_EDITOR EXPORT_CLASS
#define EXPTP_EDITOR EXPORT_TEMPL
#else
#define EXPCL_EDITOR IMPORT_CLASS
#define EXPTP_EDITOR IMPORT_TEMPL
#endif

NotifyCategoryDecl(editor, EXPCL_EDITOR, EXPTP_EDITOR);
ConfigureDecl(config_leveleditor, EXPCL_EDITOR, EXPTP_EDITOR);

extern EXPCL_EDITOR void init_libleveleditor();
