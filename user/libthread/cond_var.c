/** @file cond_var.c
 *  @brief contains implmentation of condition variable for thread library
 *
 *  @author Carlos Montemayor (andrewid: cmontema, email: carl6256@gmail.com)
 */

#include <contracts.h>
#include <stddef.h>
#include <simics.h>
#include <malloc.h>
#include <syscall.h>
#include <thread.h>
#include <cond.h>
#include "thr_internals.h"
#include "variable_queue.h"
#include "error_code.h"

/** @brief adds the currently calling thread to the condition variable
 *         thread list
 *  requires cv != NULL
 *        && cond_var_is_valid( cv )
 *        && thread_list_size_check( cv )
 *  @param cv a condition variable
 */
void append_thread( cond_t* cv )
{
  thr_stack_meta_t* new_thread = find_current_thread_meta();

  if ( new_thread == NULL )
  {
    panic("cond_wait: can't find its stack meta data.\n");
  }

  Q_INIT_ELEM(new_thread, cv_link);

  Q_INSERT_TAIL((cv->cond_queue), new_thread, cv_link);
}

/** @brief initializes the given condition variable
 *  @param cv pointer to the condition variable to initialize
 *
 *  requires cv != NULL
 *           and cv is uninitialized
 *
 *  @return 0 on success, negative number on failure
 */
int cond_init( cond_t* cv )
{
  // if the pointer is non-null assume it has already been initialized and destroyed
  if ( cv == NULL )
  {
    printf("cond_init: mutex pointer is NULL.\n");
    return ERROR_NULL_POINTER;
  }

  if ( (cv->valid) == COND_INITIALIZED )
  {
    printf("cond_init: tried to initialize initialized condition variable\n");
    return ERROR_INIT_ON_USE;
  }

  // intialize the cond_mutex
  if ( mutex_init( &(cv->cond_mutex)) < 0 )
  {
    return ERROR_INIT_ON_USE;
  }

  // allocate memory for cond_queue
  cv->cond_queue = (cond_queue_t*)malloc(sizeof(cond_queue_t));
  if ( cv->cond_queue == NULL){
    return ERROR_MALLOC_FAILED;
  }

  // initalize the cond_queue
  Q_INIT_HEAD( cv->cond_queue );

  // set valid flag to true
  (cv->valid) = COND_INITIALIZED;

  return SUCCESS_RETURN;
}

/** @brief destroys the given condition variable
 *  @param cv pointer to the condition variable to destroy
 *
 *  requires cv != NULL
 *          && cv is initialized but in use
 *           or being waited on
 *
 * @return void
 */
void cond_destroy( cond_t* cv)
{

  if ( NULL == cv )
  {
    printf( "cond_destroy: trying to destroy a NULL condition variable" );
    return;
  }

  mutex_lock( &(cv->cond_mutex) );
  if ( (cv->cond_queue) != NULL )
  {
    if ( (cv->cond_queue->size) != 0 )
    {
      mutex_unlock( &(cv->cond_mutex) );
      printf( "cond_destroy: trying to destroy while thread queue is nonempty.\n" );
      return;
    }
  }

  (cv->valid) = COND_UNINITIALIZED;
  mutex_unlock( &(cv->cond_mutex) );
  mutex_destroy( &(cv->cond_mutex) );
  free( cv->cond_queue );

  cv->cond_queue = NULL;
}

/** @brief waits on the given condition variable and gives up the mutex
 *  @param cv pointer to the condition variable to wait on
 *  @param mp pointer to the mutex to temporarily give up
 *
 *  this function will block until another thread calls
 *  cond_signal on cv, and will temporarily release the mutex
 *  mp while the function blocks. When this function returns, mp
 *  will automatically be re-aquired.
 *
 *  @return void
 *
 */
void cond_wait( cond_t* cv, mutex_t* mp )
{

  if ( NULL == cv )
  {
    printf( "cond_wait: trying to wait on a NULL condition variable\n" );
    return;
  }

  mutex_lock( &(cv->cond_mutex) );
  if ( (cv->valid) == COND_UNINITIALIZED )
  {
    mutex_unlock( &(cv->cond_mutex) );
    printf( "cond_wait: trying to wait on uninitialized condition variable\n" );
    return;
  }

  // add to thread queue
  append_thread( cv );
  mutex_unlock( &(cv->cond_mutex) );

  // give up on our mutex to let
  // some other thread do work
  mutex_unlock( mp );

  // deschedule oneself
  // cv->cond_queue->tail->thr_state = NOTRUNNABLE;
  int reject_val = 0;
  deschedule( &reject_val );

  // re-aquire lock
  mutex_lock( mp );
}

/** @brief wakes one thread waiting on cv
 *  @param cv pointer to condition variable to wake a thread up from
 *
 *  this function will wake a single thread that is waiting on cv
 *  per call, guaranteed to be runnable when this function returns
 *  unless thread crashes
 *
 *  @return void
 */
void cond_signal( cond_t* cv )
{
  mutex_lock( &(cv->cond_mutex) );

  if ( (cv->cond_queue->size) > 0 )
  {
    // get front thread of the queue
    thr_stack_meta_t* next_thread = Q_GET_FRONT( cv->cond_queue );

    // remove the front thread
    Q_REMOVE( cv->cond_queue , next_thread, cv_link);

    assert( (cv->cond_queue->size) >= 0 );

    // schedule next thread
    int next_tid = (next_thread->tid);
    mutex_unlock( &(cv->cond_mutex) );

    while ( make_runnable( next_tid ) < 0 )
    {
      if ( find_thread_meta_by_tid( next_tid ) == NULL )
      {
        break;
      }

      thr_yield( next_tid );
    }
  }
  else
  {
    mutex_unlock( &(cv->cond_mutex) );
  }
}

/** @brief signals ALL the threads waiting on cv to wake up
 *  @param cv pointer to the condition variable to broadcast to
 *
 *  requires cv != NULL
 *         && cond_var_is_valid( cv )
 *         && thread_list_size_check( cv )
 *
 * @return void
 */
void cond_broadcast( cond_t* cv )
{
  thr_stack_meta_t* next_thread = NULL;
  // could implement this by just calling cond_signal a bunch of times
  mutex_lock( &(cv->cond_mutex) );

  while ( (cv->cond_queue->size) > 0 )
  {
    // get front thread of the queue
    next_thread = Q_GET_FRONT( cv->cond_queue );

    // remove the front thread
    Q_REMOVE( cv->cond_queue, next_thread, cv_link );

    // schedule next thread
    int next_tid = (next_thread->tid);
    while ( make_runnable( next_thread->tid ) )
    {
      thr_yield( next_tid );
    }
  }
  mutex_unlock( &(cv->cond_mutex) );
}
