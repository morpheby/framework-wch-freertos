
#include <stdlib.h>

#ifdef __PICOLIBC__

#include <malloc.h>
#include <errno.h>
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include "picolibc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

struct __lock {
    SemaphoreHandle_t mutex;
};

struct __lock __lock___libc_recursive_mutex = { NULL };

static uint32_t recursive_mutex_counter = 0;

void __retarget_lock_init(_LOCK_T *lock) {
  _LOCK_T l = malloc(sizeof(struct __lock));
  l->mutex = xSemaphoreCreateMutex();
  *lock = l;
}

void __retarget_lock_acquire(_LOCK_T lock) {
  if (lock == NULL) {
    // Assume it is a global lock
    assert(recursive_mutex_counter == 0);
    if (recursive_mutex_counter == 0) {
      vTaskSuspendAll();
    }
    ++recursive_mutex_counter;
  } else {
    xSemaphoreTake(lock->mutex, portMAX_DELAY);
  }
}

void __retarget_lock_release(_LOCK_T lock) {
  if (lock == NULL) {
    assert(recursive_mutex_counter == 1);
    --recursive_mutex_counter;
    if (recursive_mutex_counter == 0) {
      (void)xTaskResumeAll();
    }
  } else {
    xSemaphoreGive(lock->mutex);
  }
}

void __retarget_lock_close(_LOCK_T lock) {
  if (lock == NULL) {
    assert(recursive_mutex_counter == 0);
    return;
  }
  vSemaphoreDelete(lock->mutex);
  free(lock);
}

void __retarget_lock_init_recursive(_LOCK_T *lock) {
  _LOCK_T l = malloc(sizeof(struct __lock));
  l->mutex = xSemaphoreCreateRecursiveMutex();
  *lock = l;
}

void __retarget_lock_acquire_recursive(_LOCK_T lock) {
  if (lock == NULL) {
    // Assume it is a global lock
    if (recursive_mutex_counter == 0) {
      vTaskSuspendAll();
    }
    ++recursive_mutex_counter;
  } else {
    xSemaphoreTakeRecursive(lock->mutex, portMAX_DELAY);
  }
}

void __retarget_lock_release_recursive(_LOCK_T lock) {
  if (lock == NULL) {
    assert(recursive_mutex_counter > 0);
    --recursive_mutex_counter;
    if (recursive_mutex_counter == 0) {
      (void)xTaskResumeAll();
    }
  } else {
    xSemaphoreGiveRecursive(lock->mutex);
  }
}

void __retarget_lock_close_recursive(_LOCK_T lock) {
  if (lock == NULL) {
    assert(recursive_mutex_counter == 0);
    return;
  }
  vSemaphoreDelete(lock->mutex);
  free(lock);
}

#endif
