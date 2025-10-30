#include <stdbool.h>
#include <assert.h>
#include <alsa/asoundlib.h>
#include <mpg123.h>
#include <pthread.h>

#include "app.h"

#include "audio_datatypes.h"
#include "audio_playback.h"
#include "audio_mixer.h"

#define DEFAULT_NUM_CHANNELS 2 // stereo
#define DEFAULT_BITRATE 44100 // 44.1 kHz
#define MAX_SONGS_QUEUED 30

static bool initialized = false;
static sLoadedFile *pMusicQ[MAX_SONGS_QUEUED]; // circular array
static int pMusicHead = 0;
static int pMusicTail = 0;

// prototypes
static void AudioPlayback_updatePMusicIndex();

// Initializes Audio Playback module
void AudioPlayback_init(void)
{
    assert(!initialized);

    for (int i=0; i<MAX_SONGS_QUEUED; i++)
    {
        pMusicQ[i] = NULL;
    }

    initialized = true;
}

// Cleans up the Audio Playback module
void AudioPlayback_cleanup(void)
{
    assert(initialized);

    AudioPlayback_clearQueue();

    initialized = false;
}

void AudioPlayback_queueSong(sLoadedFile *pLoadedFile)
{
    pMusicQ[pMusicTail] = pLoadedFile;
    AudioMixer_queueMusic(pMusicQ[pMusicTail]->musicData);
    pMusicTail = (pMusicTail + 1) % MAX_SONGS_QUEUED;
}

int AudioPlayback_fillQueueInfo(char** strs, double* songlens, int bufLen, int numSongsToDisp)
{
    AudioPlayback_updatePMusicIndex();

    int selectedInd = 0;
    int i = pMusicHead;

    for (int j=1;j<8;j++)
    {
        int prev = (i-1+MAX_SONGS_QUEUED) % MAX_SONGS_QUEUED;
        if (pMusicQ[prev] != NULL && pMusicQ[prev]->musicData != NULL) i=prev;
        else break;
        selectedInd++;
    }

    // if (strs && songlens && bufLen) {} ;
    int arrInd = 0;

    // printf("head: %d, tail: %d\n", pMusicHead, pMusicTail);

    while (i != pMusicTail && arrInd < numSongsToDisp)
    {
        // printf("i: %d, arrInd: %d\n", i, arrInd);
        snprintf(strs[arrInd], bufLen, "%s", pMusicQ[i]->metadata->title);
        songlens[arrInd] = pMusicQ[i]->metadata->lengthSeconds;
        i = (i + 1) % MAX_SONGS_QUEUED;
        arrInd++;
    }

    // clear remaining strings
    arrInd = (arrInd + 1) % MAX_SONGS_QUEUED;
    while (arrInd < numSongsToDisp)
    {
        strs[arrInd][0] = '\0';
        arrInd = (arrInd + 1) % MAX_SONGS_QUEUED;
    }

    return selectedInd;
}

void AudioPlayback_clearQueue()
{
    AudioMixer_clearSoundQueue();
    AudioMixer_clearMusicQueue();
    for (int i=0; i<MAX_SONGS_QUEUED; i++)
    {
        pMusicQ[i] = NULL;
    }

    pMusicHead = 0;
    pMusicTail = 0;

}

// Loads the song to be played. Should automatically start playing.
void AudioPlayback_loadSong(char* filePath, sLoadedFile *pLoadedFile)
{
    assert(initialized);

    // Potential optimization: Check to make sure the song is not already loaded.

    printf("Loading new song: %s\n", filePath);

    if (pMusicQ[pMusicTail] != NULL)
    {
        printf("Too many songs in queue. Overwriting");
        pMusicQ[pMusicTail] = NULL;
    }

    AudioMixer_readMp3FileIntoMemory(filePath, pLoadedFile->musicData, pLoadedFile->metadata);
}




// Pauses music playback
void AudioPlayback_pauseMusic(void)
{
    AudioMixer_pauseMusic();
}

// Resumes music playback. Only starts music if it was previously paused
void AudioPlayback_resumeMusic(void)
{
    AudioMixer_resumeMusic();
}

// Skips the current song being played.
void AudioPlayback_skip(void)
{
    AudioMixer_nextMusic();
    AudioPlayback_updatePMusicIndex();
}

// Starts the song from the beginning. 
// If already in the first 3 seconds of the song, goes to previous song.
void AudioPlayback_previous(void)
{
    AudioMixer_prevMusic();
    AudioPlayback_updatePMusicIndex();
}

void AudioPlayback_restart(void)
{
    AudioMixer_restartMusic();
}

musicMetadata_t* AudioPlayback_getCurrentMetadata(void)
{
    if (pMusicQ[pMusicHead] != NULL)
    {
        AudioPlayback_updatePMusicIndex();
        return pMusicQ[pMusicHead]->metadata;
    }
    else 
    {
        return NULL;
    }
}

double AudioPlayback_getSongPlaytime(void)
{
    return AudioMixer_getPlaytime();
}


// Sets pMusicIndex to the songs currently being played
static void AudioPlayback_updatePMusicIndex()
{

    for (int i=0; i < MAX_SONGS_QUEUED; i++)
    {
        if (pMusicQ[i] != NULL && pMusicQ[i]->musicData !=NULL && pMusicQ[i]->musicData->playingInMixer)
        {
            pMusicHead = i;
            return;
        }
    }
}