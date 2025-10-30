#include <assert.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pthread.h>

static pthread_mutex_t i2cLock = PTHREAD_MUTEX_INITIALIZER;

static void lockI2c() {
  pthread_mutex_lock(&i2cLock);
}

static void unlockI2c() {
  pthread_mutex_unlock(&i2cLock);
}

// initialization func
int i2cManager_init_i2c_bus(char *bus, int address) {
  int i2c_file_desc = open(bus, O_RDWR);
  if (i2c_file_desc == -1) {
    printf("I2C DRV: Unable to open bus for read/write (%s)\n", bus);
    perror("Error is:");
    exit(EXIT_FAILURE);
  }
  if (ioctl(i2c_file_desc, I2C_SLAVE, address) == -1) {
    perror("Unable to set I2C device to slave address.");
    exit(EXIT_FAILURE);
  }
  return i2c_file_desc;
}
// write to channel
void i2cManager_write_i2c_reg16(int i2c_file_desc, uint8_t reg_addr,
                                uint16_t value) {
  int tx_size = 1 + sizeof(value);
  uint8_t buff[tx_size];
  buff[0] = reg_addr;
  buff[1] = (value & 0xFF);
  buff[2] = (value & 0xFF00) >> 8;

  lockI2c();
  int bytes_written = write(i2c_file_desc, buff, tx_size);
  unlockI2c();

  if (bytes_written != tx_size) {
    perror("Unable to write i2c register");
    exit(EXIT_FAILURE);
  }
}
void i2cManager_write_i2c_reg8(int i2c_file_desc, uint8_t reg_addr,
                                uint8_t value) {
  int tx_size = 1 + sizeof(value);
  uint8_t buff[tx_size];
  buff[0] = reg_addr;
  buff[1] = value;

  lockI2c();
  int bytes_written = write(i2c_file_desc, buff, tx_size);
  unlockI2c();

  if (bytes_written != tx_size) {
    perror("Unable to write i2c register");
    exit(EXIT_FAILURE);
  }
                                }
// read register
uint16_t i2cManager_read_i2c_reg16(int i2c_file_desc, uint8_t reg_addr) {
  lockI2c();
  // To read a register, must first write the address
  int bytes_written = write(i2c_file_desc, &reg_addr, sizeof(reg_addr));
  if (bytes_written != sizeof(reg_addr)) {
    perror("Unable to write i2c register.");
    exit(EXIT_FAILURE);
  }
  // Now read the value and return it
  uint16_t value = 0;
  int bytes_read = read(i2c_file_desc, &value, sizeof(value));
  if (bytes_read != sizeof(value)) {
    perror("Unable to read i2c register");
    exit(EXIT_FAILURE);
  }
  unlockI2c();

  return value;
}
uint16_t i2cManager_read_i2c_reg8(int i2c_file_desc, uint8_t reg_addr) {
  lockI2c();
  // To read a register, must first write the address
  int bytes_written = write(i2c_file_desc, &reg_addr, sizeof(reg_addr));
  if (bytes_written != sizeof(reg_addr)) {
    perror("Unable to write i2c register.");
    exit(EXIT_FAILURE);
  }
  // Now read the value and return it
  uint8_t value = 0;
  int bytes_read = read(i2c_file_desc, &value, sizeof(value));
  if (bytes_read != sizeof(value)) {
    perror("Unable to read i2c register");
    exit(EXIT_FAILURE);
  }
  unlockI2c();
  
  return value;
}

static uint16_t i2cManager_human_readable(uint16_t rawData) {
  uint16_t scaled = 0;
  uint16_t raw_top = (rawData & 0xFF00) >> 8;
  uint16_t raw_bot = (rawData & 0x00FF) << 8;
  uint16_t human_readable = raw_top + raw_bot;
  scaled = human_readable >> 4;

  return scaled;
}
uint16_t i2cManager_readInput(int i2c_fileDesc, uint8_t reg_addr) {
  // read a register
  uint16_t scaledResult = 0;
  uint16_t raw_read = i2cManager_read_i2c_reg16(i2c_fileDesc, reg_addr);
  scaledResult = i2cManager_human_readable(raw_read);
  //printf("Raw reading: 0x%04x\n", raw_read);
  // final human readable - scaled
  //printf("Sacled reading: %8d\n", scaledResult);

    return scaledResult;
}
uint16_t i2cManager_readInputRaw(int i2c_fileDesc, uint8_t reg_addr) {
  // read a register
  uint16_t raw_read = i2cManager_read_i2c_reg16(i2c_fileDesc, reg_addr);
  //printf("Raw reading: 0x%04x\n", raw_read);
  // final human readable - scaled
  //printf("Sacled reading: %8d\n", scaledResult);

  return raw_read;
}
uint8_t i2cManager_readInput_reg8(int i2c_fileDesc, uint8_t reg_addr) {
  // read a register
  int16_t rawResult = 0;
  rawResult = i2cManager_read_i2c_reg8(i2c_fileDesc, reg_addr);
  //printf("Raw reading: 0x%04x\n", rawResult);
  // final human readable - scaled
  //printf("Sacled reading: %8d\n", scaledResult);

  return rawResult;
}
