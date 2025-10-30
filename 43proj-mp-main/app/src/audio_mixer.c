#include <alsa/asoundlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <limits.h>
#include <alloca.h> // needed for mixer
#include <mpg123.h>

#include "audio_mixer.h"

#include "visualizer.h"
#include "audio_datatypes.h"


#define DEFAULT_VOLUME 80

#define SAMPLE_RATE 44100
#define NUM_CHANNELS 2
#define SAMPLE_SIZE (sizeof(short)) 			// bytes per sample
#define INITIAL_BUFFER_SIZE 640000

// Prototypes
// static void changeIndex(int* index, int d, int max);


// Currently active (waiting to be played) sound bites
#define MAX_SOUND_BITES 30
typedef struct {
	// A pointer to a previously allocated sound bite (soundData_t struct).
	// Note that many different sound-bite slots could share the same pointer
	// (overlapping cymbal crashes, for example)
	soundData_t *pSound;

	// The offset into the pData of pSound. Indicates how much of the
	// sound has already been played (and hence where to start playing next).
	int location;
} playbackSound_t;

typedef struct {
	musicData_t *pMusic;
	int location;
} playbackMusic_t;

typedef struct
{
    short* buffer; 
    size_t soundsBufferSize;
} playbackBuffer_t;

static bool initialized = false;
static bool isPaused = false;

snd_pcm_t *pcmHandle;
snd_pcm_hw_params_t *params;
playbackBuffer_t playbackBuffer;

// Holds sound bites to be played. Can play multiple at once
static playbackSound_t soundBites[MAX_SOUND_BITES];
// Holds the songs to be played. 
static int musicBitesHead = 0;
static int musicBitesTail = 0;
static playbackMusic_t musicBites[MAX_SOUND_BITES];

// Playback threading
void* playbackThread();
static _Bool stopping = false;
static pthread_t playbackThreadId;
static pthread_mutex_t audioMutex = PTHREAD_MUTEX_INITIALIZER;


