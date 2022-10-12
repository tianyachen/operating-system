/** @file autostack.c
 *  @brief autostack handler functions for legacy programs
 *
 *  @author Tianya Chen (andrewid: tianyac, email: tianyakc@gmail.com)
 */

#include <stdio.h>
#include <simics.h>
#include <ureg.h>
#include <syscall.h>
#include <malloc.h>
#include <thr_internals.h>
#include "swexn_internals.h"

static uint32_t g_root_stk_high; ///< the lowest address of root stack 
static uint32_t g_root_stk_low;  ///< the highest address of root stack 
static uint32_t g_swexn_stk_high; ///< the highest address of swexn handler stack
static uint32_t g_swexn_stk_low;  ///< the lowest of swexn handler stack
static uint32_t g_stack_grow_size; ///< the fixed grow size for autostack handler


/** @brief get the highest address of root stack
 *  @returns void
 */
uint32_t get_root_stack_high() {
    return g_root_stk_high;
}

/** @brief get the lowest address of root stack
 *  @returns void
 */
uint32_t get_root_stack_low() {
    return g_root_stk_low;
}

/** @brief the autostack handler to handle exception and grow the stack
 *         for legacy programs
 *  @param arg gerenal type arg to put pass to the handler
 *  @param ureg a pointer the the user register struct
 *  @returns void
 */
void autostack_handler(void *arg, ureg_t *ureg){

  /* only handles page fault */
  if (ureg->cause == SWEXN_CAUSE_PAGEFAULT){
    /* check if the page fault is within the current stack frame
       autostack growth only handles page fault within the current stack frame */
    if (ureg->cr2 <= ureg->ebp && (ureg->cr2 + POINTER32) >= ureg->esp){
      g_root_stk_low -= g_stack_grow_size;
      if(new_pages((void *)g_root_stk_low, g_stack_grow_size) < 0){
        panic("autostack.c: can't allocate more memory to grow the stack.");
      }

      /* re-register the autostack handler */
      if(swexn(esp3, autostack_handler, arg, ureg) < 0){
        panic("autostack.c: swexn() failed.");
      }
    }
  }

}

/** @brief installs a exception handler to handler stack autogrow
 *  @returns void
 */
void install_autostack(void *stack_high, void *stack_low){

  if (stack_high == NULL && stack_low == NULL){
    panic("autostack.c: initial stack_high and/or stack_low are NULL.");
  }

  g_root_stk_high = (uint32_t)stack_high;
  g_root_stk_low = (uint32_t)stack_low;
  g_stack_grow_size = (uint32_t) (stack_high - stack_low);
  g_stack_grow_size = round_up_stack_size(g_stack_grow_size);

  /* allocate the software handler stack on the heap. */
  void* swexn_stack_low = malloc(SWEXN_STACK_SIZE);
  /* if can't allocate enough memeory for the autostack handler to grow, exit */
  if (swexn_stack_low == NULL){
    panic("autostack.c: can't allocate memory for autostack handler.");
  }

  g_swexn_stk_low = (uint32_t)swexn_stack_low;
  g_swexn_stk_high = g_swexn_stk_low + SWEXN_STACK_SIZE;

  esp3 = (void*) (swexn_stack_low + SWEXN_STACK_SIZE);
  esp3 = (void*) (((uint32_t) esp3) & ESP_ALIGN_MASK);

  if(swexn(esp3, autostack_handler, NULL, NULL) < 0){
    panic("autostack.c: swexn() failed.");
  }
}
