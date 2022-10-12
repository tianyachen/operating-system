/** @file rwlock.c
 *  @brief implementation of reader/writer lock functions
 *
 *  @author Carlos Montemayor   (andrewid: cmontema, email: carl6256@gmail.com)
 */

#include <stddef.h>
#include <syscall.h>
#include <malloc.h>
#include <error_code.h>
#include <mutex.h>
#include <thread.h>
#include <thr_internals.h>
#include <rwlock.h>
#include <rwlock_internals.h>

/** @brief initializes a new or destroyed reader-writer lock
 *  @param rwlock pointer to the reader-writer lock to initialize
 *
 *  requires that rwlock is not already initialized and it not null
 *
 *  @return 0 on success, -1 on failure
 */
int rwlock_init( rwlock_t* rwlock )
{
  if ( rwlock == NULL )
  {
    printf("rwlock_init: trying to init a NULL lock!\n");
    return ERROR_NULL_POINTER;
  }

  if ( (rwlock->valid) )
  {
    printf("rwlock_init: trying to init an initialized lock!\n");
    return ERROR_DOUBLE_INITIALIZATION;
  }

  if ( mutex_init( &(rwlock->data_mutex) ) < 0 )
  {
    printf("rwlock_init: trying to init an initialized lock!\n");
    return ERROR_DOUBLE_INITIALIZATION;
  }

  (rwlock->valid) = true;
  (rwlock->reader_count) = 0;
  (rwlock->waiting_rw) = (rw_queue_t*) malloc( sizeof(rw_queue_t) );
  (rwlock->mode) = RW_UNLOCKED;

  Q_INIT_HEAD( rwlock->waiting_rw );

  return SUCCESS_RETURN;
}

/** @brief destroys an initialized reader-writer lock
 *  @param rwlock pointer to the reader-writer lock to destroy
 *
 *  requires that rwlock is not null, initialized, and unlocked
 *
 *  @return void
 */
void rwlock_destroy( rwlock_t* rwlock )
{

  if ( rwlock == NULL )
  {
    printf("rwlock_destroy: trying to destroy a NULL lock!\n");
    return;
  }

  if ( !rwlock->valid )
  {
    printf("rwlock_destroy: trying to destroy an uninitialized lock!\n");
    return;
  }

  mutex_lock( &(rwlock->data_mutex) );

  if ( (rwlock->mode) != RW_UNLOCKED )
  {
    mutex_unlock( &(rwlock->data_mutex) );
    printf("rwlock_destroy: trying to destroy a busy lock!\n");
    return;
  }

  if ( (rwlock->waiting_rw)->size > 0 )
  {
    mutex_unlock( &(rwlock->data_mutex) );
    printf("rwlock_destroy: trying to destroy a busy lock!\n");
    return;
  }

  // set rwlock struct to destoyed values
  (rwlock->valid) = false;
  (rwlock->reader_count) = -1;
  free( rwlock->waiting_rw );

  mutex_unlock( &(rwlock->data_mutex) );

  mutex_destroy( &(rwlock->data_mutex) );
}

/** @brief aquires a reader-writer lock
 *  @param rwlock pointer to the reader-writer lock to aquires
 *  @param type the mode to aquire the lock in, can be reading or writing
 *
 *  requires type == RWLOCK_READ || type == RWLOCK_WRITE
 *           and rwlock is not NULL and initialized
 *  @return void
 */
