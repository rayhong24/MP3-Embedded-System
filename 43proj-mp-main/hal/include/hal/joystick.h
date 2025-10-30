#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_

typedef enum { JOYSTICK_HIGH, JOYSTICK_LOW, JOYSTICK_STATION } JoystickDirection;

JoystickDirection Joystick_setDirection(int);
void Joystick_init(void);
void Joystick_cleanup(void);

#endif
