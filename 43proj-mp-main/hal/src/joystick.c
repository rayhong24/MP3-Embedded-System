#include "hal/joystick.h"
#include "hal/i2c_manager.h"
#include "../../app/include/app.h"
// #include "../../app/include/volume.h"
#include "../../app/include/timing.h"
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
// Device bus & address
// address = ADC = 48
#define I2CDRV_LINUX_BUS "/dev/i2c-1"
#define I2C_DEV_ADDR 0x48
// Register in TLA2024
// 0x01 - write - continuously track data
// 0x00 - read current voltage
#define REG_CONF 0x01
#define REG_DATA 0x00
// Conf reg contents for continuously sampling different channels
// 83C2 = Y axis,  83D2 = X axis
#define TLA2024_CHANNEL_CONF_0 0x83C2
#define TLA2024_CHANNEL_CONF_1 0x83D2

#define DIRECTION_BOUNDARY_LOW 750
#define DIRECTION_BOUNDARY_HIGH 950

static bool is_initialized = false;
static int i2c_joystick_fd = 0;

#include "../../app/include/audio_playback.h"
// Y-Axis
// max = 1648, min = 42, stationary = 822-867
// stationary stable = 750-950
// X-Axis
// max = 1613-1640, min = 4, stationariy = 840-864
// stationary stable = 770-930
// final together stable = 750-950
static bool isThreadActive = true;
static pthread_t joystickThread;
static void *joystickThread_Func(void *arg) {
  while (isThreadActive) {
    // write to register 0x01 to continuously sample
    i2cManager_write_i2c_reg16(i2c_joystick_fd, REG_CONF, TLA2024_CHANNEL_CONF_0);
    uint16_t joyNumX = i2cManager_readInput(i2c_joystick_fd, REG_DATA);
    JoystickDirection curDirectionX = Joystick_setDirection(joyNumX);

    i2cManager_write_i2c_reg16(i2c_joystick_fd, REG_CONF, TLA2024_CHANNEL_CONF_1);
    uint16_t joyNumY = i2cManager_readInput(i2c_joystick_fd, REG_DATA);
    JoystickDirection curDirectionY = Joystick_setDirection(joyNumY);

    if (curDirectionX == JOYSTICK_HIGH)
      App_joystickLeft();
    else if(curDirectionX == JOYSTICK_LOW)
      App_joystickRight();
    else if(curDirectionY == JOYSTICK_HIGH)
      App_joystickUp();
    else if(curDirectionY == JOYSTICK_LOW)
      App_joystickDown();
    

    if(curDirectionX != JOYSTICK_STATION || curDirectionY != JOYSTICK_STATION) {
      // printf("it's STATIC! no movement\n");
      sleepForMs(200);
    }
    
    sleepForMs(10);
  }

  (void)arg;
  // return NULL;
  pthread_exit(NULL);
}

JoystickDirection Joystick_setDirection(int scaledNum) {
  JoystickDirection directionResult;
  JoystickDirection highEnd = JOYSTICK_LOW;
  JoystickDirection lowEnd = JOYSTICK_HIGH;

  if (scaledNum > DIRECTION_BOUNDARY_HIGH) {
    directionResult = highEnd;
  } else if (scaledNum < DIRECTION_BOUNDARY_LOW) {
    directionResult = lowEnd;
  } else {
    directionResult = JOYSTICK_STATION;
  }
  // debug
  return directionResult;
}

void Joystick_init(void) {
  assert(!is_initialized);

  // init joystick device
  i2c_joystick_fd = i2cManager_init_i2c_bus(I2CDRV_LINUX_BUS, I2C_DEV_ADDR);

  isThreadActive = true;
  is_initialized = true;
  pthread_create(&joystickThread, NULL, joystickThread_Func, NULL);
}

void Joystick_cleanup(void) {
  assert(is_initialized);
  isThreadActive = false;
  pthread_detach(joystickThread);
  close(i2c_joystick_fd);
  is_initialized = false;
}
