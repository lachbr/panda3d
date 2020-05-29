#include "config_networksystem.h"

#include "enet/enet.h"

NotifyCategoryDef(networksystem, "")

ConfigureDef(config_networksystem)

ConfigureFn(config_networksystem) {
  init_libnetworksystem();
}

void init_libnetworksystem() {
  static bool initialized = false;
  if (initialized)
    return;

  // Initialize the network library
  if (enet_initialize() != 0) {
    std::cout << "Failed to initialize ENet!" << std::endl;
    return;
  }

  initialized = true;
}
