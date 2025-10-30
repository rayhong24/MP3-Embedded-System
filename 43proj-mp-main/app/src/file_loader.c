#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <dirent.h>
#include <string.h>

#include "app.h"
#include "file_loader.h"

#define MUSIC_DIRECTORY "mp3-files"
#define MAX_FILE_STR_LEN 1024 


static void* loadThreadFunc();

static bool initialized = false;

static sLoadedFile* loadedFiles[MAX_NUM_LOADED_FILES];
static int nextFileInd = 0;

static bool runLoadThread = false;
static pthread_t loadThread;


void FileLoader_init(void)
{
    assert(!initialized);
    initialized = true;

    for (int i=0; i<MAX_NUM_LOADED_FILES; i++)
    {
        loadedFiles[i] = malloc(sizeof(sLoadedFile));
        FileLoader_initFileType(loadedFiles[i]);
    }

    runLoadThread = true;
    if (pthread_create(&loadThread, NULL, loadThreadFunc, NULL) != 0)
    {
        perror("Failed to spawn thread");
        exit(EXIT_FAILURE);
    }
}

void FileLoader_cleanup(void)
{
    assert(initialized);

    runLoadThread = false;
    pthread_join(loadThread, NULL);

    AudioPlayback_clearQueue();

    for (int i=0; i<MAX_NUM_LOADED_FILES; i++)
    {
        FileLoader_freeFileType(loadedFiles[i]);
        free(loadedFiles[i]);
    }

    initialized = false;
}

void FileLoader_queueFile(int i)
{
    if (loadedFiles[i] != NULL) AudioPlayback_queueSong(loadedFiles[i]);
}

void FileLoader_replaceFile(int i)
{
    if (loadedFiles[i] != NULL)
    {
        AudioPlayback_clearQueue();
    }
    AudioPlayback_queueSong(loadedFiles[i]);
}

void FileLoader_initFileType(sLoadedFile* pLoadedFile)
{
    assert(initialized);
    // pLoadedFile = malloc(sizeof(sLoadedFile));
    pLoadedFile->filename = "";
    pLoadedFile->musicData = malloc(sizeof(musicData_t));
    pLoadedFile->musicData->playingInMixer = false;
    pLoadedFile->metadata = malloc(sizeof(musicMetadata_t));
}

void FileLoader_freeFileType(sLoadedFile* pLoadedFile)
{
    assert(initialized);
    free(pLoadedFile->metadata);
    free(pLoadedFile->musicData);
}


static void* loadThreadFunc()
{
    // loop through all files and load them
    DIR *d;

    struct dirent *dir;

    d = opendir(MUSIC_DIRECTORY);

    if (d) {
        char fullPath[MAX_FILE_STR_LEN];
        while ((dir = readdir(d)) != NULL) {
            if (!runLoadThread) return NULL;
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;


            snprintf(fullPath, sizeof(fullPath), "%s/%s", MUSIC_DIRECTORY, dir->d_name);

            printf("%s\n", fullPath);

            AudioPlayback_loadSong(fullPath, loadedFiles[nextFileInd]);
            App_addMetadata(loadedFiles[nextFileInd]->metadata);

            nextFileInd++;
        }

        closedir(d);
    }

    return NULL;
}