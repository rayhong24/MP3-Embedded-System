#ifndef _TIME_UTIL_H_
#define _TIME_UTIL_H_

#include <sys/time.h>

struct timespec ms_timespec(long milliseconds);

struct timeval ms_timeval(long milliseconds);

#endif
