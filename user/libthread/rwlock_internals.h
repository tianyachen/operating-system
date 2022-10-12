/** @file rwlock_internals.h
 *  @brief helper function and additional data structure defs for rwlock library
 *
 *  @author Carlos Montemayor (andrewid: cmontema, email: carl6256@gmail.com)
 */

#ifndef _RWLOCK_INTERNAL_H
#define _RWLOCK_INTERNAL_H

#include <rwlock.h>

int append_rw( rwlock_t* rwlock, int type );
int dequeue( rwlock_t* rwlock );

#endif
