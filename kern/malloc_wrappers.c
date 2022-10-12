#include <stddef.h>
#include <malloc.h>
#include <mutex.h>

static mutex_t heap_mutex;

int malloc_init(){
  return mutex_init( &heap_mutex );
}

/* safe versions of malloc functions */
void *malloc(size_t size)
{
  mutex_lock( &heap_mutex );
  void *mem_ptr = _malloc(size);
  mutex_unlock( &heap_mutex );
  return mem_ptr;
}

void *memalign(size_t alignment, size_t size)
{
  mutex_lock( &heap_mutex );
  void *mem_ptr = _memalign(alignment, size);
  mutex_unlock( &heap_mutex );
  return mem_ptr;
}

void *calloc(size_t nelt, size_t eltsize)
{
  mutex_lock( &heap_mutex );
  void *mem_ptr = _calloc(nelt, eltsize);
  mutex_unlock( &heap_mutex );
  return mem_ptr;
}

void *realloc(void *buf, size_t new_size)
{
  mutex_lock( &heap_mutex );
  void *mem_ptr = _realloc(buf, new_size);
  mutex_unlock( &heap_mutex );
  return mem_ptr;
}

void free(void *buf)
{
  mutex_lock( &heap_mutex );
  _free(buf);
  mutex_unlock( &heap_mutex );
  return;
}

void *smalloc(size_t size)
{
  mutex_lock( &heap_mutex );
  void *mem_ptr = _smalloc(size);
  mutex_unlock( &heap_mutex );
  return mem_ptr;
}

void *smemalign(size_t alignment, size_t size)
{
  mutex_lock( &heap_mutex );
  void *mem_ptr = _smemalign(alignment, size);
  mutex_unlock( &heap_mutex );
  return mem_ptr;
}

void sfree(void *buf, size_t size)
{
  mutex_lock( &heap_mutex );
  _sfree(buf, size);
  mutex_unlock( &heap_mutex );
  return;
}


