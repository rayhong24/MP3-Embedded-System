#include "display.h"
#include <stdio.h>
#include "hal/rotary_encoder.h"
#include "hal/gpio.h"
#include "volume.h"
#include "tilting.h"
#include "shutdown.h"
#include "tests.h"
#include "hal/btn_statemachine.h"
#include "hal/rotary_encoder.h"
#include "hal/joystick.h"
#include "hal/gpio.h"
#include "hal/accelerometer.h"
#include <signal.h>
#include "audio_playback.h"
#include "visualizer.h"
#include <unistd.h>
#include "effect_loader.h"
#include "file_loader.h"
#include "app.h"

#include "audio_mixer.h"

const Display_Opts_t DISPLAY_OPTS = {
  .titleFont = &Font20,
  .songNameFont = &Font24,
  .artistFont = &Font16,
  .timeFont = &Font12,
  .volumeBarHeight = 25,
};

// Prototypes
static void init(void);
static void cleanup(void);

void signal_exit(int signo) {
  (void)signo;

  shutdown_triggerShutdown();
}

int main()
{
  init();
  
  printf("initialized\n");


  //wait for trigger shutdown
  shutdown_waitForShutdown();

  printf("Shutting down...\n");
  
  cleanup();

  return EXIT_SUCCESS;
}

void init(void) {
  system("/mnt/remote/myApps/r5/load_r5_mcu.sh");
  // Initialize all modules; HAL modules first
  Gpio_init();
  Joystick_init();
  RotaryStateMachine_init();
  BtnStateMachine_init();

  Visualizer_init();
  Accelerometer_init();
  
  Display_init(DISPLAY_OPTS);

  AudioMixer_init();
  AudioPlayback_init();
  EffectLoader_init();
  App_init(); 
  FileLoader_init();
  Volume_init();
  Tilting_init();
  

  signal(SIGINT, signal_exit);
  shutdown_init();
}
void cleanup(void) {
  // Cleanup all modules (HAL modules last)
  Tilting_cleanup();
  Volume_cleanup();
  App_cleanup();
  FileLoader_cleanup();
  EffectLoader_cleanup();
  AudioPlayback_cleanup();
  AudioMixer_cleanup();

  Display_cleanup();
  Accelerometer_cleanup();
  Visualizer_cleanup();
  
  BtnStateMachine_cleanup();
  RotaryStateMachine_cleanup();
  Joystick_cleanup();
  Gpio_cleanup();
}
