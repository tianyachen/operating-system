/** @file create_new_thread.S
 *  @brief does the essential work of thr_create in setting up new base
 *         and stack pointers for child thread and jumping to new stack
 *
 *  @author Tianya Chen (andrewid: tianyac, email: tianyac@andrew.cmu.edu)
 */

#include <asm_style.h>
#include <syscall_int.h>

.global create_new_thread

create_new_thread:
    PUSH  %ebp                  // push prev frame pointer onto stack
    MOVL  %esp, %ebp            // update frame pointer
    MOVL  8(%ebp), %edx         // move 1st arg (child's function pointer) to edx
    MOVL  12(%ebp), %ecx        // move 2nd arg (child's stack pointer) to ecx
    INT   $THREAD_FORK_INT      // index to thread_fork_int function
    CMP   $0x0, %eax            // check if return value == 0
    JNE   .parent_thread        // jump to parent if ret != 0
    MOVL  %ecx, %esp 			// move to new stack with new stack pointer
    MOVL  %edx, %ebp			// move to new stack with new stack frame pointer
	JMP   run_thr_func	        // jump to run_thr_func()

.parent_thread:
    MOVL  %ebp, %esp            // update stack pointer
    POP   %ebp                  // restore ebp
    RET                         // return to caller function
