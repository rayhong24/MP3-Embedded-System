#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "tests.h"
#include "hal/btn_statemachine.h"
#include "hal/rotary_encoder.h"
#include "audio_playback.h"
#include "volume.h"
#include "file_loader.h"


void Tests_buttons(int seconds)
{
    printf("Detecting button presses for %d seconds\n", seconds);

    BtnStateMachine_setVerbose(eROTARY_ENCODER_BUTTON_STATEMACHINE, true);
    BtnStateMachine_setVerbose(eJOYSTICK_BUTTON_STATEMACHINE, true);

    BtnStateMachine_startEventChecks(eROTARY_ENCODER_BUTTON_STATEMACHINE);
    BtnStateMachine_startEventChecks(eJOYSTICK_BUTTON_STATEMACHINE);

    sleep(seconds);

    BtnStateMachine_stopEventChecks(eJOYSTICK_BUTTON_STATEMACHINE);
    BtnStateMachine_stopEventChecks(eROTARY_ENCODER_BUTTON_STATEMACHINE);

    BtnStateMachine_setVerbose(eROTARY_ENCODER_BUTTON_STATEMACHINE, false);
    BtnStateMachine_setVerbose(eJOYSTICK_BUTTON_STATEMACHINE, false);

    printf("Done testing buttons\n");
}

void Tests_rotaryEncoder(int seconds)
{
    printf("Detecting Rotary Encoder ticks for %d seconds\n", seconds);


    printf("Not implemented. Waiting for rotary_encoder module to use a separate thread.\n");



    printf("Done testing rotary encoder\n");

}

static void audioSleepPrintCurrentSongInfo(int seconds)
{
    for (int i=0; i<(seconds/3); i++)
    {

        musicMetadata_t* metadata = AudioPlayback_getCurrentMetadata();

        printf(
            "Current song details:\n\
                Title: %s\n\
                Artist: %s\n\
                Album: %s\n\
                Duration: %4.2f seconds\n", 
            metadata->title, metadata->artist, metadata->album, metadata->lengthSeconds);
        sleep(3);
    }

}

void Tests_audioPlayback(void)
{
    printf("Loading songs\n");

    sLoadedFile* lf0 = malloc(sizeof(sLoadedFile));
    sLoadedFile* lf1 = malloc(sizeof(sLoadedFile));
    sLoadedFile* lf2 = malloc(sizeof(sLoadedFile));
    sLoadedFile* lf3 = malloc(sizeof(sLoadedFile));

    FileLoader_initFileType(lf0);
    FileLoader_initFileType(lf1);
    FileLoader_initFileType(lf2);
    FileLoader_initFileType(lf3);


    AudioPlayback_loadSong("mp3-files/stylish-deep-electronic-262632.mp3", lf0);
    AudioPlayback_loadSong("mp3-files/lazy-day-stylish-futuristic-chill-239287.mp3", lf1);
    AudioPlayback_loadSong("mp3-files/lofi-background-music-309034.mp3", lf2);
    AudioPlayback_loadSong("mp3-files/never_gonna.mp3", lf3);

    audioSleepPrintCurrentSongInfo(10);

    printf("Pausing music for 3 seconds\n");
    AudioPlayback_pauseMusic();

    sleep(3);

    printf("Resuming...\n");
    AudioPlayback_resumeMusic();

    audioSleepPrintCurrentSongInfo(10);

    printf("skipping to next song\n");
    AudioPlayback_skip();

    audioSleepPrintCurrentSongInfo(10);

    printf("Playing previous\n");
    AudioPlayback_previous();

    audioSleepPrintCurrentSongInfo(10);

    printf("skipping 2 songs\n");
    AudioPlayback_skip();
    AudioPlayback_skip();

    audioSleepPrintCurrentSongInfo(10);

    printf("Restarting song\n");
    AudioPlayback_restart();

    audioSleepPrintCurrentSongInfo(10);


    AudioPlayback_clearQueue();

    FileLoader_freeFileType(lf0);
    FileLoader_freeFileType(lf1);
    FileLoader_freeFileType(lf2);
    FileLoader_freeFileType(lf3);

    free(lf0);
    free(lf1);
    free(lf2);
    free(lf3);


    printf("Done audio testing. Songs still in queue\n");
}