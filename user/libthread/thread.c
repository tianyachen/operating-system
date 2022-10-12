/** @file thread.c
 *
 *  @brief implementations of core thread library functions
 *
 *  @author Tianya Chen       (andrewid: tianyac, email: tianyac@andrew.cmu.edu)
 *  @author Carlos Montemayor (andrewid: cmontema, email: carl6256@gmail.com)
 */

#include <stddef.h>
#include <stdio.h>
#include <simics.h>
#include <contracts.h>
#include <syscall.h>
#include <malloc.h>
#include <thread.h>
#include <error_code.h>
#include "thr_internals.h"
#include <swexn_internals.h>
#include <mutex.h>
#include <cond.h>

/* global variables  */
/** global variable for the root
 * thread's metadata, allocated
 * as a global variable to be outside
 * the heap and be "permanent"
 */
thr_stack_meta_t g_root_thr_meta;

/** @brief initializes the thread's metadata and tracking structures
 *  @param size the maximum size of any thread's stack
 *  @return 0 on success, negative number on failure
 */
int thr_init(unsigned int size){
  // regsiter a general software exception handler
  install_swexn();

  // the size must a multiple of PAGE_SIZE.
  g_thr_stack_size = round_up_stack_size(size + sizeof(thr_stack_meta_t));

  //initialzie heap mutex
  if ( malloc_init() < 0 )
  {
    panic("thr_init: malloc initialization failed!");
  }

  // initialize thread mutexes and cond variables
  int ret = 0;
  ret |= mutex_init( &g_stack_mutex );
  ret |= mutex_init( &g_thr_table_mutex );
  ret |= mutex_init( &g_free_stk_table_mutex );
  ret |= mutex_init( &(g_root_thr_meta.meta_mutex) );
  ret |= cond_init( &(g_root_thr_meta.meta_cv) );
  if (ret < 0) {
    mutex_destroy( &g_stack_mutex );
    mutex_destroy( &g_thr_table_mutex );
    mutex_destroy( &g_free_stk_table_mutex );
    mutex_destroy( &(g_root_thr_meta.meta_mutex) );
    cond_destroy( &(g_root_thr_meta.meta_cv) );
    return ERROR_THR_INIT_FAILED;
  }

  //initialize thread stack table
  Q_INIT_HEAD(&g_thr_table);
  Q_INIT_HEAD(&g_free_stk_table);

  // intialize g_root_thr_meta.
  (g_root_thr_meta.ret_addr) = &thr_exit;
  (g_root_thr_meta.func) = NULL;
  (g_root_thr_meta.arg) = NULL;
  (g_root_thr_meta.thr_state) = RUNNABLE;
  Q_INIT_ELEM(&g_root_thr_meta, thr_table_link);
  Q_INIT_ELEM(&g_root_thr_meta, free_stk_table_link);
  Q_INIT_ELEM(&g_root_thr_meta, cv_link);
  Q_INIT_ELEM(&g_root_thr_meta, rw_link);
  (g_root_thr_meta.join_flag) = NOTJOINING;
  (g_root_thr_meta.rw_type) = RWLOCK_INVALID;
  (g_root_thr_meta.exit_status) = NULL;
  (g_root_thr_meta.root) = IS_ROOT;
  (g_root_thr_meta.tid) = gettid(); //thr_getid() no longer calls gettid();
  (g_root_thr_meta.stack_high) = get_root_stack_high();
  (g_root_thr_meta.stack_low) = get_root_stack_low();
  (g_root_thr_meta.zero) = 0;

  // add root entry as the first thread and CAN'T be removed from g_thr_table
  mutex_lock( &g_thr_table_mutex );
  Q_INSERT_FRONT( &g_thr_table, &g_root_thr_meta, thr_table_link);
  mutex_unlock( &g_thr_table_mutex );

  // get the current g_stacks_brk, same as root stack low
  mutex_lock( &g_stack_mutex );
  g_stacks_brk = (g_root_thr_meta.stack_low);
  mutex_unlock( &g_stack_mutex );

  return SUCCESS_RETURN;
}

/** @brief creates a new thread that starts at the provided function
 *         with the provided arguments
 *  @param func pointer to the function to start the child thread in
 *  @param arg set of arguments to pass into child threads initial function
 *  @return tid of child thread on success, negative number on failure
 */
