/** @file semaphore.c
 *  @brief implementation of semaphore library functions
 *
 *  @author Tianya Chen (andrewid: tianyac, email: tianyakc@gmail.com)
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <simics.h>
#include <syscall.h>
#include <mutex.h>
#include <cond.h>
#include <sem.h>

#include "mutex_private.h"
#include "error_code.h"

/** @brief initializes the semaphore
 *  @param sem the pointer to a semphore struct
 *  @param count the count of how many thread can acquires the sem at same time
 *  @return 0 on success, negative number on failure
 */
int sem_init( sem_t *sem, int count ){
    if (sem == NULL) {
        return ERROR_NULL_POINTER;
    }

    if (sem->valid){
        return ERROR_DOUBLE_INITIALIZATION;
    }

    /* initialize the internal lock */
    int ret;
    if ( (ret = mutex_init( &(sem->lock) ) ) < 0){
        return ret;
    }
    if ( (ret = cond_init( &(sem->cv) ) ) < 0){
        return ret;
    }
    /* lock the critical session */
    mutex_lock( &(sem->lock) );
    sem->valid = true;
    sem->count = count;
    mutex_unlock( &(sem->lock) );
    return SUCCESS_RETURN;
}

/** @brief atomically decreases the sem count if count >0, otherwsie block until
 *         count is large than zero.
 *  @param sem pointer to initialized and available semaphore to destroy
 *  @returns void
 */
void sem_wait( sem_t *sem ){
    if (sem == NULL) {
        panic("sem_wait: trying to wait a NULL semaphore.");
    }

    if (sem->valid == false ){
        panic("sem_wait: trying to wait an uninitialized semaphore.");
    }

    mutex_lock( &(sem->lock) );

    while(sem->count <= 0) {
        /* if count is zero, suspend the thread until sem->count becomes nonzero. */
        cond_wait( &(sem->cv), &(sem->lock) );
    }
    /* count is more than 0, decrements and return immedately. */
    (sem->count)--;

    mutex_unlock( &(sem->lock) );
    return;
}

/** @brief increase the sem count and signal the first tid on cv_queue
 *  @param sem pointer to the initialized semaphore
 *
 *  requires sem != NULL && sem is initialized
 *
 *  @return void
 */
void sem_signal( sem_t *sem ){
    if (sem == NULL) {
        panic("sem_signal: trying to signal a NULL semaphore.");
    }

    if (sem->valid == false ){
        panic("sem_signal: trying to signal an uninitialized semaphore.");
    }
    
    mutex_lock( &(sem->lock) );
    // increments the count.
    (sem->count)++;
    cond_signal( &(sem->cv) );
    mutex_unlock( &(sem->lock) );
    return;
}

/** @brief destroys semaphore lock pointed to by sem
 *  @param sem pointer to the initialized semaphore
 *
 *  requires sem != NULL && sem is initialized
 *
 *  @return void
 */
void sem_destroy( sem_t *sem ){
    if (sem == NULL) {
        panic("sem_destroy: trying to destroy a NULL semaphore.");
    }

    if (sem->valid){
        panic("sem_destroy: trying to destroy an uninitialized semaphore.");
    }

    mutex_lock( &(sem->lock) );
    sem->valid = false;
    sem->count = 0;
    mutex_unlock( &(sem->lock) );

    /* destroy mutex lock and condition variable */
    mutex_destroy( &(sem->lock) );
    cond_destroy( &(sem->cv) );
    return;
}



