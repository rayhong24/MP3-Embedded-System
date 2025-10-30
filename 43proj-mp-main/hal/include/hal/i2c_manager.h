#ifndef _I2C_MANAGER_H_
#define _I2C_MANAGER_H_

#include <stdint.h>

#define I2CDRV_LINUX_BUS "/dev/i2c-1" // I2C bus 1 device

int i2cManager_init_i2c_bus(char *, int);
void i2cManager_write_i2c_reg16(int, uint8_t,
                                uint16_t);
void i2cManager_write_i2c_reg8(int, uint8_t,
                                uint8_t);
uint16_t i2cManager_read_i2c_reg16(int, uint8_t);
uint16_t i2cManager_read_i2c_reg8(int, uint8_t);
uint16_t i2cManager_readInput(int, uint8_t);
uint16_t i2cManager_readInputRaw(int, uint8_t);
uint16_t i2cManager_readInput_reg8(int, uint8_t);

void i2cManager_init(void);
void i2cManager_cleanup(void);


#endif
