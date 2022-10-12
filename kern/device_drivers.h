#ifndef _DEVICE_DRIVERS_H_
#define _DEVICE_DRIVERS_H_

/* defines constants */
#define EIGHT_BITS 8
#define SIXTEEN_BITS 16
#define TIMER_INTERRUPT_RATE 11931
#define TIMER_TICK_CALIBRATE_NUM 14551
#define KEYBOARD_BUF_SIZE 256
#define CHAR_SPACE 0x20
#define CHAR_NEWLINE 0xA
#define CHAR_BACKSPACE 0x8
#define CHAR_CARRIAGE 0xD

/* type define tick callback fucntion prototype */
typedef void fn_tick_t(unsigned int);

/* type define interrupt handler */
typedef void int_handler_t(void);

/** @brief Tick function, to be called by the timer interrupt handler
 * 
 *  In a real game, this function would perform processing which
 *  should be invoked by timer interrupts.
 *
 **/
void tick(unsigned int numTicks);

void configure_timer(void);
void link_tick_funct(fn_tick_t *tick);
void install_int_handler(int_handler_t *handler_ptr, int index);
void keyboard_int_handler(void);

/** @brief Wrapper function for keyboard handler.
 * 
 *  Use assembly to push all general purpose registers to stack, call handler
 *  and restores the register state and return.
 *
 *  @return void.
 **/
void keyboard_int_handler_wrapper(void);
void timer_int_handler(void);

/** @brief Wrapper function for timer handler.
 * 
 *  Use assembly to push all general purpose registers to stack, call handler
 *  and restores the register state and return.
 *
 *  @return void.
 **/
void timer_int_handler_wrapper(void);

#endif /* _DEVICE_DRIVERS_H_ */