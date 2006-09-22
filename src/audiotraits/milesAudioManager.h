// Filename: milesAudioManager.h
// Created by:  skyler (June 6, 2001)
// Prior system by: cary
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001 - 2004, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://etc.cmu.edu/panda3d/docs/license/ .
//
// To contact the maintainers of this program write to
// panda3d-general@lists.sourceforge.net .
//
////////////////////////////////////////////////////////////////////

#ifndef __MILES_AUDIO_MANAGER_H__ //[
#define __MILES_AUDIO_MANAGER_H__

#include "pandabase.h"
#ifdef HAVE_RAD_MSS //[

#include "audioManager.h"
#include "mss.h"
#include "pset.h"
#include "pmap.h"
#include "pdeque.h"

class MilesAudioSound;

class EXPCL_MILES_AUDIO MilesAudioManager: public AudioManager {
public:
  // See AudioManager.h for documentation.
  
  MilesAudioManager();
  ~MilesAudioManager();

  virtual void shutdown();

  bool is_valid();
  
  virtual PT(AudioSound) get_sound(const string& file_name, bool positional = false);
  void uncache_sound(const string& file_name);
  void clear_cache();
  void set_cache_limit(unsigned int count);
  unsigned int get_cache_limit() const;

  void set_volume(float volume);
  float get_volume() const;

  void set_play_rate(float play_rate);
  float get_play_rate() const;
  
  void set_active(bool active);
  bool get_active() const;

  void set_concurrent_sound_limit(unsigned int limit = 0);
  unsigned int get_concurrent_sound_limit() const;

  void reduce_sounds_playing_to(unsigned int count);

  void stop_all_sounds();

private:
  // The sound cache:
  class SoundData : public ReferenceCount {
  public:
    SoundData();
    ~SoundData();
    float get_length();

    string _basename;
    HAUDIO _audio;
    S32 _file_type;
    string _raw_data;
    bool _has_length;
    float _length;  // in seconds.
  };
  typedef pmap<string, PT(SoundData) > SoundMap;
  SoundMap _sounds;

  typedef pset<MilesAudioSound* > AudioSet;
  // The offspring of this manager:
  AudioSet _sounds_on_loan;

  typedef pset<MilesAudioSound* > SoundsPlaying;
  // The sounds from this manager that are currently playing:
  SoundsPlaying _sounds_playing;

  // The Least Recently Used mechanism:
  typedef pdeque<const string* > LRU;
  LRU _lru;
  // State:
  float _volume;
  float _play_rate;
  bool _active;
  int _cache_limit;
  bool _cleanup_required;
  // keep a count for startup and shutdown:
  static int _active_managers;
  static bool _miles_active;
  unsigned int _concurrent_sound_limit;
  
  bool _is_valid;
  bool _hasMidiSounds;

  // Optional Downloadable Sound field for software midi
  static HDLSFILEID _dls_field;

  typedef pset<MilesAudioManager *> Managers;
  static Managers *_managers;
  
  PT(SoundData) load(Filename file_name);
  // Tell the manager that the sound dtor was called.
  void release_sound(MilesAudioSound* audioSound);
  
  void most_recently_used(const string& path);
  void uncache_a_sound();

  void starting_sound(MilesAudioSound* audio);
  void stopping_sound(MilesAudioSound* audio);
#ifdef WIN32
  // utility function that should be moved to another class:
  bool get_registry_entry(HKEY base, 
                          const char* subKeyName, 
                          const char* keyName, 
                          string& result);
#endif
  // get the default dls file path:
  void get_gm_file_path(string& result);

  void force_midi_reset();
  void cleanup();

  friend class MilesAudioSound;


public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    AudioManager::init_type();
    register_type(_type_handle, "MilesAudioManager",
                  AudioManager::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

EXPCL_MILES_AUDIO PT(AudioManager) Create_AudioManager();


#endif //]

#endif //]


