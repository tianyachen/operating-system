/** @file atomic_increment.S
 *  @brief wrapper function for the intel xdd instruction
 *
 *  @author Tianya Chen (tianyac)
 */


.global atomic_increment

atomic_increment:
    PUSH        %ebp          // push old frame pointer onto stack
    MOVL        %esp, %ebp    // set frame pointer
    MOVL        8(%ebp), %edx // move address of ticket_num to %edx
    MOVL        $1, %ecx      // move 1 to %ecx
    LOCK XADD   %ecx, (%edx)  // (%edx) + 1, and load to (%edx)
    MOVL        %ecx, %eax    // move old value to %eax
    MOVL        %ebp, %esp    // restore stack pointer
    POP         %ebp          // restore frame pointer
    RET                       // returns %eax
