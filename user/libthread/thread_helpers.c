/** @file thread_helpers.c
 *  @brief helper functions for the implementation of the thread library
 *
 *  @author Tianya Chen       (andrewid: tianyac, email: tianyac@andrew.cmu.edu)
 *  @author Carlos Montemayor (andrewid: cmontema, email: carl6256@gmail.com)
 */

#include <contracts.h>
#include <simics.h>
#include <stddef.h>
#include <stdio.h>
#include <thread.h>
#include <syscall.h>
#include <error_code.h>
#include <mutex.h>
#include <cond.h>
#include <swexn_internals.h>
#include "thr_internals.h"

/** @brief corrects size input to be a multiple of the page size
 *  @param size the size to round to a multiple of the page size
 *
 *  ensures return value % PAGE_SIZE == 0
 *
 *  @return the rounded up size
 */
unsigned int round_up_stack_size(unsigned int size){
  return (PAGE_SIZE - (size % PAGE_SIZE)) + size;
}

/** @brief gets the calling thread's metadata
 *  @return a pointer to the metadata struct on success, NULL on failure
 */
thr_stack_meta_t* find_current_thread_meta(){

  // if the table size is zero meaning we haven't initialized the thr_init.
  if (g_thr_table.size == 0){
    return NULL;
  }

  uint32_t ebp = read_ebp();


  return find_thread_meta_by_ebp(ebp);
}

/** @brief returns the metadata for a given thread with input ebp
 *  @param ebp the given ebp address as unsigned int
 *
 *  @returns a pointer to the metadata struct of the thread if it exists in the
 *           task, NULL otherwise
 */
thr_stack_meta_t* find_thread_meta_by_ebp ( uint32_t ebp ){
  // iterate through the thread table
  thr_stack_meta_t *current_thread;
  Q_FOREACH(current_thread, &g_thr_table, thr_table_link) {
    if (ebp <= current_thread->stack_high && ebp >= current_thread->stack_low){
      return current_thread;
    }
  }
  return NULL;
}

/** @brief returns the metadata for a given thread in the current task
 *  @param tid the thread id of the task that the caller wants the metadata of
 *
 *  requires tid >= 0, and tid exists
 *
 *  @returns a pointer to the metadata struct of the thread if it exists in the
 *           task, NULL otherwise
 */
thr_stack_meta_t* find_thread_meta_by_tid( int tid ){

  if ( tid < 0 ){
    return NULL;
  }

  // if the table size is zero meaning we haven't initialized the thr_init. 
  if (g_thr_table.size == 0){
    return NULL;
  }

  // iterate through the thread table
  thr_stack_meta_t *current_thread;
  Q_FOREACH(current_thread, &g_thr_table, thr_table_link) {
    if (current_thread->tid == tid){
      return current_thread;
    }
  }
  return NULL;
}

/** @brief loads the stack_meta_ptr with default metadata info
 *         and initializes it
 *  @param stack_meta_ptr pointer to the stack metadata struct to fill
 *  @param first_init tells if this stack struct hasn't been initialized before
 *  @param func pointer to the function that the child thread should start at
 *  @param arg initial arguments for the child thread
 *  @return 0 on success, negative number on failure
 */
int initialize_stack_meta(thr_stack_meta_t* stack_meta_ptr, bool first_init,
                          void *(*func)(void *), void *arg){

  if (stack_meta_ptr == NULL || func == NULL){
    return ERROR_INIT_STACK_META_FAILED;
  }

  assert( (((uint32_t) stack_meta_ptr) % ESP_ALIGNMENT) == 0 );

  uint32_t thr_stack_high = (uint32_t)stack_meta_ptr + sizeof(thr_stack_meta_t);
  uint32_t thr_stack_low = thr_stack_high - g_thr_stack_size;

  // only initialize the meta_mutex and meta_cv on first init
  if (first_init){
    int ret = 0;
    ret |= mutex_init( &(stack_meta_ptr->meta_mutex) );
    ret |= cond_init( &(stack_meta_ptr->meta_cv) );
    if (ret < 0) {
      return ERROR_INIT_STACK_META_FAILED;
    }
  }

  Q_INIT_ELEM(stack_meta_ptr, thr_table_link);
  Q_INIT_ELEM(stack_meta_ptr, free_stk_table_link);
  Q_INIT_ELEM(stack_meta_ptr, cv_link);
  Q_INIT_ELEM(stack_meta_ptr, rw_link);

  mutex_lock( &(stack_meta_ptr->meta_mutex) );
  stack_meta_ptr->ret_addr = &thr_exit;
  stack_meta_ptr->func = func;
  stack_meta_ptr->arg = arg;
  stack_meta_ptr->thr_state = UNSTARTED;
  stack_meta_ptr->join_flag = NOTJOINING;
  stack_meta_ptr->rw_type = RWLOCK_INVALID;
  stack_meta_ptr->exit_status = NULL;
  stack_meta_ptr->root = IS_NOT_ROOT;
  stack_meta_ptr->tid = UNSIGNED_TID;
  stack_meta_ptr->stack_high = thr_stack_high;
  stack_meta_ptr->stack_low = thr_stack_low;
  stack_meta_ptr->zero = NULL;
  mutex_unlock( &(stack_meta_ptr->meta_mutex) );

  return SUCCESS_RETURN;
}

/** @brief initializes the stack meta and loads it onto the thread table
 *  @param size the size of the stack to allocate
 *  @param func pointer to the function that the child thread should start at
 *  @param arg initial arguments for the child thread
 *  @return pointer to the stack metadata on success, NULL on failure
 */
