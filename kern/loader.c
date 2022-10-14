/**
 * The 15-410 kernel project.
 * @name loader.c
 *
 * Functions for the loading
 * of user programs from binary 
 * files should be written in
 * this file. The function 
 * elf_load_helper() is provided
 * for your use.
 */
/*@{*/

/* --- Includes --- */
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <exec2obj.h>
#include <loader.h>
#include <elf_410.h>


/* --- Local function prototypes --- */ 


/**
 * Copies data from a file into a buffer.
 *
 * @param filename   the name of the file to copy data from
 * @param offset     the location in the file to begin copying from
 * @param size       the number of bytes to be copied
 * @param buf        the buffer to copy the data into
 *
 * @return returns the number of bytes copied on succes; -1 on failure
 */
int getbytes( const char *filename, int offset, int size, char *buf )
{
  if ( size < 0 || offset < 0 || (!buf) ){
    return -1;
  }

  for ( int i = 0; i < exec2obj_userapp_count; i++ ){
    const exec2obj_userapp_TOC_entry *entry = &exec2obj_userapp_TOC[i];
    if (strncmp(entry->execname, filename, MAX_EXECNAME_LEN) == 0){
      if (offset > entry->execlen){
        return -1;
      }
      int ret_len = min(entry->execlen - offset, size);
      memcpy(buf, entry->execbytes + offset, ret_len);
      return ret_len;
    }
  }

  return -1;
}

int load(char *filename, char *argv[], uint32_t *eip, uint32_t *esp){
  if (elf_load_helper(&se_hdr, filename) != ELF_SUCCESS) {
    return -1;
  }
  // allocate_memory(se_hdr, argc, argv)
  // eip = entry_point
  // esp = stack_base;
  // fill mem, new pages()
  return -1;
}

int exec(char *filename, char *argv[]){
  uint32_t eip, esp;
  if (load(new_filename, argv, &eip, &esp) < 0){
    return -1;
  }

  //jump(eip);
}

/*@}*/
