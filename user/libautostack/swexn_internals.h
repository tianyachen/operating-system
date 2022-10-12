/** @file swexn_internals.h
 *  @brief specification for internal swexn functions
 *
 *  @author Tianya Chen (andrewid: tianyac, email: tianyac@andrew.cmu.edu)
 */

#include <stdint.h>
#include <ureg.h>

#ifndef _AUTOSTACK_INTERNALS_H
#define _AUTOSTACK_INTERNALS_H

/* PAGE_SIZE is 4096 defined in sepc/syscall.h */
/// size of the swexn stack
#define SWEXN_STACK_SIZE PAGE_SIZE /* This size might be too large for swexn */
#ifndef ESP_ALIGNMENT
#define ESP_ALIGNMENT 4 ///< macro for checking ESP alignment
#endif

#ifndef ESP_ALIGN_MASK
#define ESP_ALIGN_MASK 0xFFFFFF00 ///< macro for aligning ESP forcefully
#endif

#ifndef POINTER32
#define POINTER32 4 ///< increment of a 32 bit aligned pointer
#endif

/** @brief global pointer to swexn stack */
void *esp3;

uint32_t get_root_stack_low();
uint32_t get_root_stack_high();
void install_autostack(void * stack_high, void * stack_low);
void autostack_handler(void *arg, ureg_t *ureg);
void install_swexn(void);
void swexn_handler(void *arg, ureg_t *ureg);

#endif /* _AUTOSTACK_INTERNALS_H */
