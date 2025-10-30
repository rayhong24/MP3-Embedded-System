#include "tilting.h"
#include "hal/accelerometer.h"
#include "hal/util/time_util.h"
#include "shutdown.h"
#include "app.h"
#include <stdbool.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define CLOCKS_PER_MS CLOCKS_PER_SEC / 1000

#define ACCELEROMETER_SAMPLE_PER_READ 15
#define TILTING_THREAD_TIMEOUT_MS 10

#define TILTING_X_AXIS_THRESHOLD 40
#define TILTING_Y_AXIS_THRESHOLD 40
#define TILTING_Z_AXIS_THRESHOLD 15

#define TIME_FOR_1_TILT_MS 500
#define TIME_FOR_REPEAT_TILTS_MS 3000

static bool initialized = false;
static pthread_t tiltingThread;

static void* Tilting_entry(void* arg)
{
  (void)arg;

  clock_t lastYTilt = 0;

  while(!shutdown_isShutdown()) {
    sAccelerationData_t acceleration = Accelerometer_readAcceleration(ACCELEROMETER_SAMPLE_PER_READ);
    
    // printf("X: %d, Y: %d, Z: %d\n", acceleration.x, acceleration.y, acceleration.z);

    bool zTilted = abs(acceleration.z) > TILTING_Z_AXIS_THRESHOLD;
    // check if the device is tilted. Device tilt depends on z-axis (gravity)
    bool isXTilted = (abs(acceleration.x) > TILTING_X_AXIS_THRESHOLD) && zTilted;
    bool isYTilted = (abs(acceleration.y) > TILTING_Y_AXIS_THRESHOLD) && zTilted;
    // if not tilted, wait minimal time and check again
    if(!isXTilted && !isYTilted && !zTilted) {
      struct timespec timeout = ms_timespec(TILTING_THREAD_TIMEOUT_MS);
      nanosleep(&timeout, NULL);
      continue;
    }

    // if y tilted, check that the tilt was shorter than 500ms
    if(isYTilted) {
      clock_t newYEvent = clock();
      double elapsed = (double)(newYEvent - lastYTilt) / CLOCKS_PER_MS;
      
      // if the tilt was longer than 500ms, ignore it (do not retrigger)
      // if longer than 1500ms, we want to trigger it again every 500ms
      if(elapsed >= TIME_FOR_1_TILT_MS && elapsed < TIME_FOR_REPEAT_TILTS_MS) {
        struct timespec timeout = ms_timespec(TILTING_THREAD_TIMEOUT_MS);
        nanosleep(&timeout, NULL);
        continue;
      }

      // tilt occurred
      // do not track it if we have been tilted a while, we want to start auto count
      if(elapsed >= TIME_FOR_REPEAT_TILTS_MS)
        lastYTilt = newYEvent;

      if(acceleration.y > 0) // guaranteed > 0 or < 0
        App_prevPage();
      else
        App_nextPage();

      // wait for 500ms to avoid retriggering
      struct timespec timeout = ms_timespec(TIME_FOR_1_TILT_MS);
      nanosleep(&timeout, NULL);
    }
  }

  return NULL;
}

void Tilting_init(void)
{
  assert(!initialized);
  initialized = true;

  if(pthread_create(&tiltingThread, NULL, Tilting_entry, NULL) != 0) {
    perror("Failed to create tilting thread");
    exit(1);
  }
}

void Tilting_cleanup(void)
{
  assert(initialized);

  if(pthread_join(tiltingThread, NULL) != 0) {
    perror("Failed to join tilting thread");
    exit(1);
  }
}
