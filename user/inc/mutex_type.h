/** @file mutex_type.h
 *  @brief This file defines the type for mutexes.
 */

#ifndef _MUTEX_TYPE_H
#define _MUTEX_TYPE_H

#include <stdbool.h>

/** struct encapsulating the type for the mutex synchronization primitive
 */
typedef struct mutex {
  unsigned char valid; ///< char indicating init status of mutex
  unsigned int ticket_num; ///< record of current ticket_num of the mutex
  unsigned int turn;       ///< record of current turn
} mutex_t;

#endif /* _MUTEX_TYPE_H */
