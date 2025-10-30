#ifndef _BTN_STATEMACHINE_H_
#define _BTN_STATEMACHINE_H_

#include <stdbool.h>

enum eBtnStatemachines
{
    eROTARY_ENCODER_BUTTON_STATEMACHINE,
    eJOYSTICK_BUTTON_STATEMACHINE,
    NUM_BTN_STATEMACHINES
};

// Initializes the BtnStateMachine module
void BtnStateMachine_init(void);

// Cleans up the BtnStateMachine module
void BtnStateMachine_cleanup(void);

// Returns the current value of button.
int BtnStateMachine_getValue(enum eBtnStatemachines button);

// Sets the value of button
void BtnStateMachine_setValue(enum eBtnStatemachines button, int value);

// Starts checking for events on button
void BtnStateMachine_startEventChecks(enum eBtnStatemachines);

// Stops checking for events on button
void BtnStateMachine_stopEventChecks(enum eBtnStatemachines);

// Sets the button values to cycle.
// Can be used if the button is for cycling between states for example
void BtnStateMachine_setCycleMode(enum eBtnStatemachines button, int numStates, int startState);

// true - Prints when an event is detected.
// false - Runs normally without printing.
void BtnStateMachine_setVerbose(enum eBtnStatemachines, bool);

#endif