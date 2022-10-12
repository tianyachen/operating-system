/*! @file FORK.S
 *  @brief contains a C-callable wrapper for the fork system call
 *
 * @author Carlos Montemayor (andrewid: cmontema, email: carl6256@gmail.com)
 */

#include <asm_style.h>
#include <syscall_int.h>

// Declare ASM functions for C linker
.global fork

fork:
  PUSH  %ebp            // push prev frame pointer onto stack
  MOVL  %esp, %ebp      // update frame pointer
  INT   $FORK_INT       // call into FORK system call
  MOVL  %ebp, %esp      // restore stack pointer
  POP   %ebp            // restore frame pointer
  RET                   // exit to your beautiful user code
