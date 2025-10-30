// Sample state machine for one GPIO pin.

#include "hal/btn_statemachine.h"
#include "hal/gpio.h"
#include "hal/util/time_util.h"

#include "../../app/include/app.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>
#include <pthread.h>
#include <poll.h>

// Pin config info: GPIO 24 (Rotary Encoder PUSH)
//   $ gpiofind GPIO24
//   >> gpiochip0 10
#define GPIO_ROTARY_CHIP          GPIO_CHIP_0
#define GPIO_ROTARY_LINE_NUMBER   10

#define GPIO_JOYSTICK_CHIP          GPIO_CHIP_2
#define GPIO_JOYSTICK_LINE_NUMBER   15


#define GPIO_WAIT_LINE_TIMEOUT_MS 250

// prototypes
static void* BtnStateMachine_doState();
static void BtnStateMachine_performEvent();

// variables
static bool isInitialized = false;
static bool checkEvents[NUM_BTN_STATEMACHINES] = {false, false};

static bool isVerbose[NUM_BTN_STATEMACHINES] = {false, false};

static sGpioLine_t* s_lineBtns[NUM_BTN_STATEMACHINES] = {NULL, NULL};
static sGpioLinesReq_t s_linesReqs[NUM_BTN_STATEMACHINES];
static atomic_int values[NUM_BTN_STATEMACHINES] = {0, 0};

static int maxCounts[NUM_BTN_STATEMACHINES] = {10000, 10000};
static bool cycleMachines[NUM_BTN_STATEMACHINES] = {false, false};


static pthread_t eventThreads[NUM_BTN_STATEMACHINES];
static enum eBtnStatemachines threadArgs[NUM_BTN_STATEMACHINES];

static enum eGpioChips statemachine_chipNumbers[NUM_BTN_STATEMACHINES] = {GPIO_ROTARY_CHIP, GPIO_JOYSTICK_CHIP};
static int statemachine_lineNumbers[NUM_BTN_STATEMACHINES] = {GPIO_ROTARY_LINE_NUMBER, GPIO_JOYSTICK_LINE_NUMBER};

static struct state* pCurrentStates[NUM_BTN_STATEMACHINES];



/*
    Define the Statemachine Data Structures
*/
struct Btn_stateEvent {
    struct state* pNextState;
    void (*action)();
};
struct state {
    struct Btn_stateEvent rising;
    struct Btn_stateEvent falling;
};


/*
    START STATEMACHINE
*/
static void on_release(enum eBtnStatemachines statemachine)
{
    if (cycleMachines[statemachine])
    {
        values[statemachine] = (values[statemachine]+1) % maxCounts[statemachine];

        if (statemachine == eROTARY_ENCODER_BUTTON_STATEMACHINE)
        {
            // Might cause issues with initialization errors in specific cases
            // Should be good enough though
            App_togglePlaybackStatus();
        }
    }
    else
    {
        if (statemachine == eJOYSTICK_BUTTON_STATEMACHINE)
        {
            App_joystickPressed();           
        }
        values[statemachine]++;
    }

    if (isVerbose[statemachine])
    {
        char* buttonNames[NUM_BTN_STATEMACHINES] = {"Rotary Button", "Joystick Button"};
        printf("BUTTON DEBUG: Press detected on %s\n", buttonNames[statemachine]);
        printf("BUTTON DEBUG: Cycle mode: %d, Current Value: %4d\n\n", cycleMachines[statemachine], values[statemachine]);
    }
}

struct state btnStates[] = {
    { // Not pressed
        .rising = {&btnStates[0], NULL},
        .falling = {&btnStates[1], NULL},
    },

    { // Pressed
        .rising = {&btnStates[0], on_release},
        .falling = {&btnStates[1], NULL},
    },
};
/*
    END STATEMACHINE
*/


void BtnStateMachine_init()
{
    assert(!isInitialized);


    s_lineBtns[eROTARY_ENCODER_BUTTON_STATEMACHINE] = Gpio_openForEvents(
        statemachine_chipNumbers[eROTARY_ENCODER_BUTTON_STATEMACHINE],
        statemachine_lineNumbers[eROTARY_ENCODER_BUTTON_STATEMACHINE]
    );
    s_lineBtns[eJOYSTICK_BUTTON_STATEMACHINE] = Gpio_openForEvents(
        statemachine_chipNumbers[eJOYSTICK_BUTTON_STATEMACHINE], 
        statemachine_lineNumbers[eJOYSTICK_BUTTON_STATEMACHINE]
    );

    isInitialized = true;
    for (int i=0; i<NUM_BTN_STATEMACHINES; i++)
    {
        Gpio_LinesRequest_init(&s_linesReqs[i]);
        Gpio_LinesRequest_addLine(&s_linesReqs[i], s_lineBtns[i]);
    }
}

