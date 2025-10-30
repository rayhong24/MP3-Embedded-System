#include "volume.h"
#include "display.h"
#include "hal/rotary_encoder.h"
#include <alloca.h>
#include <limits.h>
#include <alsa/asoundlib.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>

#include "app.h"

#define DEFAULT_VOLUME 80
#define DEFAULT_VOL_INCREMENTS 5
#define MAX_VOLUME 100

#define VOLUME_TO_DISPLAY_CONVERSION (PROGRESS_BAR_MAX / MAX_VOLUME)

static int volume = 0;
static bool s_initialized = false;


void Volume_decreaseVolume(void) {
  // if current freq is already 0, don't subt anything
  if (volume > 0) {
    int setNewVolume = volume - DEFAULT_VOL_INCREMENTS;
    Volume_setVolume(setNewVolume);
    // debug
    // printf("Current decreased Vol is...: %d\n", volume);
  } else {
    printf("Volume cannot be lower than 0\n");
  }
}
void Volume_increaseVolume(void) {
  // if current freq is already 500, don't add anything
  if (volume < MAX_VOLUME) {
    int setNewVolume = volume + DEFAULT_VOL_INCREMENTS;
    Volume_setVolume(setNewVolume);
    // debug
    // printf("Current increased Vol is...: %d\n", volume);

  } else {
    printf("Volume cannot be higher than 100\n");
  }
}

// Function copied from:
// http://stackoverflow.com/questions/6787318/set-alsa-master-volume-from-c-code
// Written by user "trenki".
void Volume_setVolume(int newVolume) {
  // Ensure volume is reasonable; If so, cache it for later getVolume() calls.
  if (newVolume < 0 || newVolume > MAX_VOLUME) {
    printf("ERROR: Volume must be between 0 and 100.\n");
    return;
  }

  App_updateVolume(newVolume);
  
  volume = newVolume;

  long min, max;
  snd_mixer_t *mixerHandle;
  snd_mixer_selem_id_t *sid;
  const char *card = "default";
  const char *selem_name = "PCM"; // For ZEN cape
  // const char *selem_name = "Speaker"; // For USB Audio

  snd_mixer_open(&mixerHandle, 0);
  snd_mixer_attach(mixerHandle, card);
  snd_mixer_selem_register(mixerHandle, NULL, NULL);
  snd_mixer_load(mixerHandle);

  snd_mixer_selem_id_alloca(&sid);
  snd_mixer_selem_id_set_index(sid, 0);
  snd_mixer_selem_id_set_name(sid, selem_name);
  snd_mixer_elem_t *elem = snd_mixer_find_selem(mixerHandle, sid);

  snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
  snd_mixer_selem_set_playback_volume_all(elem, volume * max / 100);

  snd_mixer_close(mixerHandle);
}

void Volume_init(void)
{
  assert(!s_initialized);
  Volume_setVolume(DEFAULT_VOLUME);

  s_initialized = true;
}

void Volume_cleanup(void)
{
  assert(s_initialized);

  s_initialized = false;
}
