#include "effect_loader.h"
#include "audio_mixer.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define KICK_FILE "wave-files/100051__menegass__gui-drum-bd-hard.wav"
#define HIHAT_FILE "wave-files/100053__menegass__gui-drum-cc.wav"
#define SNARE_FILE "wave-files/100059__menegass__gui-drum-snare-soft.wav"
#define SNARE_HARD_FILE "wave-files/100058__menegass__gui-drum-snare-hard.wav"
#define CYMBAL_FILE "wave-files/100057__menegass__gui-drum-cyn-soft.wav"
#define TOMLOW_FILE "wave-files/100064__menegass__gui-drum-tom-lo-hard.wav"
#define TOMMID_FILE "wave-files/100066__menegass__gui-drum-tom-mid-hard.wav"

#define CLICK_FILE "wave-files/click.wav"
#define QUEUE_FILE "wave-files/queue.wav"
soundData_t theKick;
soundData_t theHihat;
soundData_t theSnare;
soundData_t theSnareHard;
soundData_t theCymbal;
soundData_t theTomLow;
soundData_t theTomMid;
soundData_t click;
soundData_t queue;

void EffectLoader_requestToBeQued(EffectCollection desiredSound) {
  if (desiredSound == EFFECT_SOUND1) {
    AudioMixer_queueSound(&click);
  }
  if (desiredSound == EFFECT_SOUND2) {
    AudioMixer_queueSound(&queue);
  }
  if (desiredSound == EFFECT_SOUND3) {
    AudioMixer_queueSound(&theSnareHard);
  }
}

static void EffectLoader_feedToMemory(void) {
  AudioMixer_readWaveFileIntoMemory(KICK_FILE, &theKick);
  AudioMixer_readWaveFileIntoMemory(HIHAT_FILE, &theHihat);
  AudioMixer_readWaveFileIntoMemory(SNARE_FILE, &theSnare);
  AudioMixer_readWaveFileIntoMemory(SNARE_HARD_FILE, &theSnareHard);
  AudioMixer_readWaveFileIntoMemory(CYMBAL_FILE, &theCymbal);
  AudioMixer_readWaveFileIntoMemory(TOMLOW_FILE, &theTomLow);
  AudioMixer_readWaveFileIntoMemory(TOMMID_FILE, &theTomMid);
  AudioMixer_readWaveFileIntoMemory(CLICK_FILE, &click);
  AudioMixer_readWaveFileIntoMemory(QUEUE_FILE, &queue);
}

void EffectLoader_init(void) {
  EffectLoader_feedToMemory();
}

void EffectLoader_cleanup(void) {
  AudioMixer_freeWaveFileData(&theTomMid);
  AudioMixer_freeWaveFileData(&theTomLow);
  AudioMixer_freeWaveFileData(&theCymbal);
  AudioMixer_freeWaveFileData(&theSnareHard);
  AudioMixer_freeWaveFileData(&theSnare);
  AudioMixer_freeWaveFileData(&theHihat);
  AudioMixer_freeWaveFileData(&theKick);

  AudioMixer_freeWaveFileData(&click);
  AudioMixer_freeWaveFileData(&queue);
}
