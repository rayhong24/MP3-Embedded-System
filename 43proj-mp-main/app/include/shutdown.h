#ifndef _SHUTDOWN_H_
#define _SHUTDOWN_H_

#include <stdbool.h>

void shutdown_init(void); 
void shutdown_cleanUp(void); 

void shutdown_waitForShutdown(void);
void shutdown_triggerShutdown(void);
bool shutdown_isShutdown(void);

#endif
