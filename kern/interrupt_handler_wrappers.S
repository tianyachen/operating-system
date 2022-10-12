/* define global function labels so that they can be called from
 * other files (.c or .S) */

.global timer_int_handler_wrapper
timer_int_handler_wrapper:
	pusha                  /* saves all general purpose registers on stack */            
	call timer_int_handler     /* calls timer handler in timer_driver.c */
	popa                   /* restores all general purpose registers */
	IRET                   /* returns to kernel code */

.global keyboard_int_handler_wrapper
keyboard_int_handler_wrapper:
	pusha                  /* saves all general purpose registers on stack */            
	call keyboard_int_handler  /* calls keyboard handler in timer_driver.c */
	popa                   /* restores all general purpose registers */
	IRET                   /* returns to kernel code */