int thr_create(void *(*func)(void *), void *arg){
  // allocate the thr_stack, thread safe
  thr_stack_meta_t* stack_meta_ptr = allocate_init_thr_stack(g_thr_stack_size, func, arg);

  if (stack_meta_ptr == NULL){
    printf("thr_create: can't allocate memory for new thread stack.\n");
    return ERROR_THR_CREATE_FAILED;
  }

  void *ebp = &(stack_meta_ptr->zero); // the base item on the stack
  void *esp = &(stack_meta_ptr->ret_addr);
  int tid = create_new_thread(ebp, esp); // return is equal to the return of thread_fork_int


  // child thread will not run the code below
  if (tid < 0){
    free_thr_stack(stack_meta_ptr);

    // signal error
    printf("thr_create: failed to create new thread.\n");
    return ERROR_THR_CREATE_FAILED;
  }

  mutex_lock( & stack_meta_ptr->meta_mutex );
  stack_meta_ptr->tid = tid;
  stack_meta_ptr->thr_state = RUNNABLE;
  cond_signal( &(stack_meta_ptr->meta_cv) );
  mutex_unlock( &stack_meta_ptr->meta_mutex );


  return tid;

}

/** @brief waits for a thread to exit and collects return status
 *  @param tid thread id of the thread to join
 *  @param statusp callback parameter to pass exit status to
 *
 *  requires stack_metadata != NULL
 *
 *  @return 0 on success, negative number on failure
 */
int thr_join(int tid, void **statusp){
  thr_stack_meta_t *exit_thread = NULL;

  if ( (exit_thread = find_thread_meta_by_tid(tid)) == NULL )
  {
    printf("thr_join: cannot find the metadata for exit_thread %d\n", tid);
    return ERROR_INVALID_TID;
  }

  if ((exit_thread->join_flag) == JOINING || (exit_thread->tid) != tid ){
    return ERROR_MULTIPLE_JOINS;
  }

  mutex_lock( &(exit_thread->meta_mutex) );

  if ((exit_thread->join_flag) == JOINING || (exit_thread->tid) != tid ){
    mutex_unlock( &(exit_thread->meta_mutex) );
    return ERROR_MULTIPLE_JOINS;
  }
  // only one thread does below
  (exit_thread->join_flag) = JOINING;

  // wait on thread's condition variable
  while (exit_thread->thr_state != TERMINATED) {
    cond_wait( &(exit_thread->meta_cv), &(exit_thread->meta_mutex) );
  }

  if ( statusp != NULL )
  {
    (*statusp) = (exit_thread->exit_status);
  }

  mutex_unlock( &(exit_thread->meta_mutex) );

  // free stack space
  free_thr_stack(exit_thread);

  return SUCCESS_RETURN;
}

/** @brief causes a thread to finish and passes on exit status
 *  @param status the exit status of the calling thread
 *  @return never returns
 */
void thr_exit(void *status)
{

  thr_stack_meta_t *exit_thread = find_current_thread_meta( );

  affirm_msg( exit_thread != NULL, "thr_exit: Thread cannot find its own stack metadata!\n");

  mutex_lock( &(exit_thread->meta_mutex) );
  (exit_thread->exit_status) = status;
  (exit_thread->thr_state) = TERMINATED;
  

  // wake the joining thread up
  if ( exit_thread->join_flag == JOINING )
  {
    cond_signal( &(exit_thread->meta_cv) );
  }
  mutex_unlock( &(exit_thread->meta_mutex) );

  lprintf("exit thread: %d\n", exit_thread->tid);
  vanish();
}

/** @brief figures out tid of calling thread
 *  @return tid of calling thread
 */
int thr_getid(void)
{
  thr_stack_meta_t *current_thread = find_current_thread_meta();
  if ( current_thread == NULL )
  {
    return gettid();
  }

  if ( current_thread->tid == UNSIGNED_TID )
  {
    return gettid();
  }

  return current_thread->tid;
}

/** @brief causes the current thread to stop running
 *         and another thread to take its place
 *  @param tid the requested thread to run next, runs thread
 *         at scheduler's discretion if this is -1
 *  @return 0 on success, negative number on failure
 */
int thr_yield(int tid)
{
  if ( tid == YIELD_ANYONE )
  {
    return yield(tid);
  }
  else
  {
    thr_stack_meta_t *stack_meta_ptr = find_thread_meta_by_tid(tid);
    if (stack_meta_ptr == NULL )
    {
      return ERROR_INVALID_TID;
    }
  }
  return yield(tid);
}
