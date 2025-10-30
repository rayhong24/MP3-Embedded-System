#ifndef _FILE_LOADER_H_
#define _FILE_LOADER_H_

#include "audio_playback.h"

#define MAX_NUM_LOADED_FILES 256

// This module should load and store music data
// Loading should be done in the background so it doesn't hold up the app


void FileLoader_init(void);

void FileLoader_cleanup(void);

void FileLoader_queueFile(int);

void FileLoader_replaceFile(int);

void FileLoader_initFileType(sLoadedFile*);

void FileLoader_freeFileType(sLoadedFile* pLoadedFile);




#endif