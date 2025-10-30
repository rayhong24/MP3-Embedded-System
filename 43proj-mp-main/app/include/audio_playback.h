#ifndef _AUDIO_PLAYBACK_H_
#define _AUDIO_PLAYBACK_H_

// Module will be incharge of audio playback.
// Users should be able to play, pause, skip, play previous, and change volume

#include "audio_datatypes.h"

typedef struct 
{
    char* filename;
    musicData_t* musicData;
    musicMetadata_t* metadata;
} sLoadedFile;

// Initializes Audio Playback module
void AudioPlayback_init(void);

// Cleans up the Audio Playback module
void AudioPlayback_cleanup(void);

void AudioPlayback_queueSong(sLoadedFile*);

int AudioPlayback_fillQueueInfo(char** strs, double* songlens, int bufLen, int numSongsToDisp);

void AudioPlayback_clearQueue(void);

// Loads the song to be played. Automatically adds the song to the queue. 
void AudioPlayback_loadSong(char*, sLoadedFile*);

// Pauses music playback
void AudioPlayback_pauseMusic(void);

// Resumes music playback. Only starts music if it was previously paused
void AudioPlayback_resumeMusic(void);

// Skips the current song being played.
void AudioPlayback_skip(void);

// Restarts the current song
void AudioPlayback_restart(void);

// Plays the previous song
void AudioPlayback_previous(void);

// Sets the volume (int from 0-100) 
void AudioPlayback_setVolume(int);

musicMetadata_t* AudioPlayback_getCurrentMetadata(void);


double AudioPlayback_getSongPlaytime(void);

#endif