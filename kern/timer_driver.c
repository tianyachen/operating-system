/** @file timer_driver.c
 *
 *  @brief Implementation of timer device driver library.
 *
 *  The internal rate of the PC timer is 1193182HZ, so there are 
 *  about 11931 cycles between each timer interript interval.
 *  11931 cycles is not 10 ms but about 9.999312762 ms.
 *  This kernel can't use floating points and one tick should be close to 10 ms.
 *  Therefore, every interupt the timer is (10 - 9.999312762) ms 
 *  faster than the PC time. The driver needs to ignore certain ticks to
 *  calibrate this error. Specifically, for every 10 / (10 - 9.999312762) ticks
 *  ignores one tick, (ignore one tick in every 14551 ticks). 
 *
 *  @author Tianya Chen (tianyac)
 *  @bug No known bugs.
 */

#include <x86/asm.h>
#include <limits.h>
#include <simics.h> 
#include <string.h>
#include <timer_defines.h>
#include <x86/interrupt_defines.h>
#include <stddef.h>

#include "device_drivers.h"


static int num_ticks = 0;       /**< the num of ticks from beginning */
static fn_tick_t *tick_callback = NULL; /**< the tick call back function */

/** @brief Configures the timer, i.e, intterupt cycles.
 *
 *   11931 cycles are raoughly 10 milliseconds with 1193182 Hz PC timer. 
 *
 *   @return void.
 **/
void configure_timer(void){
    outb(TIMER_MODE_IO_PORT, TIMER_SQUARE_WAVE);

    unsigned short num_cycles_per_tick = TIMER_INTERRUPT_RATE;

    // sends lower bits first
    outb(TIMER_PERIOD_IO_PORT, num_cycles_per_tick);

    // then sends higher bits 
    outb(TIMER_PERIOD_IO_PORT, num_cycles_per_tick >> EIGHT_BITS);
}

/** @brief Links the app tick call back function with the timer handler.
 *
 *   @return void.
 **/
void link_tick_funct(fn_tick_t *tick){
    tick_callback = tick;
}

/** @brief Handles timer intterupts. 
 *
 *   For every 14551 ticks, ignores one of them to calibrate the time. 
 *
 *   @return void.
 **/
void timer_int_handler(void){
    if (num_ticks && num_ticks % TIMER_TICK_CALIBRATE_NUM == 0){
        // ignores the ticks
    } else {
        num_ticks++;
        if (tick_callback) {
            tick_callback(num_ticks);
        }
    }
    // sends msg to PIC telling interrupt is processed. 
    outb(INT_CTL_PORT, INT_ACK_CURRENT);
}