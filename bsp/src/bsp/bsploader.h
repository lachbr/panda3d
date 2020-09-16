/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file bsploader.h
 * @author Brian Lach
 * @date March 27, 2018
 */

#ifndef BSPLOADER_H
#define BSPLOADER_H

#include "config_bsp.h"
#include "filename.h"
#include "cycleData.h"
#include "bsplevel.h"

NotifyCategoryDeclNoExport(bsploader);

class BulletWorld;

/**
 * Loads and handles the operations of PBSP files.
 */
class EXPCL_PANDABSP BSPLoader {
PUBLISHED:
  BSPLoader();

  void set_physics_world(BulletWorld *world);
  INLINE BulletWorld *get_physics_world() const {
    return _physics_world;
  }

  virtual bool read(const Filename &file, bool is_transition = false);

  void set_ai(bool ai);
  INLINE bool is_ai() const {
    return _ai;
  }

  INLINE bool has_level() const;
  INLINE BSPLevel *get_level() const;

  INLINE UpdateSeq get_level_context() const;
  INLINE bool is_valid_level_context(const UpdateSeq &seq) const;

  INLINE bool has_visibility() const {
    BSPLevel *level = get_level();
    return (level != nullptr) && level->_has_pvs_data;
  }

  virtual void cleanup(bool is_transition = false);

  static void set_global_ptr(BSPLoader *ptr);
  static BSPLoader *get_global_ptr();

protected:
  virtual void cleanup_entities(bool is_transition);
  virtual void load_entities() = 0;
  virtual PT(BSPLevel) make_level();

  void set_level(BSPLevel *level);
  void inc_level_context();

protected:
  bool _ai;
  BulletWorld *_physics_world;

private:

  // This is data that must be cycled between pipeline stages.
  class CData : public CycleData {
  public:
    CData();
    CData(const CData &copy);
    virtual CycleData *make_copy() const;

  private:
    PT(BSPLevel) _level;
    UpdateSeq _level_context;

    friend class BSPLoader;
  };

  PipelineCycler<CData> _cycler;
  typedef CycleDataWriter<CData> CDWriter;
  typedef CycleDataReader<CData> CDReader;

  static BSPLoader *_global_ptr;
};

INLINE BSPLoader::CData::
CData() :
  _level(nullptr) {
}

INLINE BSPLoader::CData::
CData(const CData &copy) :
  _level(copy._level),
  _level_context(copy._level_context) {
}

INLINE void BSPLoader::
set_level(BSPLevel *level) {
  CDWriter cdata(_cycler);
  cdata->_level = level;
}

INLINE BSPLevel *BSPLoader::
get_level() const {
  CDReader cdata(_cycler);
  return cdata->_level;
}

INLINE bool BSPLoader::
has_level() const {
  return get_level() != nullptr;
}

INLINE UpdateSeq BSPLoader::
get_level_context() const {
  CDReader cdata(_cycler);
  return cdata->_level_context;
}

INLINE bool BSPLoader::
is_valid_level_context(const UpdateSeq &seq) const {
  return seq == get_level_context();
}

INLINE void BSPLoader::
inc_level_context() {
  CDWriter cdata(_cycler);
  cdata->_level_context++;
}

extern EXPCL_PANDABSP LColor color_from_value(const std::string &value, bool scale = true, bool gamma = false);

#endif // BSPLOADER_H
