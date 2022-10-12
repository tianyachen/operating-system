/*! @file TASK_VANISH.S
 *  @brief contains a C-callable wrapper for the task_vanish system call
 *
 * @author Carlos Montemayor (andrewid: cmontema, email: carl6256@gmail.com)
 */

#include <asm_style.h>
#include <syscall_int.h>

// Declare ASM functions for C linker
.global task_vanish

task_vanish:
  PUSH  %ebp            // push prev frame pointer onto stack
  MOVL  %esp, %ebp      // update frame pointer
  MOVL  8(%ebp), %esi   // copy input variable from stack
  INT   $TASK_VANISH_INT// call into TASK_VANISH system call
