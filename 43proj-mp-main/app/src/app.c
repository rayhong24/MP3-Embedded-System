#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdatomic.h>

#include "hal/btn_statemachine.h"

#include "display.h"
#include "audio_playback.h"
#include "effect_loader.h"
#include "file_loader.h"
#include "app.h"
#include "volume.h"

#define DEFAULT_VOLUME 80
#define DEFAULT_PLAYBACK_STATE eMUSIC_PLAYING
#define MAX_SONG_NAME_LEN 64
// Make sure it matches the one in display.c
#define MAX_SONGS_DISP 7


// prototypes
static void* displayThreadFunc();

static bool initialized = false;

static int volume = 80;
static enum ePlaybackState currPlaybackState = DEFAULT_PLAYBACK_STATE;

static bool runDisplayThread = false;
static pthread_t displayThread;

static musicMetadata_t *loadedSongsMetadata[MAX_NUM_LOADED_FILES];
static int songMetaInd = 0;
static int selectedSong = 0;
// static int queueSelectedSong = 0;
static atomic_int currPage = MAIN;

static eMainPageElem mainSelected = PLAY;

char* dispStrs[MAX_SONGS_DISP];
double dispLengths[MAX_SONGS_DISP];
int dispWindowStart = 0;
int dispWindowEnd = MAX_SONGS_DISP;
int dispWindowSelected = 0; // value between 0 and MAX_SONGS_DISP

char* queueStrs[MAX_SONGS_DISP];
double queueLengths[MAX_SONGS_DISP];
int queueWindowStart = 0;
int queueWindowEnd = MAX_SONGS_DISP;
int queueWindowSelected = 0; // value between 0 and MAX_SONGS_DISP

void App_init(void)
{
    initialized = true;

    BtnStateMachine_setCycleMode(eROTARY_ENCODER_BUTTON_STATEMACHINE, eNUM_PLAYBACK_STATES, DEFAULT_PLAYBACK_STATE);
    BtnStateMachine_startEventChecks(eROTARY_ENCODER_BUTTON_STATEMACHINE);

    BtnStateMachine_startEventChecks(eJOYSTICK_BUTTON_STATEMACHINE);

    for (int i=0;i<MAX_SONGS_DISP; i++)
    {
        dispStrs[i] = malloc(MAX_SONG_NAME_LEN);
        queueStrs[i] = malloc(MAX_SONG_NAME_LEN);
        queueStrs[i][0] = '\0';
    }

    runDisplayThread = true;
    if (pthread_create(&displayThread, NULL, displayThreadFunc, NULL) != 0)
    {
        perror("Failed to create thread");
        exit(EXIT_FAILURE);
    }
}

void App_cleanup(void)
{
    assert(initialized);

    runDisplayThread = false;
    pthread_join(displayThread, NULL);

    for (int i=0;i<MAX_SONGS_DISP; i++)
    {
        free(dispStrs[i]);
        free(queueStrs[i]);
    }

    BtnStateMachine_stopEventChecks(eROTARY_ENCODER_BUTTON_STATEMACHINE);
    BtnStateMachine_stopEventChecks(eJOYSTICK_BUTTON_STATEMACHINE);

    initialized = false;
}

void App_updateVolume(int newVolume)
{
    assert(initialized);

    volume = newVolume;
}


void App_togglePlaybackStatus()
{
    if (currPlaybackState == eMUSIC_PLAYING)
    {
        AudioPlayback_pauseMusic();
        currPlaybackState = eMUSIC_PAUSED;
    }
    else
    {
        AudioPlayback_resumeMusic();
        currPlaybackState = eMUSIC_PLAYING;
    }
}

void App_setPlaybackStatus(enum ePlaybackState state)
{
    if (state == eMUSIC_PLAYING)
    {
        AudioPlayback_resumeMusic();
        currPlaybackState = eMUSIC_PLAYING;
    }
    else 
    {
        AudioPlayback_pauseMusic();
        currPlaybackState = eMUSIC_PAUSED;
    }
    
}

void App_addMetadata(musicMetadata_t *metadata)
{
    loadedSongsMetadata[songMetaInd] = metadata;
    songMetaInd++;
}

void App_joystickDown(void)
{

    printf("playback status: %d\n", currPlaybackState);
    if (currPage == MAIN)
    {
    }
    else if (currPage == FILES)
    {
        // Checks if last song
        if (selectedSong+1 < songMetaInd)
        {
            selectedSong++;
            EffectLoader_requestToBeQued(EFFECT_SOUND1);
            // Checks if at bottom of window
            if (selectedSong >= dispWindowEnd)
            {
                dispWindowStart++;
                dispWindowEnd++;
            }
        }

        if (dispWindowSelected < MAX_SONGS_DISP-1 && dispWindowSelected < songMetaInd-1)
        {dispWindowSelected++;}

        printf("selected song: %d\n", selectedSong);
    }
    else if (currPage == QUEUE)
    {

    }
    printf("Joystickick Down: selected song: %d\n", selectedSong);
}

