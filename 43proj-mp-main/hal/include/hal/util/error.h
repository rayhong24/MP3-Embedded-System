#ifndef _ERROR_UTIL_H_
#define _ERROR_UTIL_H_

#include <stdio.h>
#include <pthread.h>

#define error_exit(msg) do { \
  perror(msg); \
  pthread_exit(NULL); \
} while(0)

#endif
