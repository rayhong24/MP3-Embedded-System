#ifndef _ACCEROMETER_HAL_H_
#define _ACCEROMETER_HAL_H_

#include <stdint.h>

/** Structure to hold the acceleration data. */
typedef struct {
  int8_t x;
  int8_t y;
  int8_t z;
} sAccelerationData_t;

void Accelerometer_init(void);
void Accelerometer_cleanup(void);

sAccelerationData_t Accelerometer_readAcceleration(uint8_t numSamples);

#endif
