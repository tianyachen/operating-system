/** @file thr_internals.h
 *
 *  @brief This file may be used to define things
 *         internal to the thread library.
 *
 *  @author Tianya Chen       (andrewid: tianyac, email: tianyac@andrew.cmu.edu)
 *  @author Carlos Montemayor (andrewid: cmontema, email: carl6256@gmail.com)
 */

#ifndef THR_INTERNALS_H
#define THR_INTERNALS_H

#include <stdint.h>
#include <types.h>
#include <mutex.h>
#include <variable_queue.h>
#include <cond_type.h>

/* thread states  */
#define UNSTARTED   0 ///< state for a brand new process
#define RUNNABLE    1 ///< state for a process that has been set up
#define NOTRUNNABLE 2 ///< state for a descheduled process
#define WAITING     3 ///< state for a blocked process
#define TERMINATED  4 ///< state for a vanished process

/* constants */
#ifndef ESP_ALIGNMENT
#define ESP_ALIGNMENT 4 ///< macro for alignment of ESP for different threads
#endif

#ifndef ESP_ALIGN_MASK
#define ESP_ALIGN_MASK 0xFFFFFF00 ///< macro for aligning ESP forcefully
#endif

#ifndef PAGE_ALIGN_MASK
/** mask that allows us to forcefully align an address */
#define PAGE_ALIGN_MASK ((unsigned int) ~((unsigned int) (PAGE_SIZE-1)))
#endif

#define YIELD_ANYONE -1 ///< macro to tell yield to choose next thread

#define LOCK_AVAILABLE 1  ///< macro indicating that mutex is available
#define LOCK_UNAVAILABLE 0  ///< macro indicating that mutex is taken

#define LOCK_INITIALIZED 1  ///< macro indicating that mutex is initialized
#define LOCK_UNINITIALIZED 0  ///< macro indicating that mutex is uninitialized

#define COND_INITIALIZED 1  ///< macro for initializing condition variable
#define COND_UNINITIALIZED 0  ///< macro for destroying condition variable

#define RW_UNLOCKED 0 ///< defines rwlock unlocked state
#define RW_READING  1 ///< defines rwlock reading locked state
#define RW_WRITING  2 ///< defines rwlock writing locked state

#define RWLOCK_INVALID -1 ///< rwlock type for stack metadata when not in rwlock

/**** thread metadata macros  *****/
/* flag indicates if thread is root  */
#define IS_ROOT 1   ///< thread is root
#define IS_NOT_ROOT 0 ///< thread isn't the root

/*  flag check if the thread is joined by another thread  */
#define NOTJOINING 0  ///< thread isn't being joined
#define JOINING 1     ///< thread is being joined

/** set a default tid when a tid field has not defined/assigned */
#define UNSIGNED_TID -999

/** a struct that defines the set of information kept
 *  about each thread to manage it, its metadata.
 *  This is kept at the top of the respective thread's
 *  stack and is used in multiple different lists:
 *    - active thread table
 *    - free thread stack table
 *    - condition variable waiting queue
 *    - rwlock waiting queue
 *  this is thanks to the variable queue implementation,
 *  and this structure contains multiple pieces
 *  of metadata for being managed by the synchronization
 *  primitives as well
 */
typedef struct thr_stack_meta {
    void *ret_addr;     ///< thread func return point
    void *func;         ///< function that thread began at
    void *arg;          ///< initial arguments for thread
    Q_NEW_LINK(thr_stack_meta) thr_table_link;    ///< vq link for thread table
    Q_NEW_LINK(thr_stack_meta) free_stk_table_link; ///< link for free stack
    Q_NEW_LINK(thr_stack_meta) cv_link;   ///< vq link for condition variable
    Q_NEW_LINK(thr_stack_meta) rw_link;   ///< vq link for reader/writer lock
    short thr_state;      ///< Below should be locked by meta mutex
    short root;   ///< indicates whether the respective thread is the root
    int tid;      ///< the thread ID of the respective thread
    int rw_type;  ///< the access mode of the current thread for rwlock
    int join_flag;  ///< flag for when a thread wants to join respective thread
    void *exit_status;  ///< stores the exit status of the respective thread
    mutex_t meta_mutex; ///< mutex to protect access to internal metadata
    cond_t meta_cv; ///< controls child thread creation timing
    uint32_t stack_high;  ///< the top of the respective thread's stack
    uint32_t stack_low;   ///< the bottom of the respective thread's stack
    void* zero;  ///< base ebp points to, zero field is always null.
} thr_stack_meta_t;

/// declares the cv queue type struct type
Q_NEW_HEAD(cond_queue_t, thr_stack_meta);

/// declares the thread table list type
Q_NEW_HEAD(thr_table_t, thr_stack_meta);

/// decalares the free thread table list type
Q_NEW_HEAD(free_stk_table_t, thr_stack_meta);

/// declares the rwlock queue type
Q_NEW_HEAD(rw_queue_t, thr_stack_meta);

/* global variables  */
thr_stack_meta_t g_root_thr_meta; ///< root thread metadata
unsigned int g_thr_stack_size;  ///< the size for new thread stacks
uint32_t g_root_stk_hi; ///< highest address of most recent thread's stack
uint32_t g_root_stk_lo; ///< lowest address of most recent thread's stack
uint32_t g_stacks_brk; ///< the low bound addr of the entire stack region (multiple stacks).

/// global thread table to track active threads
thr_table_t g_thr_table;
/// global free stack table to track unallocated stack space
free_stk_table_t g_free_stk_table;

/* mutexes */
mutex_t g_stack_mutex;  ///< mutex protecting stack metadata metadata
mutex_t g_thr_table_mutex;  ///< protects active thread table
mutex_t g_free_stk_table_mutex; ///< protects free thread stacks table

/** @brief returns current value of %ebp
 *  @return current value of %ebp
 */
uint32_t read_ebp(void);

/** @brief splits current thread into current thread and child thread
 *  @param ebp the base pointer for the child thread
 *  @param esp the stack pointer for the child thread
 *  @return 0 if returning to child thread, tid of child thread
 *          if returning to parent thread on success,
 *          negative number on failure (to parent)
 */
int create_new_thread(void *ebp, void *esp);
unsigned int round_up_stack_size(unsigned int size);
int malloc_init();
int initialize_stack_meta(thr_stack_meta_t* stack_meta_ptr, bool first_init,
                          void *(*func)(void *), void *arg);
thr_stack_meta_t* find_current_thread_meta();
thr_stack_meta_t* find_thread_meta_by_ebp( uint32_t ebp );
thr_stack_meta_t* find_thread_meta_by_tid( int tid );
thr_stack_meta_t* allocate_init_thr_stack(unsigned int size, void *(*func)(void *), void *arg);
void free_thr_stack(thr_stack_meta_t* stack_meta_ptr);
void run_thr_func(void *(*func)(void *), void *arg);
void print_thr_table( thr_table_t* header );
void print_thr_stack_meta_by_tid( int tid );
void print_thr_stack_meta( thr_stack_meta_t* meta );



#endif /* THR_INTERNALS_H */
