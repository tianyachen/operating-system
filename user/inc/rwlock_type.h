/** @file rwlock_type.h
 *  @brief This file defines the type for reader/writer locks.
 */

#ifndef _RWLOCK_TYPE_H
#define _RWLOCK_TYPE_H

#define RWLOCK_

#include <stdbool.h>
#include <thr_internals.h>
#include <mutex.h>

/** redefines queue header struct created with the
 *  variable queue library
 */
typedef struct rw_queue_t rw_queue_t;

/** encapsulates the reader writer lock structure
 */
typedef struct rwlock {
  bool valid; ///< boolean indicating whether lock is initialized or not

  /// mutex to protect access to the internal mutexes and count
  mutex_t data_mutex;

  /// waiting queue for threads that want to aquire the lock
  rw_queue_t* waiting_rw;

  // internal structure of rw_lock
  int reader_count; ///< number of readers currently holding the lock
  int mode; ///< lock mode, either reading, writing, or unlocked
} rwlock_t;

#endif /* _RWLOCK_TYPE_H */
