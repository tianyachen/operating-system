#ifndef _HANDLER_INSTALLATION_H
#define _HANDLER_INSTALLATION_H


/*********************************************************************/
/*                                                                   */
/* Interface for device-driver initialization and timer callback     */
/*                                                                   */
/*********************************************************************/

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
int handler_install(void (*tickback)(unsigned int));

#endif /* _HANDLER_INSTALLATION_H */