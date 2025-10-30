#include "timing.h"
#include <time.h>

void triggerReqDelay(void) {
  // after a LED trigger, linux need time
  // to collect req files to user.
  // need 100ms delay each time trigger changes
  long seconds = 0;
  long nanoseconds = 150000000;
  struct timespec reqDelay = {seconds, nanoseconds};
  nanosleep(&reqDelay, (struct timespec *)NULL);
}
// specified delay when needed
void timing_specDelay(long sec, long nanosec) {
  // after a LED trigger, linux need time
  // to collect req files to user.
  // need 100ms delay each time trigger changes
  long seconds = sec;
  long nanoseconds = nanosec;
  struct timespec reqDelay = {seconds, nanoseconds};
  nanosleep(&reqDelay, (struct timespec *)NULL);
}
long long getTimeInMs(void) {
  struct timespec spec;
  clock_gettime(CLOCK_REALTIME, &spec);
  long long seconds = spec.tv_sec;
  long long nanoSeconds = spec.tv_nsec;
  long long milliSeconds = seconds * 1000 + nanoSeconds / 1000000;
  return milliSeconds;
}
void sleepForMs(long long delayInMs) {
  const long long NS_PER_MS = 1000 * 1000;
  const long long NS_PER_SECOND = 1000000000;

  long long delayNs = delayInMs * NS_PER_MS;
  int seconds = delayNs / NS_PER_SECOND;
  int nanoseconds = delayNs % NS_PER_SECOND;

  struct timespec reqDelay = {seconds, nanoseconds};
  nanosleep(&reqDelay, (struct timespec *)NULL);
}
