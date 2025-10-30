#ifndef _APP_H_
#define _APP_H_

// This module is responsible for binding all the other modules. It keeps track of the state of the program for example.

#include "audio_playback.h"

enum ePlaybackState
{
    eMUSIC_PLAYING,
    eMUSIC_PAUSED,
    eNUM_PLAYBACK_STATES,
};


void App_init(void);

void App_cleanup(void);

void App_addMetadata(musicMetadata_t*);

void App_joystickRight(void);
void App_joystickLeft(void);
void App_joystickUp(void);
void App_joystickDown(void);

void App_nextPage(void);
void App_prevPage(void);

void App_joystickPressed(void);

void App_updateVolume(int);

void App_togglePlaybackStatus();



#endif