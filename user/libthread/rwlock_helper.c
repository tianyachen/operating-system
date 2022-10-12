/** @file rwlock_helper.c
 *  @brief a series of helper functions for the rwlock library implementation
 *
 *
 *  @author Carlos Montemayor (andrewid: cmontema, email: carl6256@gmail.com)
 */

#include <stddef.h>
#include <malloc.h>
#include <syscall.h>
#include <thread.h>
#include <rwlock.h>
#include <rwlock_internals.h>

/** @brief appends a rwlock request to the end of the rwlock's waiting queue
 *  @param  rwlock pointer to lock to append to
 *  @param  type the mode to try to aquire the lock in
 *
 *  requires caller owns the rwlock's data_mutex
 *
 *  @return 0 on success, -1 on error
 */
int append_rw( rwlock_t *rwlock, int type )
{
  if ( rwlock == NULL )
  {
    return -1;
  }

  if ( !(rwlock->valid) )
  {
    return -1;
  }

  thr_stack_meta_t* new_element = find_current_thread_meta();
  new_element->rw_type = type;
  rw_queue_t* header = (rwlock->waiting_rw);

  Q_INSERT_TAIL( header, new_element, rw_link );

  return 0;
}

/** @brief removes and wakes up appropriate threads off waiting queue
 *  @param rwlock pointer to the lock to dequeue waiting threads from
 *
 *  requires rwlock != NULL
 *         && rwlock->valid
 *         && caller owns the rwlock's data_mutex
 *
 *  @returns next rwlock state
 */
int dequeue( rwlock_t* rwlock )
{
  thr_stack_meta_t* next_waiting = Q_GET_FRONT( rwlock->waiting_rw );

  if ( next_waiting == NULL )
  {
    return RW_UNLOCKED;
  } else
  {
    int type = (next_waiting->rw_type);

    if ( RWLOCK_READ == type )
    {
      // dequeue all reads are part of the reader request prefix of the list
      while ( RWLOCK_READ == type )
      {
        thr_stack_meta_t* next  = Q_GET_NEXT( next_waiting, rw_link );
        Q_REMOVE( rwlock->waiting_rw, next_waiting, rw_link );
        next_waiting->rw_type = RWLOCK_INVALID;

        while ( make_runnable( next_waiting->tid ) < 0 )
        {
          thr_yield( next_waiting->tid );
        }

        if ( next == NULL )
        {
          break;
        }

        // prepare for next iteration
        next_waiting = next;
        type = (next_waiting->rw_type);
      }

      return RW_READING;
    }
    else if (RWLOCK_WRITE == type )
    {
      // dequeue only the next writer
      Q_REMOVE( rwlock->waiting_rw, next_waiting, rw_link );

      while ( make_runnable( next_waiting->tid )  < 0 )
      {
        thr_yield( next_waiting->tid );
      }

      return RW_WRITING;
    }
  }

  return RW_UNLOCKED;
}