void rwlock_lock( rwlock_t* rwlock, int type )
{
  if ( !((RWLOCK_READ == type) || (RWLOCK_WRITE == type)) )
  {
    printf("rwlock_lock: invalid type parameter\n");
    return;
  }

  if ( rwlock == NULL )
  {
    printf("rwlock_lock: trying to lock a NULL lock!\n");
    return;
  }

  if ( !(rwlock->valid) )
  {
    printf("rwlock_lock: trying to unlock an uninitialized lock!\n");
    return;
  }

  mutex_lock( &(rwlock->data_mutex) );

  if ( RWLOCK_READ == type )
  {
    // if the lock is aquired, this changes the state to READING immediately,
    // if the lock cannot be aquired and this is blocked, the state will be
    // modified to READING after this thread has been made runnable again
    int next_state = RW_READING;

    switch ( (rwlock->mode) )
    {
      case RW_UNLOCKED:
        (rwlock->reader_count) += 1;
        break;
      case RW_READING:

        if ( (rwlock->waiting_rw)->size == 0 )
        {
          (rwlock->reader_count) += 1;
          break;
        }
      case RW_WRITING:;

        append_rw( rwlock, type);

        mutex_unlock( &(rwlock->data_mutex) );

        int reject_val = 0;
        deschedule( &reject_val );

        mutex_lock(  &(rwlock->data_mutex) );
        (rwlock->reader_count) += 1;
        break;
      default:
        break;
    }

    (rwlock->mode) = next_state;
  }
  else if ( RWLOCK_WRITE == type)
  {
    // if the lock is aquired, this changes the state to WRITING immediately,
    // if the lock cannot be aquired and this is blocked, the state will be
    // modified to WRITING after this thread has been made runnable again
    int next_state = RW_WRITING;

    switch ( (rwlock->mode) )
    {
      case RW_UNLOCKED:
        break;
      case RW_READING:
      case RW_WRITING:;

        append_rw( rwlock, type);

        mutex_unlock( &(rwlock->data_mutex) );

        int reject_val = 0;
        deschedule( &reject_val );

        mutex_lock(  &(rwlock->data_mutex) );
        break;
      default:
        break;
    }

    (rwlock->mode) = next_state;
  }

  mutex_unlock( &(rwlock->data_mutex) );
}

/** @brief releases a reader-writer lock
 *  @param rwlock pointer to a reader-writer lock to release
 *
 *  requires that the thread currently owns rwlock
 *           and that rwlock is not null, initialized, and locked
 *
 *  @return void
 */
void rwlock_unlock( rwlock_t* rwlock )
{
  if ( rwlock == NULL )
  {
    printf("rwlock_unlock: trying to unlock a NULL lock!\n");
    return;
  }

  if ( !(rwlock->valid) )
  {
    printf("rwlock_unlock: trying to unlock an uninitialized lock!\n");
    return;
  }

  mutex_lock( &(rwlock->data_mutex) );

  // here we assume the unlocking application is responsible
  // and is one of the people holding the lock
  int next_state = RW_UNLOCKED;

  switch ( (rwlock->mode) )
  {

    case RW_UNLOCKED:
      // signal error
      // should not be unlocking an unlocked rwlock
      break;
    case RW_READING:
      if ( (rwlock->reader_count) > 1 )
      {
        (rwlock->reader_count) -= 1;
      }
      else
      {
        (rwlock->reader_count) -= 1;

        next_state = dequeue( rwlock );
      }
      break;
    case RW_WRITING:
      next_state = dequeue( rwlock );
      break;
    default:
      break;
  }

  (rwlock->mode) = next_state;

  mutex_unlock( &(rwlock->data_mutex) );
}

/** @brief converts a writer with the lock into a reader with the lock
 *  @param rwlock pointer to a reader-writer lock to convert from
 *         writing mode to reading mode
 *  requires that rwlock is currently in writing mode, the current thread
 *           owns the lock, and the lock is initialized, not NULL, and locked
 *  @return void
 */
void rwlock_downgrade( rwlock_t* rwlock )
{
  if ( rwlock == NULL )
  {
    printf("rwlock_downgrade: trying to downgrade on a NULL lock!\n");
    return;
  }

  if ( !(rwlock->valid) )
  {
    printf("rwlock_downgrade: trying to downgrade on an uninitialized lock!\n");
    return;
  }

  mutex_lock( &(rwlock->data_mutex) );

  if ( (rwlock->mode) == RW_WRITING )
  {
    (rwlock->reader_count) += 1;

    if ( (rwlock->waiting_rw)->size > 0 )
    {
      if ( RWLOCK_READ == (Q_GET_FRONT(rwlock->waiting_rw)->rw_type) )
      {
        dequeue( rwlock );
      }
    }

    (rwlock->mode) = RW_READING;
  }

  mutex_unlock( &(rwlock->data_mutex) );
}
