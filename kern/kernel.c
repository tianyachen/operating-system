/** @file kernel.c
 *  @brief An initial kernel.c
 *
 *  You should initialize things in kernel_main(),
 *  and then run stuff.
 *
 *  @author Harry Q. Bovik (hqbovik)
 *  @author Fred Hacker (fhacker)
 *  @bug No known bugs.
 */

#include <common_kern.h>

/* libc includes. */
#include <stdio.h>
#include <simics.h>                 /* lprintf() */

/* multiboot header file */
#include <multiboot.h>              /* boot_info */

/* x86 specific includes */
#include <x86/asm.h>                /* enable_interrupts() */

#include <malloc.h>
#include <handler_installation.h>
#include <device_drivers.h>

volatile static int __kernel_all_done = 0;

/** @brief Kernel entrypoint.
 *  
 *  This is the entrypoint for the kernel.
 *
 * @return Does not return
 */
int kernel_main(mbinfo_t *mbinfo, int argc, char **argv, char **envp)
{
    if (malloc_init() < 0){
        panic("kernel_main: malloc_init failed!");
    }

    if (handler_install(tick) < 0){
        panic("kernel_main: handler_install failed!");
    }

    // initialzie other stuff !
    // enable_interrupts()
    // clear_console();



    /*
     * When kernel_main() begins, interrupts are DISABLED.
     * You should delete this comment, and enable them --
     * when you are ready.
     */

    lprintf( "Hello from a brand new kernel!" );

    while (!__kernel_all_done) {
        continue;
    }

    return 0;
}
