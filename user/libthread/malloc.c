/** @file malloc.c
 *  @brief C standard memory allocation functions but with a mutex wrapper
 *         to make them thread safe
 *
 * Assuming a correct mutex implementation, these are definitely thread safe.
 * Just potentially slow because we can't do anything but wrap the functions in
 * a mutex.
 *
 * @author Carlos Montemayor (andrewid: cmontema, email: carl6256@gmail.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <types.h>
#include <stddef.h>
#include <stdbool.h>
#include <error_code.h>
#include <mutex.h>
#include <thr_internals.h>

static mutex_t heap_mutex; ///< mutex protecting non-safe allocation functions
static bool mutex_initialized = false; ///< indicates multi-threading has begun

/** @brief initializes the relevant synchronization primitives for thread safe
 *         malloc/calloc/realloc/free
 *  @returns 0 on success, negative integer on failure
 */
int malloc_init()
{
  if ( !mutex_initialized )
  {
    int ret = mutex_init( &heap_mutex );

    if ( ret == SUCCESS_RETURN )
    {
      mutex_initialized = true;
    }
    else
    {
      printf("malloc_init: mutex_initialization failed!\n");
    }

    return ret;
  }
  else
  {
    return ERROR_DOUBLE_INITIALIZATION;
  }
}

/** @brief thread safe wrapper for malloc
 *  @param __size the number of bytes to request to be allocated
 *  @return a generic pointer to the newly allocated space on success,
 *          NULL on failure
 */
void *malloc(size_t __size)
{
  if ( mutex_initialized )
  {
    mutex_lock( &heap_mutex );
    void* mem_ptr = _malloc( __size );
    mutex_unlock( &heap_mutex );

    return mem_ptr;
  }
  else
  {
    return _malloc( __size );
  }
}

/** @brief thread safe wrapper for calloc
 *  @param __nelt number of "base units" to allocate
 *  @param __eltsize size of the base "unit" to allocate in
 *  @returns a generic pointer to newly allocated space on success,
 *           NULL on failure
 */
void *calloc(size_t __nelt, size_t __eltsize)
{
  if ( mutex_initialized )
  {
    mutex_lock( &heap_mutex );
    void* mem_ptr = _calloc( __nelt, __eltsize );
    mutex_unlock( &heap_mutex );

    return mem_ptr;
  }
  else
  {
    return _calloc( __nelt, __eltsize );
  }
}

/** @brief thread safe wrapper for realloc
 *  @param __buf pointer to old allocated space to let go
 *  @param __new_size request for new size of the given buffer
 *  @returns a generic pointer to newly reallocated space on success,
 *           NULL on failure
 */
void *realloc(void *__buf, size_t __new_size)
{
  if ( mutex_initialized )
  {
    mutex_lock( &heap_mutex );
    void* mem_ptr = _realloc( __buf, __new_size );
    mutex_unlock( &heap_mutex );

    return mem_ptr;
  }
  else
  {
    return _realloc( __buf, __new_size );
  }
}

/** @brief thread safe wrapper for free
 *  @param __buf pointer to allocated space to let go of
 *  @return void
 */
void free(void *__buf)
{
  if ( mutex_initialized )
  {
    mutex_lock( &heap_mutex );
    _free( __buf );
    mutex_unlock( &heap_mutex );
  }
  else
  {
    _free( __buf );
  }
}
