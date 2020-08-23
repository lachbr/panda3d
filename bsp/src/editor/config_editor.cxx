#include "config_editor.h"
#include "solid.h"
#include "solidNode.h"
#include "solidGeomNode.h"

NotifyCategoryDef(editor, "");

ConfigureDef(config_editor);
ConfigureFn(config_editor) {
  init_libeditor();
}

void
init_libeditor() {
  static bool initialized = false;
  if (initialized) {
    return;
  }

  initialized = true;

  CSolid::init_type();
  SolidNode::init_type();
  SolidGeomNode::init_type();
}
