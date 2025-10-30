#ifndef _TIMING_H_
#define _TIMING_H_

void triggerReqDelay(void);
void timing_specDelay(long, long);
long long getTimeInMs(void); 
void sleepForMs(long long delayInMs);

#endif
