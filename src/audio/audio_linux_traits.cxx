// Filename: audio_linux_traits.C
// Created by:  cary (02Oct00)
// 
////////////////////////////////////////////////////////////////////

#include "audio_linux_traits.h"

#ifdef AUDIO_USE_LINUX

#include "audio_manager.h"
#include "config_audio.h"
#include <ipc_thread.h>
#include <ipc_mutex.h>
#include <set>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>

typedef set<Buffer*> BufferSet;

static bool have_initialized = false;
static mutex buffer_mutex;
static byte* buffer1;
static byte* buffer2;
static byte* current_buffer;
static byte* back_buffer;
byte* zero_buffer;
static byte* scratch_buffer;
static byte* fetch_buffer;
static int want_buffers = 0, have_buffers = 0;
static bool initializing = true;
static bool stop_mixing = false;
static int output_fd;
static thread* update_thread;
static int sample_size = sizeof(short);

BufferSet buffers;

static void swap_buffers(void) {
  byte *tmp = current_buffer;
  current_buffer = back_buffer;
  back_buffer = tmp;
}

INLINE static signed short read_buffer(byte* buf, int idx) {
  signed short ret = 0;
  switch (sample_size) {
  case 1:
    ret = *((signed char *)(&buf[idx]));
    break;
  case 2:
    ret = *((signed short *)(&buf[idx*2]));
    break;
  default:
    audio_cat->debug() << "unknown sample size (" << sample_size << ")"
		       << endl;
    break;
  }
  return ret;
}

INLINE static void write_buffer(byte* buf, int idx, signed short val) {
  switch (sample_size) {
  case 1:
    *((signed char *)(&buf[idx])) = val & 0xff;
    break;
  case 2:
    *((signed short *)(&buf[idx*2])) = val;
    break;
  default:
    audio_cat->debug() << "unknown sample size (" << sample_size << ")"
		       << endl;
    break;
  }
}

INLINE static signed short sound_clamp(signed int value) {
  signed short ret = 0;
  switch (sample_size) {
  case 1:
    ret = (value > 127)?127:((value < -128)?(-128):(value));
    break;
  case 2:
    ret = (value > 32767)?32767:((value < -32768)?(-32768):(value));
    break;
  default:
    audio_cat->debug() << "unknown sample size (" << sample_size << ")"
		       << endl;
    break;
  }
  return ret;
}

static void mix_in(byte* buf, byte* from, float vol, float pan) {
  int done = audio_buffer_size / sample_size;
  for (int i=0; i<done; i+=2) {
    signed short left = read_buffer(buf, i);
    signed short right = read_buffer(buf, i+1);

    // now get the incoming data
    signed short in_left = read_buffer(from, i);
    signed short in_right = read_buffer(from, i+1);

    // figure out panning at some point
    in_left = (short int)(in_left * vol);
    in_right = (short int)(in_right * vol);

    // compute mixed values
    left = sound_clamp(left+in_left);
    right = sound_clamp(right+in_right);

    // write it back to the buffer
    write_buffer(buf, i, left);
    write_buffer(buf, i+1, right);
  }
}

static void mix_buffer(byte* buf) {
  memcpy(scratch_buffer, zero_buffer, audio_buffer_size);
  // do stuff
  for (BufferSet::iterator i=buffers.begin(); i!=buffers.end(); ++i)
    mix_in(scratch_buffer, (*i)->get_buffer(fetch_buffer), 1., 0.);
  BufferSet to_del;
  for (BufferSet::iterator j=buffers.begin(); j!=buffers.end(); ++j)
    if ((*j)->is_done())
      to_del.insert(*j);
  {
    mutex_lock m(buffer_mutex);
    memcpy(buf, scratch_buffer, audio_buffer_size);
    --want_buffers;
    ++have_buffers;
    for (BufferSet::iterator k=to_del.begin(); k!=to_del.end(); ++k) {
      buffers.erase(*k);
      (*k)->reset();
    }
  }
}

static void update_linux(void) {
  if (buffers.empty())
    return;
  switch (want_buffers) {
  case 0:
    // all buffers are full right now.  This is a good state.
    break;
  case 1:
    // mix a buffer and put it in place.
    mix_buffer(back_buffer);
    break;
  case 2:
    if (!initializing)
      audio_cat->warning() << "audio buffers are being starved" << endl;
    mix_buffer(current_buffer);
    mix_buffer(back_buffer);
    initializing = false;
    break;
  default:
    audio_cat->error() << "audio system wants more then 2 buffers!" << endl;
  }
}

