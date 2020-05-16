
#include "config_recast.h"
#include "dconfig.h"


Configure( config_recastnavigation );
NotifyCategoryDef( recastnavigation , "");

ConfigureFn( config_recastnavigation ) {
  init_librecastnavigation();
}

void
init_librecastnavigation() {
  static bool initialized = false;
  if (initialized) {
    return;
  }
  initialized = true;

  return;
}

