// Low-level GPIO access using gpiod
#ifndef _GPIO_H_
#define _GPIO_H_


#include <gpiod.h>

typedef struct sGpioLine sGpioLine_t;
/** Structure to request one or more GPIO lines */
typedef struct {
  sGpioLine_t* lines[GPIOD_LINE_BULK_MAX_LINES];
  unsigned short numLines;
} sGpioLinesReq_t;

typedef struct gpiod_line_bulk sGpioLineBulk_t;

/** Possible events for a GPIO line */
typedef enum {
  RISING_EDGE = GPIOD_LINE_EVENT_RISING_EDGE,
  FALLING_EDGE = GPIOD_LINE_EVENT_FALLING_EDGE
} eGpioEvent_t;

/** Structure with GPIO event results */
typedef struct {
  unsigned int lineNumber;
  eGpioEvent_t event;
  struct timespec timestamp;
} sGpioEventResult_t;

enum eGpioChips {
  GPIO_CHIP_0,
  GPIO_CHIP_1,
  GPIO_CHIP_2,
  GPIO_NUM_CHIPS
};

void Gpio_init(void);

/** Initializes a GPIO lines request structure */
void Gpio_LinesRequest_init(sGpioLinesReq_t* linesReq);
/** Adds a GPIO line to a GPIO lines request structure. */
void Gpio_LinesRequest_addLine(sGpioLinesReq_t* linesReq, sGpioLine_t* line);

// Opening a pin gives us a "line" that we later work with.
//  chip: such as GPIO_CHIP_0
//  pinNumber: such as 15
struct GpioLine* Gpio_openForEvents2(enum eGpioChips chip, int pinNumber);

/**
 * Opens a GPIO line for events.
 * Note: Gpio_close() to close the line.
 */
sGpioLine_t* Gpio_openForEvents(enum eGpioChips chip, int pinNum);

/**
 * Blocking call that waits for a GPIO line change event.
 * @param lines The line to wait for events.
 * @param bulkEvents
 * @param timeout The maximum time to wait for an event.
 */
unsigned int Gpio_waitForLineChange(
  sGpioLinesReq_t* lines,
  sGpioLineBulk_t* bulkEvents,
  struct timespec* timeout
);

/**
 * Retrieve event details from a GPIO line event.
 * @param[in] bulkEvents
 * @param[in] lineEvent The index of the event in the bulkEvents.
 * @param[out] eventResult
 */
void Gpio_readLineEvent(
  sGpioLineBulk_t* bulkEvents,
  unsigned int lineEvent,
  sGpioEventResult_t* eventResult
);

/** Close a GPIO line */
void Gpio_close(sGpioLine_t* line);
void Gpio_close2(struct GpioLine* line);

void Gpio_cleanup(void);

int Gpio_waitForMultiLineChange(
  struct GpioLine* line1,
  struct GpioLine* line2,
  struct gpiod_line_bulk *bulkEvents
);

#endif
