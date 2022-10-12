/** @file cond_type.h
 *  @brief This file defines the type for condition variables.
 */

#ifndef _COND_TYPE_H
#define _COND_TYPE_H

#include <stdbool.h>
#include <variable_queue.h>
#include <mutex_type.h>

/**  a queue type for keeping track of threads waiting on condition variable
 *
 */
typedef struct cond_queue_t cond_queue_t; // forward declaration

/** struct that encapsulates the type for the condition variable
 *  synchronization primitive
 */
typedef struct cond
{
  /* Basic idea here is to have a linked list queue of anything that's called
   * cond_wait on this , then we pick to the first to be signaled, allowing it
   * to grab its lock again*/
  unsigned char valid; ///< char telling whether conditional variable is valid

  /// NULL-terminated doubly-linked-list
  cond_queue_t* cond_queue;

  /// mutex to lock the thread queue
  mutex_t cond_mutex;

} cond_t;

#endif /* _COND_TYPE_H */
