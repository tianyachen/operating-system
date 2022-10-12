/** @file mutex.c
 *  @brief implementation of mutex library functions
 *
 * @author Carlos Montemayor (andrewid: cmontema, email: carl6256@gmail.com)
 * @author Tianya Chen       (andrewid: tianyac, email: tianyac@andrew.cmu.edu)
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <simics.h>
#include <syscall.h>
#include <mutex.h>
#include <thread.h>
#include <mutex_private.h>
#include "error_code.h"

/** @brief initializes mutex lock pointed to by mp
 *  @param mp the pointer to an uninitialized mutex to initialize
 *  @returns 0 on success, negative on failure
 */
int mutex_init( mutex_t *mp )
{
  if ( mp == NULL )
  {
    printf("mutex_init: mutex pointer is NULL.\n");
    return ERROR_NULL_POINTER;
  }

  if ( (mp->valid == LOCK_INITIALIZED) && ( mp->ticket_num != 0)){
    printf("mutex_init: try to init on mutex that is used.\n");
    return ERROR_INIT_ON_USE;
  }

  // initalize mutex lock
  (mp->valid) = LOCK_INITIALIZED;
  (mp->ticket_num) = 0;
  (mp->turn) = 0;

  return SUCCESS_RETURN;
}

/** @brief destroys mutex lock pointed to by mp
 *  @param mp pointer to initialized and available mutex to destroy
 *  @returns void
 */
void mutex_destroy( mutex_t *mp )
{
  if ( mp == NULL )
  {

    printf("mutex_destroy: mutex pointer is NULL.\n");
    return;
  }

  if ( (mp->valid == LOCK_INITIALIZED) && ( mp->ticket_num != mp->turn) )
  {
    printf("mutex_destroy: try to destroy on mutex that is used.\n");
    return;
  }

  (mp->valid) = LOCK_UNINITIALIZED;
  (mp->ticket_num) = 0;
  (mp->turn) = 0;

}

/** @brief aquires the mutex pointed to by mp
 *  @param mp pointer to the initialized mutex lock to aquire
 *
 *  requires mp != NULL && mp is initialized
 *
 *  @return void
 */
void mutex_lock( mutex_t *mp )
{
  if ( mp == NULL )
  {
    printf("mutex_lock: the mutex pointer is NULL.\n");
    return;
  }

  if ( (mp->valid == LOCK_UNINITIALIZED) )
  {
    printf("mutex_lock: the mutex is uninitialized.\n");
    return;
  }

  // get the current ticket num
  unsigned int myticket = atomic_increment( &(mp->ticket_num) );

  // loop until my ticket == turn
  while( myticket != (mp->turn) ){
    // sleep while mutex isn't running. FOR EFFICIENCY!
    thr_yield(YIELD_ANYONE);
  }
}

/** @brief releases the mutex pointed to by mp
 *  @param mp pointer to the mutex lock to release
 *
 *  requires mp != NULL && mp is initialized && mp was locked by calling thread
 *
 *  @return void
 */
void mutex_unlock( mutex_t *mp )
{
  if ( mp == NULL )
  {
    printf("mutex_unlock: the mutex pointer is null.\n");
    return;
  }

  if ( (mp->valid) == LOCK_UNINITIALIZED )
  {
    printf("mutex_unlock: can't unlock an uninitialized mutex.\n");
    return;
  }

  atomic_increment( &(mp->turn) );
}
