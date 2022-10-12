/** @file handler_installation.c
 *
 *  @brief Functions that install interrupt handler functions. 
 *  
 *
 *  @author Tianya Chen (tianyac)
 *  @bug No known bugs.
 */

#include <x86/asm.h>
#include <x86/seg.h>  
#include <simics.h>    
#include <ctype.h>
#include <string.h>
#include <timer_defines.h>
#include <keyhelp.h>

#include "device_drivers.h"

/* please see intel-sys.pdf page 151 for trap gate descriptor
   the flags are defined in binary as 1000 1111 0000 0000 */
#define TRAP_GATE_FLAGS 0x8f00
#define ENTRY_SIZE	8


/*! The trap gate struct type to be inserted into the IDT entry */
typedef struct {
	unsigned short offset_0_15;      /*!< offset from 0 bit 15 bit. */ 
	unsigned short segment_selector;  /*!< segment selector. */ 
	unsigned short flags;       /*!< flags and privilege level. */ 
    unsigned short offset_16_31;  /*!< offset from 16 bit 31 bit. */ 
}trap_gate_t;

/** @brief Installs a given interrupt handler
 *
 *
 *  @param handler_ptr Pointer to the handler function
 *  @param index Insert index to the IDT table entry
 *
 *  @return void
 **/
void install_int_handler(int_handler_t *handler_ptr, int index){
	trap_gate_t idt_entry;
    // fisrt lower 16 bit function address
	idt_entry.offset_0_15 = (unsigned short)((unsigned int)handler_ptr);
    idt_entry.segment_selector = SEGSEL_KERNEL_CS;
	idt_entry.flags = TRAP_GATE_FLAGS;
    // second higher 16 bit function address
	idt_entry.offset_16_31 = 
        (unsigned short)(((unsigned int)handler_ptr) >> SIXTEEN_BITS);

	// put the entry to idt vactor
	void *base = idt_base();
	void *idt_offset = (void *)((char *)base + ENTRY_SIZE * index);
	memcpy(idt_offset, (void *)&idt_entry, sizeof(trap_gate_t));
}

/** @brief The driver-library initialization function
 *
 *   Installs the timer and keyboard interrupt handler.
 *   NOTE: handler_install should ONLY install and activate the
 *   handlers; any application-specific initialization should
 *   take place elsewhere.
 *
 *   @param tickback Pointer to clock-tick callback function
 *
 *   @return A negative error code on error, or 0 on success
 **/
int handler_install(void (*tickback)(unsigned int)) 
{
	configure_timer();
	link_tick_funct(tickback);
	install_int_handler(timer_int_handler_wrapper, TIMER_IDT_ENTRY);
	install_int_handler(keyboard_int_handler_wrapper, KEY_IDT_ENTRY);

	return 0;
}