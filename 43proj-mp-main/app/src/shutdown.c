#include "shutdown.h"
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

static pthread_mutex_t shutdown_key_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t shutdown_key_cond = PTHREAD_COND_INITIALIZER;
static bool isInitialized = false;
static bool isShutdown = false;

void shutdown_init(void) {
  assert(!isInitialized);
  isInitialized = true;

  pthread_mutex_lock(&shutdown_key_mutex);
}

void shutdown_waitForShutdown(void) {
  assert(isInitialized);

  while(!isShutdown)
    // wait for shutdown signal without busy waiting
    pthread_cond_wait(&shutdown_key_cond, &shutdown_key_mutex);
  
  pthread_mutex_unlock(&shutdown_key_mutex);
}

void shutdown_triggerShutdown(void) {
  assert(isInitialized);

  isShutdown = true;
  pthread_mutex_lock(&shutdown_key_mutex);
  pthread_cond_broadcast(&shutdown_key_cond);
  pthread_mutex_unlock(&shutdown_key_mutex);
}

bool shutdown_isShutdown(void)
{
  assert(isInitialized);

  pthread_mutex_lock(&shutdown_key_mutex);
  bool shutdown = isShutdown;
  pthread_mutex_unlock(&shutdown_key_mutex);

  return shutdown;
}

void shutdown_cleanUp(void) {
  assert(isInitialized);

  isInitialized = false;
}
