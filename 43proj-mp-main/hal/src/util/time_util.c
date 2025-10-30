#include "hal/util/time_util.h"

static const long long NANO_TO_MILLI = 1000 * 1000;
static const long long NANO_TO_SEC = 1000000000;
static const long long MILLI_TO_SEC = 1000;

struct timespec ms_timespec(long milliseconds)
{
  long long delayNs = milliseconds * NANO_TO_MILLI;

  return (struct timespec) {
    .tv_sec = delayNs / NANO_TO_SEC,
    .tv_nsec = delayNs % NANO_TO_SEC
  };
}

struct timeval ms_timeval(long milliseconds)
{
  return (struct timeval) {
    .tv_sec = milliseconds / MILLI_TO_SEC,
    .tv_usec = milliseconds % MILLI_TO_SEC
  };
}
