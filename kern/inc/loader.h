/* The 15-410 kernel project
 *
 *     loader.h
 *
 * Structure definitions, #defines, and function prototypes
 * for the user process loader.
 */

#ifndef _LOADER_H
#define _LOADER_H
     
#include <stdint.h>

#define min(a, b)	((a) < (b) ? (a) : (b))

/* --- Prototypes --- */

int getbytes( const char *filename, int offset, int size, char *buf );
int load(char *filename, char *argv[], uint32_t *eip, uint32_t *esp);

#endif /* _LOADER_H */
