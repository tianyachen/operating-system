/*! @file GETCHAR.S
 *  @brief contains a C-callable wrapper for the get_char system call
 *
 * @author Carlos Montemayor (andrewid: cmontema, email: carl6256@gmail.com)
 */

#include <asm_style.h>
#include <syscall_int.h>

// Declare ASM functions for C linker
.global getchar

getchar:
  PUSH  %ebp            // push prev frame pointer onto stack
  MOVL  %esp, %ebp      // update frame pointer
  INT   $GETCHAR_INT    // call into GETCHAR system call
  MOVL  %ebp, %esp      // restore stack pointer
  POP   %ebp            // restore frame pointer
  RET                   // exit to your beautiful user code
