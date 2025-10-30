#ifndef _TILTING_H_
#define _TILTING_H_

/**
 * Starts tilting thread.
 * Listens to accelerometer and callsback on app module for the appropriate action.
 */
void Tilting_init(void);
void Tilting_cleanup(void);

#endif
