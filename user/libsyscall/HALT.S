/*! @file HALT.S
 *  @brief contains a C-callable wrapper for the HALT syscall
 *
 * @author Carlos Montemayor (andrewid: cmontema, email: carl6256@gmail.com)
 */

#include <asm_style.h>
#include <syscall_int.h>

// Declare ASM functions for C linker
.global halt

halt:
  PUSH  %ebp            // push prev frame pointer onto stack
  MOVL  %esp, %ebp      // update frame pointer
  MOVL  8(%ebp), %esi   // copy input variable from stack
  INT   $HALT_INT       // call into HALT system call
  // END OF FUNCTION, HALT STOPS THE KERNEL