void AudioMixer_init(void)
{
	initialized = true;

	isPaused = false;

    mpg123_init();

	pthread_mutex_lock(&audioMutex);

	for (int i=0; i<MAX_SOUND_BITES; i++)
	{
		soundBites[i].pSound = NULL;
		soundBites[i].location = 0;

		musicBites[i].pMusic = NULL;
		musicBites[i].location = 0;
	}

	pthread_mutex_unlock(&audioMutex);

	// Open the PCM output
	int err = snd_pcm_open(&pcmHandle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (err < 0) {
		printf("Playback open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

    err = snd_pcm_set_params(pcmHandle,
        SND_PCM_FORMAT_S16_LE,
        SND_PCM_ACCESS_RW_INTERLEAVED,
        NUM_CHANNELS,
        SAMPLE_RATE,
        1,
        50000);


	// Allocate this software's playback buffer to be the same size as the
	// the hardware's playback buffers for efficient data transfers.
	// ..get info on the hardware buffers:
 	unsigned long unusedBufferSize = 0;
	snd_pcm_get_params(pcmHandle, &unusedBufferSize, &playbackBuffer.soundsBufferSize);
	// ..allocate playback buffer:
	playbackBuffer.buffer = malloc(playbackBuffer.soundsBufferSize * sizeof(short));

	// Launch playback thread:
	pthread_create(&playbackThreadId, NULL, playbackThread, NULL);
}


// Client code must call AudioMixer_freeWaveFileData to free dynamically allocated data.
void AudioMixer_readWaveFileIntoMemory(char *fileName, soundData_t *pSound)
{
	assert(initialized);
	assert(pSound);

	// The PCM data in a wave file starts after the header:
	const int PCM_DATA_OFFSET = 44;

	// Open the wave file
	FILE *file = fopen(fileName, "r");
	if (file == NULL) {
		fprintf(stderr, "ERROR: Unable to open file %s.\n", fileName);
		exit(EXIT_FAILURE);
	}

	// Get file size
	fseek(file, 0, SEEK_END);
	int sizeInBytes = ftell(file) - PCM_DATA_OFFSET;

	soundData_t pMonoSound;
	pMonoSound.numSamples = sizeInBytes / SAMPLE_SIZE;

	// Search to the start of the data in the file
	fseek(file, PCM_DATA_OFFSET, SEEK_SET);

	// Allocate space to hold all PCM data
	pMonoSound.pData = malloc(sizeInBytes);
	if (pMonoSound.pData == 0) {
		fprintf(stderr, "ERROR: Unable to allocate %d bytes for file %s.\n",
				sizeInBytes, fileName);
		exit(EXIT_FAILURE);
	}

	// Read PCM data from wave file into memory
	int samplesRead = fread(pMonoSound.pData, SAMPLE_SIZE, pMonoSound.numSamples, file);
	if (samplesRead != pMonoSound.numSamples) {
		fprintf(stderr, "ERROR: Unable to read %d samples from file %s (read %d).\n",
				pMonoSound.numSamples, fileName, samplesRead);
		exit(EXIT_FAILURE);
	}

	pSound->pData = malloc(sizeInBytes*2);
	pSound->numSamples = pMonoSound.numSamples;

	for (int i=0; i<pMonoSound.numSamples; i++)
	{
		pSound->pData[2*i] = pMonoSound.pData[i];
		pSound->pData[2*i+1] = pMonoSound.pData[i];
	}

	free(pMonoSound.pData);
}

void AudioMixer_readMp3FileIntoMemory(char *filename, musicData_t *pMusic, musicMetadata_t *pMetadata)
{
    assert(initialized);
    assert(pMusic);

    printf("Starting to read file into memory\n");

    mpg123_handle *mp3Handle = mpg123_new(NULL, NULL);

    // open MP3 file
    if (mpg123_open(mp3Handle, filename) != MPG123_OK)
    {
        perror("Failed to open MP3 file");
        return;
    }



    printf("creating buffer\n");

    size_t soundsBufferSize = mpg123_outblock(mp3Handle);
    unsigned char* buffer = (unsigned char*)malloc(soundsBufferSize);
    if (!buffer)
    {
        mpg123_close(mp3Handle);
        mpg123_delete(mp3Handle);
		perror("ERROR: Unable to allocate bytes for file");
		exit(EXIT_FAILURE);
    }

	// get length of song
	double lengthSeconds;

    long sampleRate;
    int channels, encoding;
	off_t totalFrames;
    mpg123_getformat(mp3Handle, (long*)&sampleRate, &channels, &encoding);
	totalFrames = mpg123_length(mp3Handle);
	lengthSeconds = (double)totalFrames / sampleRate;

    size_t pcm_capacity = INITIAL_BUFFER_SIZE;
    unsigned char *pcm_data = malloc(pcm_capacity);
    size_t pcm_size = 0;

    size_t bytesRead = 0;


    while (mpg123_read(mp3Handle, buffer, soundsBufferSize, &bytesRead) == MPG123_OK) 
    {        
        if (pcm_size + bytesRead > pcm_capacity)
        {
            size_t new_capacity = pcm_capacity * 2;
            if (new_capacity < pcm_size + bytesRead)
            {
                new_capacity = pcm_size + bytesRead;
            }

            unsigned char *temp = realloc(pcm_data, new_capacity);
            if (!temp) {
                perror("Failed to realloc memory");
                free(pcm_data);
                free(buffer);
                mpg123_close(mp3Handle);
                mpg123_delete(mp3Handle);
                exit(EXIT_FAILURE);
            }
            pcm_data = temp;  // Only assign if realloc succeeds
            pcm_capacity = new_capacity;
        }

        memcpy(pcm_data+pcm_size, buffer, bytesRead);
        pcm_size += bytesRead;
    }

	free(buffer);

	// Extract metadata
	char* title;
	char* artist;
	char* album;
	mpg123_id3v1* id3v1;
	mpg123_id3v2* id3v2;
	if (mpg123_id3(mp3Handle, &id3v1, &id3v2) == MPG123_OK)
	{
		if (id3v2)
		{
			title = id3v2->title && id3v2->title->p ? strdup(id3v2->title->p) : strdup("Unknown");
			artist = id3v2->artist && id3v2->artist->p ? strdup(id3v2->artist->p) : strdup("Unknown");
			album = id3v2->album && id3v2->album->p ? strdup(id3v2->album->p) : strdup("Unknown");
		}
		else if (id3v1)
		{
			title = strdup(id3v1->title);
			artist = strdup(id3v1->artist);
			album = strdup(id3v1->album);
		}
		else
		{
			title = strdup("Unknown");
			artist = strdup("Unknown");
			album = strdup("Unknown");
		}
	}
	else
	{
		title = strdup("Unknown");
		artist = strdup("Unknown");
		album = strdup("Unknown");
	}

    pMusic->pData = pcm_data;
    pMusic->numSamples = pcm_size / sizeof(short);
	pMusic->playingInMixer = false;

	pMetadata->title = title;
	pMetadata->artist = artist;
	pMetadata->album = album;
	pMetadata->lengthSeconds = lengthSeconds;


    mpg123_close(mp3Handle);
    mpg123_delete(mp3Handle);
    printf("done reading\n");

}

void AudioMixer_freeWaveFileData(soundData_t *pSound)
{
	assert(initialized);
	pSound->numSamples = 0;
	free(pSound->pData);
	pSound->pData = NULL;
}

void AudioMixer_freeMp3FileData(musicData_t *pMusic)
{
	assert(initialized);
	pMusic->numSamples = 0;
	free(pMusic->pData);
	pMusic->pData = NULL;
	pMusic->playingInMixer = false;
}

void AudioMixer_freeMp3MetaData(musicMetadata_t *pMetadata)
{
	free(pMetadata->title);
	pMetadata->title = NULL;
	free(pMetadata->artist);
	pMetadata->artist = NULL;
	free(pMetadata->album);
	pMetadata->album = NULL;
	pMetadata->lengthSeconds = 0;
}

void AudioMixer_queueSound(soundData_t *pSound)
{
	assert(initialized);
	// Ensure we are only being asked to play "good" sounds:
	assert(pSound->numSamples > 0);
	assert(pSound->pData);


	// Insert the sound by searching for an empty sound bite spot
	pthread_mutex_lock(&audioMutex);

	for (int i=0; i < MAX_SOUND_BITES; i++)
	{
		if (soundBites[i].pSound == NULL)
		{
			soundBites[i].pSound = pSound;
			soundBites[i].location = 0;
			pthread_mutex_unlock(&audioMutex);
			return;
		}
	}

	pthread_mutex_unlock(&audioMutex);

	printf("WARNING: No available slots for new sounds.\n");
	return;
}


void AudioMixer_clearSoundQueue(void)
{
	assert(initialized);

	pthread_mutex_lock(&audioMutex);

	for (int i=0; i < MAX_SOUND_BITES; i++)
	{
		soundBites[i].pSound = NULL;
		soundBites[i].location = 0;
	}

	pthread_mutex_unlock(&audioMutex);

	
	musicBitesHead = 0;
	musicBitesTail = 0;
}

void AudioMixer_queueMusic(musicData_t *pMusic)
{
	assert(initialized);
	// Ensure we are only being asked to play "good" sounds:
	assert(pMusic->numSamples > 0);
	assert(pMusic->pData);

	pthread_mutex_lock(&audioMutex);

	if (musicBites[musicBitesTail].pMusic != NULL)
	{

		printf("All music slots full\n");
		pthread_mutex_unlock(&audioMutex);
		return;
	}

	musicBites[musicBitesTail].pMusic = pMusic;
	musicBites[musicBitesTail].pMusic->playingInMixer = false;
	musicBites[musicBitesTail].location = 0;

	musicBitesTail = (musicBitesTail + 1) % MAX_SOUND_BITES;

	pthread_mutex_unlock(&audioMutex);
}

void AudioMixer_nextMusic()
{
	if (musicBites[musicBitesHead].pMusic != NULL)
	{
		pthread_mutex_lock(&audioMutex);
		musicBites[musicBitesHead].location = 0;
		musicBites[musicBitesHead].pMusic->playingInMixer = false;
		pthread_mutex_unlock(&audioMutex);
		musicBitesHead = (musicBitesHead + 1) % MAX_SOUND_BITES;
	}


	printf("%d\n", musicBitesHead);
}
void AudioMixer_prevMusic()
{

	int prevI = (musicBitesHead - 1 + MAX_SOUND_BITES) % MAX_SOUND_BITES;

	if (musicBites[musicBitesHead].pMusic != NULL)
	{
		if (musicBites[prevI].pMusic != NULL)
		{
			pthread_mutex_lock(&audioMutex);
			musicBites[musicBitesHead].location = 0;
			musicBites[musicBitesHead].pMusic->playingInMixer = false;
			// Negative modulo remains negative
			pthread_mutex_unlock(&audioMutex);
			musicBitesHead = prevI;
		}
		else
		{
			printf("No songs before. No action\n");
		}
	}
	else if (musicBites[prevI].pMusic != NULL)
	{
		musicBitesHead = prevI;
	}
	
	printf("%d\n", musicBitesHead);
}

void AudioMixer_restartMusic()
{
	pthread_mutex_lock(&audioMutex);
	musicBites[musicBitesHead].location = 0;
	pthread_mutex_unlock(&audioMutex);
}

void AudioMixer_clearMusicQueue(void)
{
	assert(initialized);

	pthread_mutex_lock(&audioMutex);

	for (int i=0; i < MAX_SOUND_BITES; i++)
	{
		musicBites[i].pMusic = NULL;
		musicBites[i].location = 0;
	}

	pthread_mutex_unlock(&audioMutex);

}

void AudioMixer_cleanup(void)
{
	assert(initialized);
	printf("Stopping audio...\n");
	initialized = false;

	// Stop the PCM generation thread
	stopping = true;
	pthread_join(playbackThreadId, NULL);

    mpg123_exit();

	// Shutdown the PCM output, allowing any pending sound to play out (drain)
	snd_pcm_drain(pcmHandle);
	snd_pcm_close(pcmHandle);

	// Free playback buffer
	// (note that any wave files read into soundData_t records must be freed
	//  in addition to this by calling AudioMixer_freeWaveFileData() on that struct.)
	free(playbackBuffer.buffer);
	playbackBuffer.buffer = NULL;

	printf("Done stopping audio...\n");
	fflush(stdout);
}

void AudioMixer_pauseMusic()
{
	isPaused = true;
}

void AudioMixer_resumeMusic()
{
	isPaused = false;
}

static void fillPlaybackBufferSounds(playbackBuffer_t *buff)
{
    size_t numFrames = buff->soundsBufferSize / NUM_CHANNELS;

	pthread_mutex_lock(&audioMutex);

	for (int i=0; i < MAX_SOUND_BITES; i++)
	{
		if (soundBites[i].pSound == NULL)
		{
			continue;
		}
		int offset = soundBites[i].location;
		short *data = soundBites[i].pSound->pData;
		size_t numSamples = soundBites[i].pSound->numSamples;
		for (size_t frame=0; frame<numFrames; frame++)
		{
			if ((offset + frame*NUM_CHANNELS) >= numSamples)
			{
				soundBites[i].pSound = NULL;
				soundBites[i].location = 0;
				break;
			}

            for (int ch=0; ch < NUM_CHANNELS; ch++)
            {
                size_t sampleIndex = frame * NUM_CHANNELS + ch;

                int mixedSample = buff->buffer[sampleIndex] + data[offset + frame*NUM_CHANNELS + ch];

                if (mixedSample > SHRT_MAX) mixedSample = SHRT_MAX;
                if (mixedSample < SHRT_MIN) mixedSample = SHRT_MIN;

                buff->buffer[sampleIndex] = (short)mixedSample;
            }

		}

		soundBites[i].location += numFrames * NUM_CHANNELS;
	}
	pthread_mutex_unlock(&audioMutex);

}

static void fillPlaybackBufferMusic(playbackBuffer_t *buff)
{
	if (musicBites[musicBitesHead].pMusic == NULL)
	{
		return;
	}

	pthread_mutex_lock(&audioMutex);
    size_t numFrames = buff->soundsBufferSize / NUM_CHANNELS;


	musicBites[musicBitesHead].pMusic->playingInMixer = true;

	int offset = musicBites[musicBitesHead].location;
	short *data = musicBites[musicBitesHead].pMusic->pData;
	size_t numSamples = musicBites[musicBitesHead].pMusic->numSamples;
	for (size_t frame=0; frame<numFrames; frame++)
	{
		if ((offset + frame*NUM_CHANNELS) >= numSamples)
		{
			musicBites[musicBitesHead].pMusic->playingInMixer = false;
			musicBites[musicBitesHead].pMusic = NULL;
			musicBites[musicBitesHead].location = 0;
			musicBitesHead = (musicBitesHead + 1) % MAX_SOUND_BITES;
			break;
		}

		for (int ch=0; ch < NUM_CHANNELS; ch++)
		{
			size_t sampleIndex = frame * NUM_CHANNELS + ch;

			int mixedSample = buff->buffer[sampleIndex] + data[offset + sampleIndex];

			if (mixedSample > SHRT_MAX) mixedSample = SHRT_MAX;
			if (mixedSample < SHRT_MIN) mixedSample = SHRT_MIN;

			Visualizer_setLEDArray((short)mixedSample);
			buff->buffer[sampleIndex] = (short)mixedSample;
		}
	}
	musicBites[musicBitesHead].location += numFrames * NUM_CHANNELS;
	pthread_mutex_unlock(&audioMutex);
}

double AudioMixer_getPlaytime(void) 
{
	double playedFrames = 0;
	pthread_mutex_lock(&audioMutex);
	if (musicBites[musicBitesHead].pMusic != NULL)
	{
		playedFrames = (double)musicBites[musicBitesHead].location / NUM_CHANNELS;
	}
	pthread_mutex_unlock(&audioMutex);

	return playedFrames / SAMPLE_RATE;
}

// Fill the buff array with new PCM values to output.
//    buff: buffer to fill with new PCM data from sound bites.
//    size: the number of *values* to store into buff
static void fillPlaybackBuffer(playbackBuffer_t *buff)
{
	assert(initialized);


	memset(buff->buffer, 0, buff->soundsBufferSize*sizeof(short));


	fillPlaybackBufferSounds(buff);

	if (!isPaused)
	{
		fillPlaybackBufferMusic(buff);
	}

	pthread_mutex_unlock(&audioMutex);
}


void* playbackThread()
{
	assert(initialized);
	while (!stopping) {
		// Generate next block of audio
		fillPlaybackBuffer(&playbackBuffer);

		// Output the audio
        // snd_pcm_prepare(pcmHandle);
		snd_pcm_sframes_t frames = snd_pcm_writei(pcmHandle,
				playbackBuffer.buffer, playbackBuffer.soundsBufferSize / NUM_CHANNELS);

		// Check for (and handle) possible error conditions on output
		if (frames < 0) {
			fprintf(stderr, "AudioMixer: writei() returned %li\n", frames);
			frames = snd_pcm_recover(pcmHandle, frames, 1);
		}
		if (frames < 0) {
			fprintf(stderr, "ERROR: Failed writing audio with snd_pcm_writei(): %li\n",
					frames);
			exit(EXIT_FAILURE);
		}
		if (frames > 0 && (unsigned long)frames < playbackBuffer.soundsBufferSize / NUM_CHANNELS) {
			printf("Short write (expected %li, wrote %li)\n",
					playbackBuffer.soundsBufferSize, frames);
		}
	}

	return NULL;
}

// static void changeIndex(int* index, int d, int max)
// {
//     *index = (*index + d) % max;
// }
