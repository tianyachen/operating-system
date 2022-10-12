/** @file keyboard_driver.c
 *
 *  @brief Implementation of keyboard device driver library.
 *  
 *
 *  @author Tianya Chen (tianyac)
 *  @bug No known bugs.
 */

#include <x86/asm.h>
#include <p1kern.h> // includes all driver interfaces declarations
#include <keyhelp.h>
#include <malloc.h>
#include <stddef.h>
#include <simics.h>
#include <string.h>
#include <x86/interrupt_defines.h>

#include "device_drivers.h"

static char keyboard_buf[KEYBOARD_BUF_SIZE]; /**< the keyboard buffer */
static int read_pos_idx = 0;     /**< the index to read char */  
static int buf_end_idx = 0;   /**< the index to insert the next byte */  

/** @brief Handles keyboard interrupts
 *
 *  This function blocks future keyboard inturrepts but not other interrupts
 *
 *  @return void
 **/
void keyboard_int_handler(void) {
    // if at the end of buffer size, wraps around to the beginning
    int next_idx = (buf_end_idx + 1) % KEYBOARD_BUF_SIZE;
    if (next_idx == read_pos_idx){
        // keyboard buffer is overflowed!
        // actually, the buffer is one byte from overflow!
        return;
    }
    // scancode is one byte
	keyboard_buf[buf_end_idx] = inb(KEYBOARD_PORT);

	// update buf_end_idx
	buf_end_idx = next_idx;

    // sends msg to PIC telling interrupt is processed. 
    outb(INT_CTL_PORT, INT_ACK_CURRENT);
}

/** @brief Returns the next character in the keyboard buffer
 *
 *  This function does not block if there are no characters in the keyboard
 *  buffer
 *
 *  @return The next character in the keyboard buffer, or -1 if the keyboard
 *          buffer does not currently contain a valid character.
 **/
int readchar(void) {
    // the function should block all interrupts
    disable_interrupts();

    kh_type augmented_key;
    for (; read_pos_idx != buf_end_idx; 
         read_pos_idx = (read_pos_idx + 1) % KEYBOARD_BUF_SIZE){
            // scancode is one byte
            augmented_key = process_scancode(keyboard_buf[read_pos_idx]);

            if(KH_HASDATA(augmented_key) && KH_ISMAKE(augmented_key)){
                read_pos_idx = (read_pos_idx + 1) % KEYBOARD_BUF_SIZE;
			    enable_interrupts();
		        return KH_GETCHAR(augmented_key);
	        }
    }

    enable_interrupts();
    // buffer doesn't contain any valid character.
    return -1;
}

/** @brief Reads a line of characters into a specified buffer
 *
 * If the keyboard buffer does not already contain a line of input,
 * readline() will spin until a line of input becomes available.
 *
 * If the line is smaller than the buffer, then the complete line,
 * including the newline character, is copied into the buffer. 
 *
 * If the length of the line exceeds the length of the buffer, only
 * len characters should be copied into buf.
 *
 * Available characters should not be committed into buf until
 * there is a newline character available, so the user has a
 * chance to backspace over typing mistakes.
 *
 * While a readline() call is active, the user should receive
 * ongoing visual feedback in response to typing, so that it
 * is clear to the user what text line will be returned by
 * readline().
 *
 *  @param buf Starting address of buffer to fill with a text line
 *  @param len Length of the buffer
 *  @return The number of characters in the line buffer,
 *          or -1 if len is invalid or unreasonably large.
 **/
int readline(char *buf, int len) {
    // creates a temp buffer
    char* temp_buf = NULL;
    temp_buf = (char *)malloc(len);
    if (temp_buf == NULL) {
        lprintf( "Keyboard driver: malloc failed!" );
        return -1;
    }
    int temp_buf_end_idx = 0; /* the index to put the next char */
    int ch = 0;
    int ret_num_chars = 0;

    while ((ch = readchar()) != CHAR_NEWLINE) {
        // removes previous characters
        if (ch == -1){
            continue;
        } else if (ch == CHAR_BACKSPACE){
            if (temp_buf_end_idx > 0){
                putbyte(ch);
                temp_buf_end_idx--;
            }
        } else if (ch == CHAR_CARRIAGE){
            // undefined behavior, currently overwrites the prompts.
            putbyte(ch);
        } else {
            temp_buf[temp_buf_end_idx++] = ch;
            if (temp_buf_end_idx >= len){
                memcpy(buf, temp_buf, len);
                temp_buf = (char *)realloc(temp_buf, 2 * temp_buf_end_idx);
                if (temp_buf == NULL){
                    lprintf( "Keyboard driver: realloc failed!" );
                    return len;
                }
            }
            putbyte(ch);
        }
    }

    if (temp_buf_end_idx < (len - 1)){
        // adds a new line char
        temp_buf[temp_buf_end_idx] = CHAR_NEWLINE;
        ret_num_chars = temp_buf_end_idx + 1;
        memcpy(buf, temp_buf, ret_num_chars);
    } else {
        // copys only part of the line
        memcpy(buf, temp_buf, len);
        ret_num_chars = len;
    }

    // free tem buffer
    free(temp_buf);
    return ret_num_chars;
}