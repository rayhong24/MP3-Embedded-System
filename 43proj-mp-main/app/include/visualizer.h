#include <stdio.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <sys/mman.h>

#include "sharedDataLayout.h"

// General R5 Memomry Sharing Routine
// ----------------------------------------------------------------
#define ATCM_ADDR     0x79000000  // MCU ATCM (p59 TRM)
#define BTCM_ADDR     0x79020000  // MCU BTCM (p59 TRM)
#define MEM_LENGTH    0x8000

#define RED_DIM 2
#define RED_BRIGHT 10
#define GREEN_DIM 1
#define GREEN_BRIGHT 9
#define BLUE_DIM 3
#define BLUE_BRIGHT 11
#define YELLOW_DIM 6
#define PURPLE_DIM 7
#define TEAL_DIM 8

#define NUM_LEDS 8
#define NUM_COLORS 6

void Visualizer_init();

void Visualizer_cleanup();

void Visualizer_setLEDArray(short val);
