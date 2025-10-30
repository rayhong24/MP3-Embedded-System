#ifndef _TESTS_H_
#define _TESTS_H_

// This module is for testing functionality of different programs
// This module assumes all required modules are initialized

// Detects button presses and prints to terminal.
void Tests_buttons(int seconds);

// Detects rotary encoder ticks and prints to terminal
void Tests_rotaryEncoder(int seconds);

void Tests_audioPlayback(void);

#endif