thr_stack_meta_t* allocate_init_thr_stack(unsigned int size,
                                          void *(*func)(void *), void *arg){
  assert((size % PAGE_SIZE) == 0);
  thr_stack_meta_t *free_spot = NULL;
  bool first_init = false;

  // If free_stk_table is not empty
  mutex_lock(&g_free_stk_table_mutex);
  if (g_free_stk_table.size > 0){
    first_init = false;

    free_spot = Q_GET_FRONT(&g_free_stk_table);
    Q_REMOVE(&g_free_stk_table, g_free_stk_table.front, free_stk_table_link);
    mutex_unlock(&g_free_stk_table_mutex);
  } else {
    first_init = true;
    mutex_unlock(&g_free_stk_table_mutex);

    // Otherwise, lower the g_stacks_brk
    mutex_lock( &g_stack_mutex );
    g_stacks_brk = g_stacks_brk & PAGE_ALIGN_MASK;
    g_stacks_brk -= size;
    /* make sure the g_stacks_brk doesn't overwirtten by other threads for later
       computation */
    uint32_t local_stack_low = g_stacks_brk;
    mutex_unlock( &g_stack_mutex );
    if (new_pages((void*)local_stack_low, size) < 0){
      return NULL;
    }
    free_spot = (thr_stack_meta_t*) ((local_stack_low + size) - sizeof(thr_stack_meta_t));
  }

// initialize the free spot stack
	if ( initialize_stack_meta( free_spot, first_init, func, arg) == ERROR_INIT_STACK_META_FAILED ){

    // move the newly allocated but can't initialzied stack, to free_stk_table
    mutex_lock( &g_free_stk_table_mutex );
    Q_INSERT_TAIL( &g_free_stk_table, free_spot, free_stk_table_link );
    mutex_unlock( &g_free_stk_table_mutex );
		printf("allocate_init_thr_stack: can't initialize stack meta.\n");
    return NULL;
	}

  // append to global thread table
  mutex_lock( &g_thr_table_mutex );
  Q_INSERT_TAIL( &g_thr_table, free_spot, thr_table_link);
  mutex_unlock( &g_thr_table_mutex );

  return free_spot;
}

/** @brief removes a metadata entry from the global table and adds to free_stk_table
 *  @param stack_meta_ptr the pointer of thr_stack to remove from the global thread table
 *  @returns void
 */
void free_thr_stack(thr_stack_meta_t* stack_meta_ptr){

    if ( stack_meta_ptr == NULL )
    {
      printf("free_thr_stack: stack meta pointer is NULL!");
      return;
    }

    // ignore the root meta
    if ( stack_meta_ptr->root == IS_ROOT ){
      return;
    }

    // remove current thr_stack from global thread table
    mutex_lock( &g_thr_table_mutex );
    Q_REMOVE( &g_thr_table, stack_meta_ptr, thr_table_link );
    mutex_unlock( &g_thr_table_mutex );

    // then append stack to free_stk_table
    mutex_lock( &g_free_stk_table_mutex );
    Q_INSERT_TAIL( &g_free_stk_table, stack_meta_ptr, free_stk_table_link );
    mutex_unlock( &g_free_stk_table_mutex );

}

/** @brief wrapper for the initial thread function
 *  @param func pointer to the function that the child thread should start at
 *  @param arg initial arguments for the child thread
 *  @return never returns
 */
void run_thr_func(void *(*func)(void *), void *arg){
  // install exception handler
  install_swexn();

  thr_stack_meta_t *stack_meta_ptr = find_current_thread_meta();

  affirm_msg( stack_meta_ptr != NULL, "thr_create: new thread could not be created!");

  // wait until the parent thread make the child thread runnable.
  mutex_lock( &(stack_meta_ptr->meta_mutex) );

  while (stack_meta_ptr->thr_state != RUNNABLE) {
      cond_wait( &(stack_meta_ptr->meta_cv), &(stack_meta_ptr->meta_mutex) );
  }
  mutex_unlock( &(stack_meta_ptr->meta_mutex) );

  // the function should call thr_exit and not return
  void *ret = func(arg);
  // if the func doesn't call thr_exit, call it here!
  thr_exit(ret);
}

/** @brief prints the topology of a thread table
 *  @param header the header struct for a thread table
 *  @returns void
 */
void print_thr_table( thr_table_t* header )
{
  lprintf("Table size: %d\n", header->size);

  thr_stack_meta_t* current;

  int entry_num = 0;
  Q_FOREACH( current, header, thr_table_link)
  {
    lprintf("Entry %d prev pointer: %x\n", entry_num, (unsigned int) current->thr_table_link.prev);
    lprintf("Entry %d next pointer: %x\n", entry_num, (unsigned int) current->thr_table_link.next);
    entry_num++;
  }
}

/** @brief prints some thread's metadata
 *  @param tid the tid corresponding to the thread whose metadata
 *         to print
 *  @returns void
 */
void print_thr_stack_meta_by_tid( int tid )
{
  thr_stack_meta_t* meta = find_thread_meta_by_tid( tid );

  if ( meta != NULL )
  {
    lprintf("prev pointer: %x\n", (unsigned int) meta->thr_table_link.prev);
    lprintf("next pointer: %x\n", (unsigned int) meta->thr_table_link.next);
  }
  else
  {
    lprintf("Metadata could not be found!\n");
  }
}

/** @brief prints some thread's metadata
 *  @param meta the metadata to print
 *  @returns void
 */
void print_thr_stack_meta( thr_stack_meta_t* meta )
{
  if ( meta != NULL )
  {
    lprintf("prev pointer: %x\n", (unsigned int) meta->thr_table_link.prev);
    lprintf("next pointer: %x\n", (unsigned int) meta->thr_table_link.next);
  }
  else
  {
    lprintf("Metadata could not be found!\n");
  }
}

