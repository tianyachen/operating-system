/*! @file VANISH.S
 *  @brief contains a C-callable wrapper for the vanish syscall
 *
 * @author Carlos Montemayor (andrewid: cmontema, email: carl6256@gmail.com)
 */

#include <asm_style.h>
#include <syscall_int.h>

// Declare ASM functions for C linker
.global vanish

vanish:
  PUSH  %ebp            // push prev frame pointer onto stack
  MOVL  %esp, %ebp      // update frame pointer
  INT   $VANISH_INT     // call into VANISH system call
  // END OF FUNCTION, HALT STOPS THE KERNEL
