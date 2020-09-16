/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file bsploader.cpp
 * @author Brian Lach
 * @date March 27, 2018
 */

#include "bsploader.h"
#include "dSearchPath.h"
#include "virtualFileSystem.h"

NotifyCategoryDef(bsploader, "");

BSPLoader *BSPLoader::_global_ptr = nullptr;

void BSPLoader::cleanup(bool is_transition) {
  BSPLevel *level = get_level();

  if (!level) {
    return;
  }

  level->cleanup(is_transition);

  cleanup_entities(is_transition);

  set_level(nullptr);
}

bool BSPLoader::read(const Filename &file, bool is_transition) {
  cleanup(is_transition);

  dtexdata_init();

  VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();

  Filename load_filename;
  if (file.is_local()) {
    // Look along the model path for the file
    DSearchPath search_path(get_model_path());
    for (int i = 0; i < search_path.get_num_directories(); i++) {
      Filename search(search_path.get_directory(i), file);
      if (vfs->exists(search)) {
        load_filename = search;
        break;
      }
    }

  } else {
    // This is an absolute filename. Use it as-is
    load_filename = file;
  }

  if (load_filename.empty()) {
    bsploader_cat.error()
      << "Could not find BSP `" << file << "` on model-path " << get_model_path()
      << "\n";
      return false;
  }

  bsploader_cat.info()
    << "Reading " << load_filename.get_fullpath() << "...\n";

  string data;
  vfs->read_file(load_filename, data, true);
  int length = data.length();
  char *buffer = new char[length + 1];
  memcpy(buffer, data.c_str(), length);
  bspdata_t *bspdata = LoadBSPImage((dheader_t *)buffer);

  PT(BSPLevel) level = make_level();
  if (!level) {
    return false;
  }
  level->set_filename(load_filename);
  if (!level->load(bspdata)) {
    level->cleanup(false);
    return false;
  }

  inc_level_context();

  // Make this the active level.
  set_level(level);

  load_entities();

  return true;
}

PT(BSPLevel) BSPLoader::
make_level() {
  return nullptr;
}

void BSPLoader::cleanup_entities(bool is_transition) {
  // Implemented in server version
}

BSPLoader::BSPLoader() :
  _ai(false),
  _physics_world(nullptr)
{
}

void BSPLoader::set_physics_world(BulletWorld *world) {
  _physics_world = world;
}

/**
 * Sets whether or not this is an AI/Server instance of the loader.
 * If this is true, only the AI views of entities will be loaded,
 * and nothing related to rendering will be dealt with.
 */
void BSPLoader::set_ai(bool ai) {
  _ai = ai;
}

void BSPLoader::set_global_ptr(BSPLoader *ptr) {
  _global_ptr = ptr;
}

BSPLoader *BSPLoader::get_global_ptr() {
  return _global_ptr;
}

CycleData *BSPLoader::CData::
make_copy() const {
  return new CData(*this);
}
