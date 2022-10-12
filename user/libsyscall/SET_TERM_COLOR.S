/*! @file SET_TERM_COLOR.S
 *  @brief contains a C-callable wrapper for the set_term_color system call
 *
 * @author Carlos Montemayor (andrewid: cmontema, email: carl6256@gmail.com)
 */

#include <asm_style.h>
#include <syscall_int.h>

// Declare ASM functions for C linker
.global set_term_color

set_term_color:
  PUSH  %ebp                // push prev frame pointer onto stack
  MOVL  %esp, %ebp          // update frame pointer
  PUSH  %esi                // save callee saved regs to stack
  MOVL  8(%ebp), %esi       // copy input variable from stack
  INT   $SET_TERM_COLOR_INT // call into SET_TERM_COLOR system call
  POP   %esi                // restore callee saved regs
  MOVL  %ebp, %esp          // restore stack pointer
  POP   %ebp                // restore frame pointer
  RET                       // exit to your beautiful user code
