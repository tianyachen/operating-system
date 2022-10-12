/** @file read_ebp.S
 *
 *  @brief contains an assembly routine to simply read the current base pointer
 *  @author Tianya Chen (tianyac)
 */

/* define read_ebp label so that they can be called from
 * other files (.c or .S) */
.global read_ebp

/* The function doesn't use any callee save registers or the stack
 * so it chooses not to save them.
 */
read_ebp:
    movl %ebp, %eax   // move the traceback %ebp to the return register %eax
    ret
