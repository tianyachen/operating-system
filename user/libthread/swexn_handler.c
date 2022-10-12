/** @file swexn_handler.c
 *  @brief software exception handler to handle thread crashes
 *
 *  @author Tianya Chen       (andrewid: tianyac, email: tianyac@andrew.cmu.edu)
 */


#include <assert.h>
#include <simics.h>
#include <stdio.h>
#include <stddef.h>
#include <syscall.h>
#include <swexn_internals.h>
#include <thr_internals.h>
#include <cond.h>

/** @brief installs a general exception handler
 *  @returns void
 */
void install_swexn(){
    /* register a software exception handler */
    if(swexn(esp3, swexn_handler, NULL, NULL) < 0){
        panic("install_swexn: failed to register a swexn handler.");
    }
}

/** @brief the software exception handler for thread crash. 
 *  @param arg gerenal type arg to put pass to the handler
 *  @param ureg a pointer the the user register struct
 *  @returns void
 */
void swexn_handler(void *arg, ureg_t *ureg){
    /* See intel-sys.pdf section 5.12, "Exception and Interrupt Reference" */
    switch(ureg->cause){
        case SWEXN_CAUSE_DIVIDE:
            printf("swexn: Divide Error Exception\n");
            break;
        case SWEXN_CAUSE_DEBUG:
            printf("swexn: Debug Exception\n");
            break;
        case SWEXN_CAUSE_BREAKPOINT:
            printf("swexn: Breakpoint Exception\n");
            break;
        case SWEXN_CAUSE_OVERFLOW:
            printf("swexn: Overflow Exception\n");
            break;
        case SWEXN_CAUSE_BOUNDCHECK:
            printf("swexn: BOUND Range Exceeded Exception\n");
            break;
        case SWEXN_CAUSE_OPCODE:
            printf("swexn: Invalid Opcode Exception\n");
            break;
        case SWEXN_CAUSE_NOFPU:
            printf("swexn: Device Not Available Exception\n");
            break;
        case SWEXN_CAUSE_SEGFAULT:
            printf("swexn: Segment Not Present\n");
            break;
        case SWEXN_CAUSE_STACKFAULT:
            printf("swexn: Stack Fault Exception\n");
            break;
        case SWEXN_CAUSE_PROTFAULT:
            printf("swexn: General Protection Exception\n");
            break;
        case SWEXN_CAUSE_PAGEFAULT:
            printf("swexn: Page-Fault at %p, on instruction: %p\n", 
                    (void*)ureg->cr2, (void*)ureg->eip);
            break;
        case SWEXN_CAUSE_FPUFAULT:
            printf("swexn: x87 FPU Floating-Point Error\n");
            break;
        case SWEXN_CAUSE_ALIGNFAULT:
            printf("swexn: Alignment Check Exception\n");
            break;
        case SWEXN_CAUSE_SIMDFAULT:
            printf("swexn: SIMD Floating-Point Exception\n");
            break;
        default:
            printf("swexn: Unknown Exception\n");
    }

    thr_stack_meta_t *crashed_thread = find_thread_meta_by_ebp((uint32_t)ureg->ebp);

    mutex_lock(&(crashed_thread->meta_mutex));
    crashed_thread->exit_status = crashed_thread->arg; //choose excellent answer
    crashed_thread->thr_state = TERMINATED;

    if (crashed_thread->join_flag == JOINING){
        cond_signal( &(crashed_thread->meta_cv) );
    } 
    mutex_unlock(&(crashed_thread->meta_mutex));
    

    /* print out some useful info */
    printf("Crashed thread: %d \n", crashed_thread->tid);

    /* terminate the thread */
    vanish();
}
