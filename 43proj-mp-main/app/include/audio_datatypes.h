#ifndef _AUDIO_DATATYPES_H_
#define _AUDIO_DATATYPES_H_

#include <stdlib.h>
#include <stdbool.h>

typedef struct {
	char* title;
	char* artist;
	char* album;
	double lengthSeconds;
} musicMetadata_t;

typedef struct {
	int numSamples;
	short *pData;
	//bool playingInMixer;
} soundData_t;


typedef struct {
	size_t numSamples;
	void *pData;
	bool playingInMixer;

} musicData_t;

// typedef struct {
// 	int numSamples;
// 	short *pData;
// } wavedata_t;

#endif