static void* internal_update(void*) {
  if ((output_fd = open(audio_device->c_str(), O_WRONLY, 0)) == -1) {
    audio_cat->error() << "could not open '" << audio_device << "'" << endl;
    return (void*)0L;
  }
  // this one I don't know about
  int fragsize = 0x0004000c;
  if (ioctl(output_fd, SNDCTL_DSP_SETFRAGMENT, &fragsize) == -1) {
    audio_cat->error() << "faied to set fragment size" << endl;
    return (void*)0L;
  }
  // for now signed, 16-bit, little endian
  int format = AFMT_S16_LE;
  if (ioctl(output_fd, SNDCTL_DSP_SETFMT, &format) == -1) {
    audio_cat->error() << "failed to set format on the dsp" << endl;
    return (void*)0L;
  }
  // set stereo
  int stereo = 1;
  if (ioctl(output_fd, SNDCTL_DSP_STEREO, &stereo) == -1) {
    audio_cat->error() << "failed to set stereo on the dsp" << endl;
    return (void*)0L;
  }
  // set the frequency
  if (ioctl(output_fd, SNDCTL_DSP_SPEED, &audio_mix_freq) == -1) {
    audio_cat->error() << "failed to set frequency on the dsp" << endl;
    return (void*)0L;
  }
  while (!stop_mixing) {
    if (have_buffers == 0) {
      ipc_traits::sleep(0, audio_auto_update_delay);
    } else {
      write(output_fd, current_buffer, audio_buffer_size);
      {
	mutex_lock m(buffer_mutex);
	swap_buffers();
	--have_buffers;
	++want_buffers;
      }
    }
  }
  delete [] buffer1;
  delete [] buffer2;
  delete [] scratch_buffer;
  delete [] fetch_buffer;
  delete [] zero_buffer;
  stop_mixing = false;
  audio_cat->debug() << "exiting internal thread" << endl;
  return (void*)0L;
}

static void shutdown_linux(void) {
  stop_mixing = true;
  while (stop_mixing);
  audio_cat->debug() << "I believe the internal thread has exited" << endl;
}

static void initialize(void) {
  if (have_initialized)
    return;

  buffer1 = new byte[audio_buffer_size];
  buffer2 = new byte[audio_buffer_size];
  scratch_buffer = new byte[audio_buffer_size];
  fetch_buffer = new byte[audio_buffer_size];
  zero_buffer = new byte[audio_buffer_size];

  for (int i=0; i<audio_buffer_size; ++i)
    zero_buffer[i] = 0;

  current_buffer = buffer1;
  back_buffer = buffer2;

  want_buffers = 2;
  have_buffers = 0;
  initializing = true;
  stop_mixing = false;

  audio_cat->info() << "spawning internal update thread" << endl;
  update_thread = thread::create(internal_update, (void*)0L,
				 thread::PRIORITY_NORMAL);

  AudioManager::set_update_func(update_linux);
  AudioManager::set_shutdown_func(shutdown_linux);
  have_initialized = true;
}

LinuxSample::~LinuxSample(void) {
}

float LinuxSample::length(void) {
  return (_data->get_size()) / (audio_mix_freq * sample_size);
}

AudioTraits::SampleClass::SampleStatus LinuxSample::status(void) {
  BufferSet::iterator i = buffers.find(_data);
  if (i != buffers.end())
    return AudioTraits::SampleClass::PLAYING;
  return AudioTraits::SampleClass::READY;
}

LinuxPlaying* LinuxSample::get_state(void) {
  return new LinuxPlaying();
}

void LinuxSample::destroy(AudioTraits::SampleClass* sample) {
  delete sample;
}

LinuxSample* LinuxSample::load_raw(byte* data, unsigned long size) {
  LinuxSample* ret = new LinuxSample(new Buffer(data, size));
  return ret;
}

LinuxMusic::~LinuxMusic(void) {
}

AudioTraits::MusicClass::MusicStatus LinuxMusic::status(void) {
  return AudioTraits::MusicClass::READY;
}

LinuxPlaying* LinuxMusic::get_state(void) {
  return new LinuxPlaying();
}

void LinuxMusic::destroy(AudioTraits::MusicClass* music) {
  delete music;
}

LinuxPlaying::~LinuxPlaying(void) {
}

AudioTraits::PlayingClass::PlayingStatus LinuxPlaying::status(void) {
  return AudioTraits::PlayingClass::BAD;
}

LinuxPlayer* LinuxPlayer::_global_instance = (LinuxPlayer*)0L;

LinuxPlayer::~LinuxPlayer(void) {
}

void LinuxPlayer::play_sample(AudioTraits::SampleClass* sample) {
  initialize();
  LinuxSample* lsample = (LinuxSample*)sample;
  buffers.insert(lsample->get_data());
}

void LinuxPlayer::play_music(AudioTraits::MusicClass*) {
}

void LinuxPlayer::set_volume(AudioTraits::SampleClass*, int) {
}

void LinuxPlayer::set_volume(AudioTraits::MusicClass*, int) {
}

LinuxPlayer* LinuxPlayer::get_instance(void) {
  if (_global_instance == (LinuxPlayer*)0L)
    _global_instance = new LinuxPlayer();
  return _global_instance;
}

#endif /* AUDIO_USE_LINUX */
