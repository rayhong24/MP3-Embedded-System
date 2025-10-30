#include "hal/rotary_encoder.h"
#include "hal/gpio.h"
#include "../../app/include/volume.h"
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

// Pin config info: GPIO 16&17 (Rotary Encoder A&B)
//   $ gpiofind GPIO16
//   >> gpiochip2 7
//   $ gpiofind GPIO17
//   >> gpiochip2 8
#define GPIO_CHIP GPIO_CHIP_2
#define GPIO_LINE_NUMBER_A 7
#define GPIO_LINE_NUMBER_B 8

static bool isInitialized = false;

struct GpioLine *s_lineA = NULL;
struct GpioLine *s_lineB = NULL;
static atomic_int counter = 0;
static atomic_bool isClockwise = false;
static atomic_bool isCounterClockwise = false;
static bool isThreadActive = true;

static pthread_t rotaryThread;
static void *rotaryThread_Func(void *arg) {
  while (isThreadActive) {
    RotaryStateMachine_doState();
    // debug
    // printf("Counter at %+5d\n", counter);
    //  sleepForMs(500);
  }

  (void)arg;
  // return NULL;
  pthread_exit(NULL);
}

/*
 *    Define the Statemachine Data Structures
 */
struct stateEvent {
  struct state *pNextState;
  void (*action)(void);
};
struct state {
  struct stateEvent a_rising;
  struct stateEvent b_rising;
  struct stateEvent a_falling;
  struct stateEvent b_falling;
};

/*
 *    START STATEMACHINE
 */
// static void on_release(void) { counter++; }
static void set_clockwise(void) {
  isClockwise = true;
  isCounterClockwise = false;
}
static void set_counterclockwise(void) {
  isCounterClockwise = true;
  isClockwise = false;
}
static void reset_clockwise(void) {
  if (isClockwise) {
    Volume_increaseVolume();
  }
  isClockwise = false;
  isCounterClockwise = false;
  // debug
  // printf("Current counter done by CW++ : %d\n", counter);
}
static void reset_counterclockwise(void) {
  if (isCounterClockwise) {
    Volume_decreaseVolume();
  }
  isClockwise = false;
  isCounterClockwise = false;
  // printf("Current counter done by CCW-- : %d\n", counter);
}

static struct state states[] = {
  {
    // At Rest
    .a_rising = {&states[0], NULL},
    .b_rising = {&states[0], NULL},
    .a_falling = {&states[1], set_clockwise},
    .b_falling = {&states[3], set_counterclockwise},
  },

  {
    // State #1
    .a_rising = {&states[0], reset_counterclockwise},
    .b_rising = {&states[1], NULL},
    .a_falling = {&states[1], NULL},
    .b_falling = {&states[2], NULL},
  },

  {
    // State #2
    .a_rising = {&states[3], NULL},
    .b_rising = {&states[1], NULL},
    .a_falling = {&states[2], NULL},
    .b_falling = {&states[2], NULL},
  },

  {
    // State #3
    .a_rising = {&states[3], NULL},
    .b_rising = {&states[0], reset_clockwise},
    .a_falling = {&states[2], NULL},
    .b_falling = {&states[3], NULL},
  },
};
/*
 *    END STATEMACHINE
 */

static struct state *pCurrentState = &states[0];

void RotaryStateMachine_init(void) {
  assert(!isInitialized);
  s_lineA = Gpio_openForEvents2(GPIO_CHIP, GPIO_LINE_NUMBER_A);
  s_lineB = Gpio_openForEvents2(GPIO_CHIP, GPIO_LINE_NUMBER_B);
  isThreadActive = true;
  isInitialized = true;
  pthread_create(&rotaryThread, NULL, rotaryThread_Func, NULL);
}
void RotaryStateMachine_cleanup(void) {
  assert(isInitialized);
  isThreadActive = false;
  // pthread_join(rotaryThread, NULL);
  pthread_detach(rotaryThread);
  isInitialized = false;
  Gpio_close2(s_lineA);
  Gpio_close2(s_lineB);
}

int RotaryStateMachine_getValue(void) { return counter; }

// TODO: This should be on a background thread!
void RotaryStateMachine_doState(void) {
  assert(isInitialized);

  // printf("\n\nWaiting for an event...\n");
  //  while (true) {
  struct gpiod_line_bulk bulkEvents;
  int numEvents = Gpio_waitForMultiLineChange(s_lineA, s_lineB, &bulkEvents);

  // Iterate over the event
  for (int i = 0; i < numEvents; i++) {
    // Get the line handle for this event
    struct gpiod_line *line_handle = gpiod_line_bulk_get_line(&bulkEvents, i);

    // Get the number of this line
    unsigned int this_line_number = gpiod_line_offset(line_handle);

    // Get the line event
    struct gpiod_line_event event;
    if (gpiod_line_event_read(line_handle, &event) == -1) {
      perror("Line Event");
      exit(EXIT_FAILURE);
    }

    // Run the state machine
    bool isRising = event.event_type == GPIOD_LINE_EVENT_RISING_EDGE;

    // Can check with line it is, if you have more than one...
    bool isRotaryA = this_line_number == GPIO_LINE_NUMBER_A;
    bool isRotaryB = this_line_number == GPIO_LINE_NUMBER_B;
    // assert(isBtn);

    struct stateEvent *pStateEvent = NULL;

    if (!isClockwise && !isCounterClockwise) {
      // it is first line event
      // at rest state
      // set clockwise or ccw condition
      if (isRotaryA) {
        if (isRising) {
          // if a rise here, stay rest
          pStateEvent = &pCurrentState->a_rising;
        } else {
          // it is clockwise, A falls instead of rise first.
          pStateEvent = &pCurrentState->a_falling;
          // isClockwise = true;
        }
      } else if (isRotaryB) {
        if (isRising) {
          // if a rise here, stay rest
          pStateEvent = &pCurrentState->b_rising;
        } else {
          // it is counter clockwise. B falls instead of rise first.
          pStateEvent = &pCurrentState->b_falling;
          // isCounterClockwise = true;
        }
      }
    } else {
      // it is not first line event.
      // and a clock cycle is set.
      // assume its either in state 1 or 3, maybe 2
      if (isRotaryA) {
        // it is A signal part of Rotary.
        if (isRising) {
          pStateEvent = &pCurrentState->a_rising;
        } else {
          pStateEvent = &pCurrentState->a_falling;
        }
      } else if (isRotaryB) {
        // it is B signal part of Rotary.
        if (isRising) {
          pStateEvent = &pCurrentState->b_rising;
        } else {
          pStateEvent = &pCurrentState->b_falling;
        }
      }
    }
    // Do the action
    if (pStateEvent->action != NULL) {
      pStateEvent->action();
    }
    pCurrentState = pStateEvent->pNextState;
  }
  // }
}