void App_joystickUp(void)
{
    if (currPage == MAIN) return;
    if (selectedSong > 0)
    {
        selectedSong--;
        EffectLoader_requestToBeQued(EFFECT_SOUND1);
        if (selectedSong < dispWindowStart)
        {
            dispWindowStart--;
            dispWindowEnd--;
        }
    }
    if (dispWindowSelected > 0) dispWindowSelected--;

    printf("Joystickick Up: selected song: %d\n", selectedSong);
}

void App_joystickRight(void)
{
    printf("Joystick Right\n");
    if (currPage == MAIN)
    {
        if (mainSelected == NEXT) App_nextPage();
        else {
            mainSelected++;
            EffectLoader_requestToBeQued(EFFECT_SOUND1);
        }
    }
    else if (currPage == QUEUE)
    {
        App_nextPage();
        // printf("Right");
    }
    else if (currPage == FILES)
    {
        if (selectedSong < songMetaInd) FileLoader_queueFile(selectedSong);
        EffectLoader_requestToBeQued(EFFECT_SOUND2);

        printf("Queuing file #%d\n", selectedSong);
    }
    printf("Right\n");
    // printf("next page: %d\n", currPage);
}

void App_joystickLeft(void)
{
    printf("Joystick Left\n");
    if (currPage == MAIN)
    {
        if(mainSelected == PREV) currPage--;
        else {
            mainSelected--;
            EffectLoader_requestToBeQued(EFFECT_SOUND1);
        }
    }
    else if (currPage == FILES)
    {
        currPage--;
    }
}

void App_joystickPressed(void)
{
    // FileLoader_queueFile(selectedSong);
    // currently also clears queue
    printf("joystick Pressed\n");
    if (currPage == MAIN)
    {
        if (mainSelected == PREV) {AudioPlayback_previous(); printf("Prev");}
        else if (mainSelected == PLAY) 
        {
            printf("Toggle Play/Pause");
            App_togglePlaybackStatus();
        }
        else if (mainSelected == NEXT) {AudioPlayback_skip(); printf("Next");}
 
    }
    else if (currPage == FILES) 
    {
        printf("Loading File at index %d\n", selectedSong);
        FileLoader_replaceFile(selectedSong);
        App_setPlaybackStatus(eMUSIC_PLAYING);
    }
}

void App_nextPage()
{
    if (currPage < NUM_PAGES)
    {
        currPage++;
    }
}
void App_prevPage(void)
{
    if (currPage > 0)
    {
        currPage--;
    }
}

static void displayMainPage(void)
{

    Display_Request_t req = 
    {
        currPage,
        "MP3",
        "Title",
        "Artist",
        0,
        0,
        false,
        100,
        mainSelected
    };

    musicMetadata_t* metadata = AudioPlayback_getCurrentMetadata();
    if (metadata != NULL)
    {
        req.songName = metadata->title;
        req.author = metadata->artist;
        req.totalPlaytime = metadata->lengthSeconds;
        req.isPlaying = currPlaybackState == eMUSIC_PLAYING;
        req.elemSelected = mainSelected;
    }
    req.currPlaytime = AudioPlayback_getSongPlaytime();
    req.volume = volume;

    // printf("%d\n", volume);
    Display_updateHomeScreen(req);
}

static void displaySongListPage()
{

    int numSongsToDisp = songMetaInd<(dispWindowEnd-dispWindowStart) ? songMetaInd : dispWindowEnd - dispWindowStart;

    for (int i=0; i<numSongsToDisp; i++)
    {
        int j = dispWindowStart+i;
        snprintf(dispStrs[i], MAX_SONG_NAME_LEN, "%s", loadedSongsMetadata[j]->title);
        dispLengths[i] = loadedSongsMetadata[j]->lengthSeconds;
        // printf("%s\n", loadedSongsMetadata[i]->title);
    }

    File_List_Display_Request_t req = 
    {
        "Songs",
        dispStrs,
        dispLengths,
        songMetaInd,
        dispWindowSelected,
    };

    Display_updateSongSelectScreen(req);
}

static void displayQueuePage()
{
    // Should play queue
    int numSongsToDisp = songMetaInd<(queueWindowEnd-queueWindowStart) ? songMetaInd : queueWindowEnd - queueWindowStart;

    int playing = AudioPlayback_fillQueueInfo(queueStrs, queueLengths, MAX_SONG_NAME_LEN, numSongsToDisp);

    // printf("playing: %d\n", playing);
    File_List_Display_Request_t req = 
    {
        "QUEUE",
        queueStrs,
        queueLengths,
        songMetaInd,
        playing,
    };

    Display_updateQueueScreen(req);
}

static void* displayThreadFunc()
{
    while (runDisplayThread)
    {
        if (currPage == MAIN)
        {
            displayMainPage();
        }
        else if (currPage == FILES)
        {
            displaySongListPage();
        }
        else 
        {
            displayQueuePage();
        }
    }


    return NULL;
}
