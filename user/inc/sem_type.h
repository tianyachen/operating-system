/** @file sem_type.h
 *  @brief This file defines the type for semaphores.
 */

#ifndef _SEM_TYPE_H
#define _SEM_TYPE_H

#include <stdbool.h>
#include "mutex_type.h"
#include "cond_type.h"

/**  struct encapsulating type for semaphore syncronization primitive
 */
typedef struct sem {
  unsigned char valid; ///< char indicating initialization off semaphore
  int count;  ///< current magnitude of semaphore
  mutex_t lock; ///< internal mutex for making internal accesses atomic
  cond_t cv;  ///< internal condition variable for semaphore functionality
} sem_t;

#endif /* _SEM_TYPE_H */
