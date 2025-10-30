#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H

// Module allows playing and loading multiple mp3 files. Also can support .wav files.

#define AUDIOMIXER_MAX_VOLUME 100

#include "audio_datatypes.h"
// init() must be called before any other functions,
// cleanup() must be called last to stop playback threads and free memory.
void AudioMixer_init(void);
void AudioMixer_cleanup(void);

// Read the contents of a wave file into the pSound structure. Note that
// the pData pointer in this structure will be dynamically allocated in
// readWaveFileIntoMemory(), and is freed by calling freeWaveFileData().
void AudioMixer_readWaveFileIntoMemory(char *fileName, soundData_t *pSound);
void AudioMixer_readMp3FileIntoMemory(char *filename, musicData_t *pSound, musicMetadata_t *pMetadata);
void AudioMixer_freeWaveFileData(soundData_t *pSound);
void AudioMixer_freeMp3FileData(musicData_t *pSound);
void AudioMixer_freeMp3MetaData(musicMetadata_t *pMetadata);

// Queue up another sound bite to play as soon as possible.
void AudioMixer_queueSound(soundData_t *pSound);
void AudioMixer_clearSoundQueue(void);

void AudioMixer_queueMusic(musicData_t *pMusic);
void AudioMixer_nextMusic();
void AudioMixer_prevMusic();
void AudioMixer_restartMusic();
void AudioMixer_clearMusicQueue(void);

double AudioMixer_getPlaytime(void);

void AudioMixer_pauseMusic(void);
void AudioMixer_resumeMusic(void);

#endif
