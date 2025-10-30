#ifndef _ROTARY_ENCODER_HAL_H_
#define _ROTARY_ENCODER_HAL_H_

void RotaryStateMachine_init(void);
void RotaryStateMachine_cleanup(void);

int RotaryStateMachine_getValue(void);

void RotaryStateMachine_doState(void);

#endif
