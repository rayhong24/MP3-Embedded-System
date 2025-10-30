#include "hal/gpio.h"
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include <errno.h>
#include <string.h>

static const char* const CHIP_NAMES[GPIO_NUM_CHIPS] = {
  "gpiochip0",
  "gpiochip1",
  "gpiochip2"
};

static bool s_initialized = false;
static struct gpiod_chip* s_openGpiodChips[GPIO_NUM_CHIPS];
static unsigned int s_numLineEvents = 0;

void Gpio_init(void)
{
  assert(!s_initialized);

  for (int i = 0; i < GPIO_NUM_CHIPS; i++) {
    // open GPIO chip
    s_openGpiodChips[i] = gpiod_chip_open_by_name(CHIP_NAMES[i]);
    if (!s_openGpiodChips[i]) {
      perror("GPIO Initializing: Unable to open GPIO chip");
      exit(EXIT_FAILURE);
    }
  }

  s_initialized = true;
}

void Gpio_LinesRequest_init(sGpioLinesReq_t* linesReq)
{
  assert(s_initialized);

  linesReq->numLines = 0;
}

void Gpio_LinesRequest_addLine(sGpioLinesReq_t* linesReq, sGpioLine_t* line)
{
  assert(s_initialized);
  assert(linesReq->numLines < GPIOD_LINE_BULK_MAX_LINES);

  linesReq->lines[linesReq->numLines] = line;
  linesReq->numLines++;
}

sGpioLine_t* Gpio_openForEvents(enum eGpioChips chip, int pinNum)
{
  assert(s_initialized);

  struct gpiod_chip* gpiodChip = s_openGpiodChips[chip];
  struct gpiod_line* line = gpiod_chip_get_line(gpiodChip, pinNum);
  if (!line) {
    perror("Unable to open GPIO line");
    exit(EXIT_FAILURE);
  }

  return (sGpioLine_t*) line;
}
// Opening a pin gives us a "line" that we later work with.
//  chip: such as GPIO_CHIP_0
//  pinNumber: such as 15
struct GpioLine *Gpio_openForEvents2(enum eGpioChips chip, int pinNumber) {
  assert(s_initialized);
  struct gpiod_chip *gpiodChip = s_openGpiodChips[chip];
  struct gpiod_line *line = gpiod_chip_get_line(gpiodChip, pinNumber);
  if (!line) {
    perror("Unable to get GPIO line");
    exit(EXIT_FAILURE);
  }

  return (struct GpioLine *)line;
}

unsigned int Gpio_waitForLineChange(
  sGpioLinesReq_t* lines,
  sGpioLineBulk_t* bulkEvents,
  struct timespec* timeout
)
{
  assert(s_initialized);

  if(lines->numLines == 0)
    return -1;

  // Source: https://people.eng.unimelb.edu.au/pbeuchat/asclinic/software/building_block_gpio_encoder_counting.html
  struct gpiod_line_bulk bulkWait;
  gpiod_line_bulk_init(&bulkWait);

  for (unsigned short i = 0; i < lines->numLines; i++) {
    sGpioLine_t* line = lines->lines[i];
    gpiod_line_bulk_add(&bulkWait, (struct gpiod_line*) line);
  }

  gpiod_line_request_bulk_both_edges_events(&bulkWait, "Event Waiting");

  int result = gpiod_line_event_wait_bulk(&bulkWait, timeout, (struct gpiod_line_bulk*) bulkEvents);
  if(result == -1) {
    perror("Error waiting on lines for event waiting");
    printf("Error code: %d (%s)\n", errno, strerror(errno));

    exit(EXIT_FAILURE);
  }
  if(result == 0) return 0;

  unsigned int numLines = gpiod_line_bulk_num_lines((struct gpiod_line_bulk*) bulkEvents);
  s_numLineEvents = numLines;
  return numLines;
}

void Gpio_readLineEvent(
  sGpioLineBulk_t* bulkEvents,
  unsigned int lineEvent,
  sGpioEventResult_t* eventResult
)
{
  assert(s_initialized);
  assert(lineEvent < s_numLineEvents);

  // get line handle for the event
  struct gpiod_line* lineHandle = gpiod_line_bulk_get_line(bulkEvents, lineEvent);
  // get the line number of the event
  unsigned int eventLineNumber = gpiod_line_offset(lineHandle);
  // get the event type
  struct gpiod_line_event event;
  if(gpiod_line_event_read(lineHandle, &event) == -1) {
    perror("Line Event");
    exit(EXIT_FAILURE);
  }

  eventResult->lineNumber = eventLineNumber;
  eventResult->event = event.event_type;
  eventResult->timestamp = event.ts;
}

void Gpio_close(sGpioLine_t* line)
{
  assert(s_initialized);
  gpiod_line_release((struct gpiod_line*) line);
}

void Gpio_close2(struct GpioLine *line) {
  assert(s_initialized);
  gpiod_line_release((struct gpiod_line *)line);
}

void Gpio_cleanup(void)
{
  assert(s_initialized);

  for (int i = 0; i < GPIO_NUM_CHIPS; i++) {
    // close GPIO chip
    gpiod_chip_close(s_openGpiodChips[i]);
    if (!s_openGpiodChips[i]) {
      perror("GPIO Initializing: Unable to open GPIO chip");
      exit(EXIT_FAILURE);
    }
  }

  s_initialized = false;
}

int Gpio_waitForMultiLineChange(
  struct GpioLine *line1,
  struct GpioLine *line2,
  struct gpiod_line_bulk *bulkEvents)
{
  assert(s_initialized);

  // Source:
  // https://people.eng.unimelb.edu.au/pbeuchat/asclinic/software/building_block_gpio_encoder_counting.html
  struct gpiod_line_bulk bulkWait;
  gpiod_line_bulk_init(&bulkWait);

  // TODO: Add more lines if needed
  gpiod_line_bulk_add(&bulkWait, (struct gpiod_line *)line1);
  gpiod_line_bulk_add(&bulkWait, (struct gpiod_line *)line2);

  gpiod_line_request_bulk_both_edges_events(&bulkWait, "Event Waiting");

  int result = gpiod_line_event_wait_bulk(&bulkWait, NULL, bulkEvents);
  if (result == -1) {
    perror("Error waiting on lines for event waiting");
    exit(EXIT_FAILURE);
  }

  int numEvents = gpiod_line_bulk_num_lines(bulkEvents);
  return numEvents;
                                }