void BtnStateMachine_cleanup()
{
    assert(isInitialized);
    for (int i=0; i<NUM_BTN_STATEMACHINES; i++)
    {
        assert(!checkEvents[i]);
    }



    for (int i=0; i<NUM_BTN_STATEMACHINES; i++)
    {
        Gpio_close(s_lineBtns[i]);
    }

    isInitialized = false;
}

int BtnStateMachine_getValue(enum eBtnStatemachines statemachine)
{
    assert(isInitialized);
    return values[statemachine];
}

void BtnStateMachine_setValue(enum eBtnStatemachines statemachine, int value)
{
    assert(isInitialized);

    values[statemachine] = value;
}

void BtnStateMachine_setCycleMode(enum eBtnStatemachines statemachine, int max, int startState)
{
    assert(isInitialized);
    assert(startState < max);

    values[statemachine] = startState;
    maxCounts[statemachine] = max;
    cycleMachines[statemachine] = true;
}

void BtnStateMachine_startEventChecks(enum eBtnStatemachines statemachine)
{
    assert(isInitialized);

    pCurrentStates[statemachine] = &btnStates[0];

    checkEvents[statemachine] = true;
    threadArgs[statemachine] = statemachine;
    if (pthread_create(&eventThreads[statemachine], NULL, BtnStateMachine_doState, &threadArgs[statemachine]) != 0)
    {
        perror("Failed to create thread");
        exit(EXIT_FAILURE);
    }
}

void BtnStateMachine_stopEventChecks(enum eBtnStatemachines statemachine)
{
    assert(isInitialized);
    assert(checkEvents[statemachine]);

    checkEvents[statemachine] = false;
    pthread_join(eventThreads[statemachine], NULL);
}

void BtnStateMachine_setVerbose(enum eBtnStatemachines statemachine, bool setVerbose)
{
    assert(isInitialized);
    isVerbose[statemachine] = setVerbose;
}

static enum eBtnStatemachines BtnStateMachine_getMachineFromLine(int lineNumber)
{
    for (int i=0; i<NUM_BTN_STATEMACHINES; i++)
    {
        if (statemachine_lineNumbers[i] == lineNumber)
        {
            return i;
        }
    }

    perror("Invalid lineNumber event found");
    exit(EXIT_FAILURE);
}


static void BtnStateMachine_performEvent(enum eBtnStatemachines statemachine)
{
  assert(isInitialized);

  struct timespec waitLineTimeout = ms_timespec(GPIO_WAIT_LINE_TIMEOUT_MS);

  sGpioLineBulk_t bulkEvents;
  unsigned int numEvents = Gpio_waitForLineChange(&s_linesReqs[statemachine], &bulkEvents, &waitLineTimeout);

  for(unsigned int eventNum = 0; eventNum < numEvents; eventNum++) {
    sGpioEventResult_t eventResult;
    Gpio_readLineEvent(&bulkEvents, eventNum, &eventResult);


    bool isRising = eventResult.event == RISING_EDGE;
    enum eBtnStatemachines machine = BtnStateMachine_getMachineFromLine(eventResult.lineNumber);

    struct Btn_stateEvent* pStateEvent = NULL;
    if (isRising) {
        pStateEvent = &pCurrentStates[machine]->rising;
    } else {
        pStateEvent = &pCurrentStates[machine]->falling;
    } 


    // Do the action
    if (pStateEvent->action != NULL) {
        pStateEvent->action(machine);
    }
    pCurrentStates[machine] = pStateEvent->pNextState;
  }
}
static void* BtnStateMachine_doState(void* arg)
{
    assert(isInitialized);

    enum eBtnStatemachines statemachine = *(enum eBtnStatemachines*)arg;

    while (checkEvents[statemachine])
    {
        BtnStateMachine_performEvent(statemachine);
    }

    return NULL;
}
