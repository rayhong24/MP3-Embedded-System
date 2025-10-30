#include "hal/accelerometer.h"
#include "hal/i2c_manager.h"
#include "hal/util/time_util.h"
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

// I2C address for accelerometer
#define I2C_DEVICE_ADDRESS 0x19

// define locations of accelerometer registers (split into 2 bytes)
// These are the LSB positions because we are reading 16 bits at a time which includes the MSB
#define X_REG 0x28
#define X_REG_HIGH 0x29
#define Y_REG 0x2A
#define Y_REG_HIGH 0x2B
#define Z_REG 0x2C
#define Z_REG_HIGH 0x2D

#define CTRL_REG 0x20

struct sAccelerationCtrlReg {
  uint8_t dataRateConfig      : 4;
  uint8_t modeSelect          : 2;
  uint8_t lowPowerModeSelect  : 2;
};

static bool s_initialized = false;
static int s_accFileDesc = 0;

/** Configure the accelerometer registers. */
static void Accelerometer_configForReading(void)
{
  struct sAccelerationCtrlReg accCtrl = {
    .dataRateConfig     = 0x2,  // 0b0010 - 12.5/1.6 Hz
    .modeSelect         = 0x1,  // 0b01   - High performance
    .lowPowerModeSelect = 0x3   // 0b11   - Low power mode 4
  };
  uint8_t accCtrlByte = *((uint8_t*) &accCtrl);
  i2cManager_write_i2c_reg8(s_accFileDesc, CTRL_REG, accCtrlByte);
}

static int8_t Accelerometer_normalizeData(int16_t data)
{
  // swap first and last byte
  int16_t swapped = (
    (data & 0x00FF) << 8) |
    ((data & 0xFF00) >> 8
  );
  return swapped; // truncate unused bits
}

/** Reads multiple samples at once. */
static void Accelerometer_readSamples(sAccelerationData_t* samples, uint8_t numSamples)
{
  for(int i = 0; i < numSamples; i++) {
    uint16_t x = i2cManager_read_i2c_reg16(s_accFileDesc, X_REG);
    uint16_t y = i2cManager_read_i2c_reg16(s_accFileDesc, Y_REG);
    uint16_t z = i2cManager_read_i2c_reg16(s_accFileDesc, Z_REG);

    sAccelerationData_t data = {
      .x = Accelerometer_normalizeData(x),
      .y = Accelerometer_normalizeData(y),
      .z = Accelerometer_normalizeData(z)
    };
    samples[i] = data;
  }
}

static sAccelerationData_t Accelerometer_avgSamples(sAccelerationData_t* samples, uint8_t sampleCount)
{
  int xSum = 0; int ySum = 0; int zSum = 0;
  for(int i = 0; i < sampleCount; i++) {
    xSum += samples[i].x;
    ySum += samples[i].y;
    zSum += samples[i].z;
  }

  sAccelerationData_t avg = {
    .x = xSum / sampleCount,
    .y = ySum / sampleCount,
    .z = zSum / sampleCount
  };
  return avg;
}

/**
 * Takes the average of the given samples.
 * Then, outliers are removed that are outside of 2 * standard deviation.
 */
static sAccelerationData_t Accelerometer_processSamples(sAccelerationData_t* samples, uint8_t numSamples)
{
  // get initial the average of the samples
  sAccelerationData_t avg = Accelerometer_avgSamples(samples, numSamples);
  
  // calculate the standard deviations for each axis
  float stdX = 0; float stdY = 0; float stdZ = 0;
  for(int i = 0; i < numSamples; i++) {
    stdX += (samples[i].x - avg.x) * (samples[i].x - avg.x);
    stdY += (samples[i].y - avg.y) * (samples[i].y - avg.y);
    stdZ += (samples[i].z - avg.z) * (samples[i].z - avg.z);
  }
  stdX = sqrt(stdX / numSamples);
  stdY = sqrt(stdY / numSamples);
  stdZ = sqrt(stdZ / numSamples);
  
  // remove outliers => samples < 2 * sd or samples > 2 * sd
  sAccelerationData_t filteredSamples[numSamples];
  int filteredCount = 0;
  for(int i = 0; i < numSamples; i++) {
    if(
      abs(samples[i].x - avg.x) < 2 * stdX &&
      abs(samples[i].y - avg.y) < 2 * stdY &&
      abs(samples[i].z - avg.z) < 2 * stdZ
    ) {
      filteredSamples[filteredCount] = samples[i];
      filteredCount++;
    }
  }
  
  if(filteredCount == 0) // no samples left, return the average
    return avg;

  // average the samples again without the outliers
  return Accelerometer_avgSamples(filteredSamples, filteredCount);
}

sAccelerationData_t Accelerometer_readAcceleration(uint8_t numSamples)
{
  assert(s_initialized);

  Accelerometer_configForReading();

  sAccelerationData_t samples[numSamples];
  Accelerometer_readSamples(samples, numSamples);
  return Accelerometer_processSamples(samples, numSamples);
}

void Accelerometer_init(void)
{
  assert(!s_initialized);

  s_accFileDesc = i2cManager_init_i2c_bus(I2CDRV_LINUX_BUS, I2C_DEVICE_ADDRESS);

  s_initialized = true;
}

void Accelerometer_cleanup(void)
{
  assert(s_initialized);
  s_initialized = false;
}
