// Filename: fmodAudioManager.cxx
// Created by:  cort (January 22, 2003)
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

#include "pandabase.h"
#ifdef HAVE_FMOD //[

#include "config_audio.h"
#include "config_util.h"
#include "config_express.h"
#include "filename.h"
#include "fmodAudioManager.h"
#include "fmodAudioSound.h"
#include "nullAudioSound.h"
#include "virtualFileSystem.h"
#include "string_utils.h"

#include <algorithm>
#include <cctype>
#include <fmod.h>

PT(AudioManager) Create_AudioManager() {
  audio_debug("Create_AudioManager() Fmod.");
  return new FmodAudioManager;
}

int FmodAudioManager::_active_managers = 0;

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::FmodAudioManager
//       Access: 
//  Description: 
////////////////////////////////////////////////////////////////////
FmodAudioManager::
FmodAudioManager() {
  audio_debug("FmodAudioManager::FmodAudioManager()");
  audio_debug("  audio_active="<<audio_active);
  audio_debug("  audio_volume="<<audio_volume);
  _active = audio_active;
  _volume = audio_volume;

  //positional audio data
  _listener_pos[0]     = 0.0f; _listener_pos[1]     = 0.0f;     _listener_pos[2] = 0.0f;
  _listener_vel[0]     = 0.0f; _listener_vel[1]     = 0.0f;     _listener_vel[2] = 0.0f;
  _listener_forward[0] = 0.0f; _listener_forward[1] = 1.0f; _listener_forward[2] = 0.0f;
  _listener_up[0]      = 0.0f; _listener_up[1]      = 0.0f;      _listener_up[2] = 1.0f;
  _distance_factor     = .3048f;
  _doppler_factor      = 1.0f;
  _drop_off_factor     = 1.0f;

  _cache_limit = audio_cache_limit;
  _concurrent_sound_limit = 0;

  // RobCode
  // Fill list of supported types
  // Order of this list (first is most important) determines
  // the search order for sound files without an extension.
  _supported_types.push_back("wav");
  _supported_types.push_back("ogg");
  _supported_types.push_back("mp3");
  _supported_types.push_back("mid");
  _supported_types.push_back("midi");
  _supported_types.push_back("rmi");

  // Initialize FMOD, if this is the first manager created.
  _is_valid = true;
  if (_active_managers == 0) {
    do {
      audio_debug("Initializing FMOD for real.");
      float fmod_dll_version = FSOUND_GetVersion();
      if (fmod_dll_version < FMOD_VERSION) {
        audio_error("Wrong FMOD DLL version.  You have "<<fmod_dll_version
            <<".  You need "<<FMOD_VERSION);
        _is_valid = false;
        break;
      }


      // If the local system doesn't have enough hardware channels,
      // Don't even bother trying to use hardware effects. Play EVERYTHING in software.
      audio_debug("Setting minimum hardware channels(min="<<audio_min_hw_channels<<")");
      FSOUND_SetMinHardwareChannels(audio_min_hw_channels);
      
      if (FSOUND_Init(audio_output_rate, audio_cache_limit, 0) == 0) {
        audio_error("Fmod initialization failure.");
        _is_valid = false;
        break;
      }
    }
    while(0); // curious -- why is there a non-loop here?
  }

  // set 3D sound characteristics as they are given in the configrc
  audio_3d_set_doppler_factor(audio_doppler_factor);
  audio_3d_set_distance_factor(audio_distance_factor);
  audio_3d_set_drop_off_factor(audio_drop_off_factor);
  // increment regardless of whether an error has occured -- the
  // destructor will do the right thing.
  ++_active_managers;
  audio_debug("  _active_managers="<<_active_managers);

  if (_is_valid)  {
    nassertv(is_valid());    
  }
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::~FmodAudioManager
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
FmodAudioManager::
~FmodAudioManager() {
  // Be sure to delete associated sounds before deleting the manager!
  nassertv(_soundsOnLoan.empty());
  clear_cache();
  --_active_managers;
  audio_debug("~FmodAudioManager(): "<<_active_managers<<" still active");
  if (_active_managers == 0) {
    audio_debug("Shutting down FMOD");
    FSOUND_Close();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::is_valid
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
bool FmodAudioManager::
is_valid() {
  bool check=true;
  if (_sounds.size() != _lru.size()) {
    audio_debug("--sizes--");
    check=false;
  } else {
    LRU::const_iterator i=_lru.begin();
    for (; i != _lru.end(); ++i) {
      SoundMap::const_iterator smi=_sounds.find(*i);
      if (smi == _sounds.end()) {
        audio_debug("--"<<*i<<"--");
        check=false;
        break;
      }
    }
  }
  return _is_valid && check;
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::get_sound
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
PT(AudioSound) FmodAudioManager::
get_sound(const string &file_name, bool positional) {
  audio_debug("FmodAudioManager::get_sound(file_name=\""<<file_name<<"\")");

  if(!is_valid()) {
     audio_debug("invalid FmodAudioManager returning NullSound");
     return get_null_sound();
  }

  nassertr(is_valid(), NULL);
  Filename path = file_name;

  // RobCode
  // test for an invalid suffix type
  string suffix = downcase(path.get_extension());
  if (!suffix.empty()) {
    SupportedTypes::iterator type_i=find(_supported_types.begin(), _supported_types.end(), suffix);
    // if suffix not found in list of supported types
    if (type_i == _supported_types.end()) {
        // print error and return
        audio_error("FmodAudioManager::get_sound: \""<<path<<"\" is not a supported sound file format.");
        audio_error("Supported formats are: WAV, OGG, MP3, MID, MIDI, RMI");
        return get_null_sound();
    } else { // the suffix is of a supported type
      audio_debug("FmodAudioManager::get_sound: \""<<path<<"\" is a supported sound file format.");
      // resolve the path normally
      VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
      vfs->resolve_filename(path, get_sound_path());
    }
  } else { // no suffix given. Search for supported file types of the same name.
    audio_debug("FmodAudioManager::get_sound: \""<<path<<"\" has no extension. Searching for supported files with the same name.");
    // look for each type of file 
    SupportedTypes::const_iterator type_i; 
    for (type_i = _supported_types.begin(); type_i != _supported_types.end(); ++type_i) { 
      path.set_extension(*type_i); // set extension as supported type
      
      VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
      if (vfs->resolve_filename(path, get_sound_path())) {
        break; // break out of loop with a valid type_i value and path with correct extension
      }
    } // end for loop
    // if no valid file found
    if (type_i == _supported_types.end() ) {
      // just print a warning for now
      audio_debug("FmodAudioManager::get_sound: \""<<file_name<<"\" does not exist, even with default sound extensions.");
      // reset path to no extension
      path.set_extension("");
    } else {
        audio_debug("FmodAudioManager::get_sound: \""<<path<<"\" found using default sound extensions.");
        suffix = downcase(path.get_extension()); // update suffix (used below when loading file)
    }
  }

  audio_debug("  resolved file_name is '"<<path<<"'");

  // Get the sound, either from the cache or from disk.
  SoundMap::iterator si = _sounds.find(path);
  SoundCacheEntry *entry = NULL;
  if (si != _sounds.end()) {
    // The sound was found in the cache.
    entry = &(*si).second;
    audio_debug("Sound file '"<<path<<"' found in cache.");
  } else {
    // The sound was not found in the cache.  Load it from disk.
    SoundCacheEntry new_entry;
    new_entry.data = load(path, new_entry.size);
    if (!new_entry.data) {
      audio_cat.error()
        << "FmodAudioManager::load(" << file_name << ", " << positional
        << ") failed.\n";
      return get_null_sound();
    }
    new_entry.refcount = 0;
    new_entry.stale = true;

    // Add to the cache
    while (_sounds.size() >= (unsigned int)_cache_limit) {
      nassertr(is_valid(), NULL);
      if (!uncache_a_sound()) {
        audio_warning(_sounds.size()+1 << " sounds cached. Limit is " << _cache_limit );
        break;
      }
    }

    si = _sounds.insert(SoundMap::value_type(path, new_entry)).first;

    // It's important that we assign entry to the address of the entry
    // we just added to the map, and not to the address of the
    // temporary variable new_entry, which we just defined locally and
    // is about to go out of scope.
    entry = &(*si).second;
  }
  nassertr(entry != NULL, NULL);
  
  // Create an FMOD object from the memory-mapped file.  Here remains
  // one last vestige of special-case MIDI code: apparently, FMOD
  // doesn't like creating streams from memory-mapped MIDI files.
  // They must therefore be streamed from disk every time.  This
  // causes strange things to happen when the same MIDI file is loaded
  // twice, and played simultaneously...so, *don't do that then*.  all
  // I can say is that MIDI support will be significantly improved in
  // FMOD v4.0!
  FSOUND_STREAM *stream = NULL;
  int flags = FSOUND_LOADMEMORY | FSOUND_MPEGACCURATE;

  // 3D sounds have to be mono. Forcing stereo streams
  // to be mono will create a speed hit.
  if (positional) {
      flags |= FSOUND_HW3D | FSOUND_FORCEMONO;
  } else {
      flags |= FSOUND_2D;
  }
  if (audio_output_bits == 8) {
      flags |= FSOUND_8BITS;
  } else if (audio_output_bits == 16) {
      flags |= FSOUND_16BITS;
  }
  if (audio_output_channels == 1) {
      flags |= FSOUND_FORCEMONO;
  }

  string os_path = path.to_os_specific();
  //string suffix = downcase(path.get_extension()); // declared above by RobCode
  
  if (suffix == "mid" || suffix == "rmi" || suffix == "midi") {
    stream = FSOUND_Stream_Open(os_path.c_str(), 0, 0, 0);
  } else {
    stream = FSOUND_Stream_Open((const char*)(entry->data),
                                flags, 0, entry->size);
  }
  if (stream == NULL) {
    audio_cat.error()
      << "FmodAudioManager::get_sound(" << file_name << ", " << positional
      << ") failed.\n";
    return get_null_sound();
  }
  inc_refcount(path);
  most_recently_used(path);

  // determine length of sound
  float length = (float)FSOUND_Stream_GetLengthMs(stream) * 0.001f;

  // Build a new AudioSound from the audio data.
  PT(AudioSound) audioSound = 0;
  PT(FmodAudioSound) fmodAudioSound = new FmodAudioSound(this, stream, path,
               length);
  audio_debug("BOO!");
  fmodAudioSound->set_active(_active);
  audio_debug("GAH!");
  _soundsOnLoan.insert(fmodAudioSound);
  audio_debug("GIR!");
  audioSound = fmodAudioSound;
  
  audio_debug("  returning 0x" << (void*)audioSound);
  nassertr(is_valid(), NULL);
  audio_debug("GOO!");

  return audioSound;
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::uncache_sound
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
void FmodAudioManager::
uncache_sound(const string& file_name) {
  audio_debug("FmodAudioManager::uncache_sound(\""<<file_name<<"\")");
  nassertv(is_valid());
  Filename path = file_name;

  VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
  vfs->resolve_filename(path, get_sound_path());
  audio_debug("  path=\""<<path<<"\"");
  SoundMap::iterator itor = _sounds.find(path);
  if (itor == _sounds.end()) {
    audio_error("FmodAudioManager::uncache_sound: no such entry "<<file_name);
    return;
  }

  // Mark the entry as stale -- when its refcount reaches zero, it will
  // be removed from the cache.
  SoundCacheEntry *entry = &(*itor).second;
  if (entry->refcount == 0) {
    // If the refcount is already zero, it can be
    // purged right now!
    audio_debug("FmodAudioManager::uncache_sound: purging "<<path
    << " from the cache.");
    delete [] entry->data;

    // Erase the sound from the LRU list as well.
    nassertv(_lru.size()>0);
    LRU::iterator lru_i=find(_lru.begin(), _lru.end(), itor->first);
    nassertv(lru_i != _lru.end());
    _lru.erase(lru_i);
    _sounds.erase(itor);
  } else {
    entry->stale = true;
  }

  nassertv(is_valid());
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::uncache_a_sound
//       Access: Public
//  Description: Uncaches the least recently used sound.
////////////////////////////////////////////////////////////////////
bool FmodAudioManager::
uncache_a_sound() {
  audio_debug("FmodAudioManager::uncache_a_sound()");
  nassertr(is_valid(), false);
  // uncache least recently used:
  unsigned int orig_size = _lru.size();

  for (LRU::iterator it = _lru.begin(); it != _lru.end(); it++) {
    LRU::reference path=_lru.front();
    SoundMap::iterator i = _sounds.find(path);
    if (i == _sounds.end()) continue;
    audio_debug("  uncaching \""<<i->first<<"\"");
    uncache_sound(path);
    nassertr(is_valid(), false);
    if (_lru.size() < orig_size) return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::most_recently_used
//       Access: Public
//  Description: Indicates that the given sound was the most recently used.
////////////////////////////////////////////////////////////////////
void FmodAudioManager::
most_recently_used(const string& path) {
  audio_debug("FmodAudioManager::most_recently_used(path=\""
      <<path<<"\")");
  LRU::iterator i=find(_lru.begin(), _lru.end(), path);
  if (i != _lru.end()) {
    _lru.erase(i);
  }
  // At this point, path should not exist in the _lru:
  nassertv(find(_lru.begin(), _lru.end(), path) == _lru.end());
  _lru.push_back(path);
  nassertv(is_valid());
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::clear_cache
//       Access: Public
//  Description: Clear out the sound cache.
////////////////////////////////////////////////////////////////////
void FmodAudioManager::
clear_cache() {
  audio_debug("FmodAudioManager::clear_cache()");
  // Mark all cache entries as stale.  Delete those which already have 
  // refcounts of zero.

  SoundMap::iterator itor = _sounds.begin();

  // Have to use a while loop, not a for loop, since we don't want to
  // increment itor in the case in which we delete an entry.
  while (itor != _sounds.end()) {
    SoundCacheEntry *entry = &(*itor).second;
    if (entry->refcount == 0) {
      audio_debug("FmodAudioManager::clear_cache: purging "<< (*itor).first
      << " from the cache.");
      delete [] entry->data;

      // Erase the sound from the LRU list as well.
      nassertv(_lru.size()>0);
      LRU::iterator lru_i=find(_lru.begin(), _lru.end(), itor->first);
      nassertv(lru_i != _lru.end());
      _lru.erase(lru_i);
      _sounds.erase(itor);

      itor = _sounds.begin();
    } else {
      entry->stale = true;
      ++itor;
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::set_cache_limit
//       Access: Public
//  Description: Set the number of sounds that the cache can hold.
////////////////////////////////////////////////////////////////////
void FmodAudioManager::
set_cache_limit(unsigned int count) {
  audio_debug("FmodAudioManager::set_cache_limit(count="<<count<<")");
  nassertv(is_valid());
  while (_lru.size() > count) {
    nassertv(is_valid());
    uncache_a_sound();
  }
  _cache_limit = count;
  nassertv(is_valid());
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::get_cache_limit
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
unsigned int FmodAudioManager::
get_cache_limit() const {
  audio_debug("FmodAudioManager::get_cache_limit() returning "
        <<_cache_limit);
  return _cache_limit;
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::release_sound
//       Access: Private
//  Description:
////////////////////////////////////////////////////////////////////
void FmodAudioManager::
release_sound(FmodAudioSound* audioSound) {
  audio_debug("FmodAudioManager::release_sound(audioSound=\""
      <<audioSound->get_name()<<"\")");
  dec_refcount(audioSound->get_name());
  _soundsOnLoan.erase(audioSound);
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::set_volume
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
void FmodAudioManager::
set_volume(float volume) {
  audio_debug("FmodAudioManager::set_volume(volume="<<volume<<")");
  if (_volume!=volume) {
    _volume = volume;
    // Tell our AudioSounds to adjust:
    AudioSet::iterator i=_soundsOnLoan.begin();
    for (; i!=_soundsOnLoan.end(); ++i) {
      (**i).set_volume((**i).get_volume());
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::get_volume
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
float FmodAudioManager::
get_volume() const {
  audio_debug("FmodAudioManager::get_volume() returning "<<_volume);
  return _volume;
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::set_active
//       Access: Public
//  Description: turn on/off
////////////////////////////////////////////////////////////////////
void FmodAudioManager::
set_active(bool active) {
  audio_debug("FmodAudioManager::set_active(flag="<<active<<")");
  if (_active!=active) {
    _active=active;
    // Tell our AudioSounds to adjust:
    AudioSet::iterator i=_soundsOnLoan.begin();
    for (; i!=_soundsOnLoan.end(); ++i) {
      (**i).set_active(_active);
    }
  }
  _active = active;
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::get_active
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
bool FmodAudioManager::
get_active() const {
  audio_debug("FmodAudioManager::get_active() returning "<<_active);
  return _active;
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::set_concurrent_sound_limit
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
void FmodAudioManager::
set_concurrent_sound_limit(unsigned int limit) {
  _concurrent_sound_limit = limit;
  reduce_sounds_playing_to(_concurrent_sound_limit);
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::get_concurrent_sound_limit
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
unsigned int FmodAudioManager::
get_concurrent_sound_limit() const {
  return _concurrent_sound_limit;
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::reduce_sounds_playing_to
//       Access: Private
//  Description: 
////////////////////////////////////////////////////////////////////
void FmodAudioManager::
reduce_sounds_playing_to(unsigned int count) {
  int limit = _sounds_playing.size() - count;
  while (limit-- > 0) {
    SoundsPlaying::iterator sound = _sounds_playing.begin();
    nassertv(sound != _sounds_playing.end());
    (**sound).stop();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::stop_a_sound
//       Access: Public
//  Description: Stop playback on one sound managed by this manager.
////////////////////////////////////////////////////////////////////
//void FmodAudioManager::
//stop_a_sound() {
//  audio_debug("FmodAudioManager::stop_a_sound()");
//  AudioSet::size_type s=_soundsOnLoan.size() - 1;
//  reduce_sounds_playing_to(s);
  //if (s == _soundsOnLoan.size()) return true;
  //return false;
//}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::stop_all_sounds
//       Access: Public
//  Description: Stop playback on all sounds managed by this manager.
////////////////////////////////////////////////////////////////////
void FmodAudioManager::
stop_all_sounds() {
  audio_debug("FmodAudioManager::stop_all_sounds()");
  reduce_sounds_playing_to(0);
  //AudioSet::iterator i=_soundsOnLoan.begin();
  //for (; i!=_soundsOnLoan.end(); ++i) {
  //  if ((**i).status()==AudioSound::PLAYING) {
  //    (**i).stop();
  //  }
  //}
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::audio_3d_update
//       Access: Public
//  Description: Commit position changes to listener and all
//               positioned sounds. Normally, you'd want to call this
//               once per iteration of your main loop.
////////////////////////////////////////////////////////////////////
void FmodAudioManager::
audio_3d_update() {
    audio_debug("FmodAudioManager::audio_3d_update()");
    FSOUND_Update();
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::audio_3d_set_listener_attributes
//       Access: Public
//  Description: Set position of the "ear" that picks up 3d sounds
////////////////////////////////////////////////////////////////////
void FmodAudioManager::
audio_3d_set_listener_attributes(float px, float py, float pz, float vx, float vy, float vz, float fx, float fy, float fz, float ux, float uy, float uz) {
    audio_debug("FmodAudioManager::audio_3d_set_listener_attributes()");

    _listener_pos[0]     = px; _listener_pos[1]     = py; _listener_pos[2]     = pz;
    _listener_vel[0]     = vx; _listener_vel[1]     = vy; _listener_vel[2]     = vz;
    _listener_forward[0] = fx; _listener_forward[1] = fy; _listener_forward[2] = fz;
    _listener_up[0]      = ux; _listener_up[1]      = uy; _listener_up[2]      = uz;

    //convert panda coordinates to fmod coordinates
    float fmod_pos [] = {_listener_pos[0], _listener_pos[2], _listener_pos[1]};
    float fmod_vel [] = {_listener_vel[0], _listener_vel[2], _listener_vel[1]};
    float fmod_forward [] = {_listener_forward[0], _listener_forward[2], _listener_forward[1]};
    float fmod_up [] = {_listener_up[0], _listener_up[2], _listener_up[1]};

    FSOUND_3D_Listener_SetAttributes(fmod_pos, fmod_vel,
                                         fmod_forward[0], fmod_forward[1], fmod_forward[2],
                                         fmod_up[0], fmod_up[1], fmod_up[2]);
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::audio_3d_get_listener_attributes
//       Access: Public
//  Description: Get position of the "ear" that picks up 3d sounds
////////////////////////////////////////////////////////////////////
void FmodAudioManager::
audio_3d_get_listener_attributes(float *px, float *py, float *pz, float *vx, float *vy, float *vz, float *fx, float *fy, float *fz, float *ux, float *uy, float *uz) {
    audio_error("audio3dGetListenerAttributes: currently unimplemented. Get the attributes of the attached object");
    //audio_debug("FmodAudioManager::audio_3d_get_listener_attributes()");
    // NOTE: swap the +y with the +z axis to convert between FMOD
    //       coordinates and Panda3D coordinates
    //float temp;
    //temp = py;
    //py = pz;
    //pz = temp;

    //temp = vy;
    //vy = vz;
    //vz = temp;

    //float pos [] =   {px, py, pz};
    //float vel [] =   {vx, vy, vz};
    //float front [] = {fx, fz, fy};
    //float up [] =    {ux, uz, uy};

    //FSOUND_3D_Listener_GetAttributes(pos, vel, &front[0], &front[1], &front[2], &up[0], &up[1], &up[2]);
}


////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::audio_3d_set_distance_factor
//       Access: Public
//  Description: Set units per meter (Fmod uses meters internally for
//               its sound-spacialization calculations)
////////////////////////////////////////////////////////////////////
void FmodAudioManager::
audio_3d_set_distance_factor(float factor) {
    audio_debug("FmodAudioManager::audio_3d_set_distance_factor(factor="<<factor<<")");
    if (factor<0.0) {
        audio_debug("Recieved value below 0.0. Clamping to 0.0.");
        factor = 0.0;
    }
    if (_distance_factor != factor){
        _distance_factor = factor;
        FSOUND_3D_SetDistanceFactor(_distance_factor*3.28f); // convert from feet to meters
    }
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::audio_3d_get_distance_factor
//       Access: Public
//  Description: Gets units per meter (Fmod uses meters internally for
//               its sound-spacialization calculations)
////////////////////////////////////////////////////////////////////
float FmodAudioManager::
audio_3d_get_distance_factor() const {
    audio_debug("FmodAudioManager::audio_3d_get_distance_factor()");
    return _distance_factor;
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::audio_3d_set_doppler_factor
//       Access: Public
//  Description: Exaggerates or diminishes the Doppler effect. 
//               Defaults to 1.0
////////////////////////////////////////////////////////////////////
void FmodAudioManager::
audio_3d_set_doppler_factor(float factor) {
    audio_debug("FmodAudioManager::audio_3d_set_doppler_factor(factor="<<factor<<")");
    if (factor<0.0) {
        audio_debug("Recieved value below 0.0. Clamping to 0.0.");
        factor = 0.0;
    }
    if (_doppler_factor != factor) {
        _doppler_factor = factor;
        FSOUND_3D_SetDopplerFactor(_doppler_factor);
    }
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::audio_3d_get_doppler_factor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
float FmodAudioManager::
audio_3d_get_doppler_factor() const {
    audio_debug("FmodAudioManager::audio_3d_get_doppler_factor()");
    return _doppler_factor;
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::audio_3d_set_drop_off_factor
//       Access: Public
//  Description: Control the effect distance has on audability.
//               Defaults to 1.0
////////////////////////////////////////////////////////////////////
void FmodAudioManager::
audio_3d_set_drop_off_factor(float factor) {
    audio_debug("FmodAudioManager::audio_3d_set_drop_off_factor("<<factor<<")");
    if (factor<0.0) {
        audio_debug("Recieved value below 0.0. Clamping to 0.0");
        factor = 0.0;
    }
    if (_drop_off_factor != factor) {
        _drop_off_factor = factor;
        FSOUND_3D_SetRolloffFactor(_drop_off_factor);
    }
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::audio_3d_get_drop_off_factor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
float FmodAudioManager::
audio_3d_get_drop_off_factor() const {
    audio_debug("FmodAudioManager::audio_3d_get_drop_off_factor()");
    return _drop_off_factor;

}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::inc_refcount
//       Access: Protected
//  Description: Increments the refcount of a file's cache entry.
////////////////////////////////////////////////////////////////////
void FmodAudioManager::
inc_refcount(const string& file_name) {
  Filename path = file_name;
  SoundMap::iterator itor = _sounds.find(path);
  if (itor == _sounds.end()) {
    audio_debug("FmodAudioManager::inc_refcount: no such file "<<path);
    return;
  }

  SoundCacheEntry *entry = &(*itor).second;
  entry->refcount++;
  entry->stale = false; // definitely not stale!
  audio_debug("FmodAudioManager: "<<path<<" has a refcount of "
        << entry->refcount);
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::dec_refcount
//       Access: Protected
//  Description: Decrements the refcount of a file's cache entry. If
//               the refcount reaches zero and the entry is stale, it
//               will be removed from the cache.
////////////////////////////////////////////////////////////////////
void FmodAudioManager::
dec_refcount(const string& file_name) {
  Filename path = file_name;
  SoundMap::iterator itor = _sounds.find(path);
  if (itor != _sounds.end()) {
    SoundCacheEntry *entry = &(*itor).second;
    entry->refcount--;
    audio_debug("FmodAudioManager: "<<path<<" has a refcount of "
    << entry->refcount);
    if (entry->refcount == 0 && entry->stale) {
      audio_debug("FmodAudioManager::dec_refcount: purging "<<path<< " from the cache.");
      delete [] entry->data;

      // Erase the sound from the LRU list as well.
      nassertv(_lru.size()>0);
      LRU::iterator lru_i=find(_lru.begin(), _lru.end(), itor->first);
      nassertv(lru_i != _lru.end());
      _lru.erase(lru_i);
      _sounds.erase(itor);
    }
  } else {
    audio_debug("FmodAudioManager::dec_refcount: no such file "<<path);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: FmodAudioManager::load
//       Access: Private
//  Description: Loads the specified file into memory.  Returns a
//               newly-allocated buffer, and stores the size of the
//               buffer in size.  Returns NULL if an error occurs.
////////////////////////////////////////////////////////////////////
char* FmodAudioManager::
load(const Filename& filename, size_t &size) const {
  // Check file type (based on filename suffix
  string suffix = filename.get_extension();
#ifdef HAVE_ZLIB
  if (suffix == "pz") {
    suffix = Filename(filename.get_basename_wo_extension()).get_extension();
  }
#endif  // HAVE_ZLIB
  suffix = downcase(suffix);
  bool bSupported = false;
  if (suffix == "wav" || suffix == "mp3" || suffix == "mid"
      || suffix == "rmi" || suffix == "midi"
      || suffix == "mod" || suffix == "s3m" || suffix == "it"
      || suffix == "ogg" || suffix == "aiff" || suffix == "wma") {
    bSupported = true;
  }
  if (!bSupported) {
    audio_error("FmodAudioManager::load: "<<filename
    <<" is not a supported file format.");
    audio_error("Supported formats are: WAV, MP3, MIDI, OGG, AIFF, WMA, MODS");
    return NULL;
  }

  // open the file.
  istream *audioFile = NULL;

  Filename binary_filename = Filename::binary_filename(filename);
  VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();

  if (!vfs->exists(filename)) {
    audio_error("File " << filename << " does not exist.");
    return NULL;
  }
  
  audioFile = vfs->open_read_file(binary_filename, true);

  if (audioFile == (istream *)NULL) {
    // Unable to open.
    audio_error("Unable to read " << filename << ".");
    return NULL;
  }

  // Determine the file size.
  audioFile->seekg(0, ios::end);
  size = (size_t)audioFile->tellg();
  audioFile->seekg(0, ios::beg);
  
  // Read the entire file into memory.
  char *buffer = new char[size];
  if (buffer == NULL) {
    audio_error("out-of-memory error while loading "<<filename);
    vfs->close_read_file(audioFile);
    return NULL;
  }
  audioFile->read(buffer, size);
  if (!(*audioFile)) {
    audio_error("Read error while loading "<<filename);
    vfs->close_read_file(audioFile);
    delete [] buffer;
    return NULL;
  }

  vfs->close_read_file(audioFile);
  return buffer;
}

#endif //]
