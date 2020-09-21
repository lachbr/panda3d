#include "config_leveleditor.h"
#include "solid.h"
#include "solidNode.h"
#include "solidGeomNode.h"

NotifyCategoryDef(editor, "");

ConfigureDef(config_leveleditor);
ConfigureFn(config_leveleditor) {
  init_libleveleditor();
}

void
init_libleveleditor() {
  static bool initialized = false;
  if (initialized) {
    return;
  }

  initialized = true;

  CSolid::init_type();
  SolidNode::init_type();
  SolidGeomNode::init_type();
}
