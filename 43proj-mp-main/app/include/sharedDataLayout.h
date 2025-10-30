#ifndef _SHARED_DATA_STRUCT_H_
#define _SHARED_DATA_STRUCT_H_

#include <stdbool.h>
#include <stdint.h>

// R5 Shared Memory Note
// - It seems that using a struct for the ATCM memory does not work 
//   (hangs when accessing memory via a struct pointer).
// - Therefore, using an array.

#define MSG_OFFSET 0x6000
#define MSG_SIZE   32
#define LED_DELAY_MS_OFFSET (MSG_OFFSET + MSG_SIZE)
#define IS_BUTTON_PRESSED_OFFSET (LED_DELAY_MS_OFFSET + sizeof(uint32_t))
#define BTN_COUNT_OFFSET (IS_BUTTON_PRESSED_OFFSET + sizeof(uint32_t))
#define LOOP_COUNT_OFFSET (BTN_COUNT_OFFSET + sizeof(uint32_t))
#define END_MEMORY_OFFSET (LOOP_COUNT_OFFSET + sizeof(uint32_t))

#define MEM_UINT8(addr) *(uint8_t*)(addr)
#define MEM_UINT32(addr) *(uint32_t*)(addr)
#endif