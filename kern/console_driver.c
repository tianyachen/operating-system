/** @file console_driver.c
 *
 *  @brief Implementation of console device driver library.
 *  
 *
 *  @author Tianya Chen (tianyac)
 *  @bug No known bugs.
 */

#include <x86/asm.h>
#include <p1kern.h> // includes all driver interfaces declarations
#include <stdio.h>
#include <simics.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include "device_drivers.h"

/* static variables  */
static bool is_cursor_hidden = true; /**< cursor hidden flag */
static int cursor_row = 0;  /**< current cusor row */
static int cursor_col = 0;  /**< current cusor column */
static int color_code = FGND_WHITE | BGND_BLACK; /**< color code to draw char */

/* static functions */
static void scroll_screen(int num_rows);
static void set_hardware_cursor(int row, int col);


/** @brief Prints character ch at the current location of the cursor.
 * 
 *  If the character is a newline ('\n'), 
 *  the cursor should be moved to the first column of 
 *  the next line (scrolling if necessary).  
 *
 *  If the character is a carriage return ('\r'), 
 *  the cursor should be immediately reset to the beginning of the current line,
 *  causing any future output to overwrite any existing output on the line.
 *
 *  If backspace ('\b') is encountered, the previous character should be erased.
 *
 *  @param ch the character to put at current cursor location.
 *  @return the input character.
 */
int putbyte( char ch )
{
    if (ch == '\n'){
        set_cursor(cursor_row + 1, 0);
        return ch;
    } else if (ch == '\r'){
        set_cursor(cursor_row, 0);
        return ch;
    } else if (ch == '\b'){
        // if not at the beginning of a line, replace prev char with space
        if (cursor_col > 0){ 
            set_cursor(cursor_row, cursor_col - 1);
            // ascii 0x20 is the space character
            draw_char(cursor_row, cursor_col, CHAR_SPACE, color_code);
        }
        return ch;
    } else {
        draw_char(cursor_row, cursor_col, ch, color_code);
        set_cursor(cursor_row, cursor_col + 1);
    }
    return ch;
}

/** @brief Prints the character array s, starting at the current 
 *  location of the cursor.
 * 
 *  If there are more characters than there are spaces 
 *  remaining on the current line, the characters should fill up the current 
 *  line and then continue on the next line.
 * 
 *  If the characters exceed available space on the entire console,
 *  the screen should scroll up one line, 
 *  and then printing should continue on the new line.
 * 
 *  If '\n', '\r', and '\b' are encountered within the string, 
 *  they should be handled as per putbyte.
 *  
 *  If len is not a positive integer or s is null, 
 *  the function has no effect. 
 *
 *  @param s the immutable input string.
 *  @param len the length of the input bytes.
 *  @return void.
 */
void putbytes( const char *s, int len ){
    if (s == NULL || len < 0) return;
    const char *ch;
    for (int i=0; i < len; ++i){
        ch = s + i; // pointer arithmetic
        putbyte(*ch);
    }
}

/** @brief Changes the foreground and background 
 *  color of future characters printed on the console.
 *
 *  If the color code is invalid, the function has no effect.
 * 
 *  @param color the new color code.
 *  @return 0 on success, or -1 if color code is invalid.
 */
int set_term_color( int color ){
    if (color > BLINK) { // blink is the highest color number
        return -1;
    }
    color_code = color;
    return 0;
}

/** @brief Writes the current foreground and background color of characters
 *  printed on the console into the argument color. 
 * 
 *  @param color the address to which the current 
 *  color information will be written.
 *  @return void.
 */
void get_term_color( int *color ) {
    if (color != NULL) {
        *color = color_code;
    }
}

/** @brief Sets the position of the cursor to the position (row, col). 
 *
 *  Subsequent calls to putbyte or putbytes should cause the console 
 *  output to begin at the new position.
 * 
 *  @param row the new row for the cursor.
 *  @param col the new column for the cursor.
 *  @return 0 on success, or -1 if cursor location is invalid.
 */
int set_cursor( int row, int col ) {
	if (row < 0 || col < 0 || row > CONSOLE_HEIGHT || col > CONSOLE_WIDTH) {
		// invalid cursor location
		return -1;
	}

    if (col == CONSOLE_WIDTH) {
        // move to the next line
        col = 0;
        row++;
    }

    if (row >= CONSOLE_HEIGHT) {
        // scrolls one row up
        scroll_screen(1);
        // sets the cursor to last row on the console
        row = CONSOLE_HEIGHT - 1;
    }
    // sets the logical cursor
    cursor_col = col;
    cursor_row = row;

    // sets the hardware cursor
    if (is_cursor_hidden == false){
        // sets the hardware to the logical cursor
        set_hardware_cursor(cursor_row, cursor_col);
    } 
    return 0;
}

/** @brief Writes the current position of the cursor 
 *  into the arguments row and col.
 * 
 *  @param row the address to which the current cursor row will be written.
 *  @param col the address to which the current cursor column will be written.
 *  @return void.
 */
void get_cursor( int *row, int *col ){
    if (row != NULL) *row = cursor_row;
    if (col != NULL) *col = cursor_col;
}

/** @brief Causes the cursor to become invisible, without changing its location.
 * 
 *  Subsequent calls to putbyte or putbytes must not cause the 
 *  cursor to become visible again.
 *
 *  If the cursor is already invisible, the function has no effect.
 *
 *  @return void.
 */
void hide_cursor(void){
    is_cursor_hidden = true;
    set_hardware_cursor(CONSOLE_HEIGHT, CONSOLE_WIDTH);
}

/** @brief Causes the cursor to become visible, without changing its location.
 * 
 *  set the hardware cursor to the logical cursor location.
 *  If the cursor is already visible, the function has no effect.
 *
 *  @return void.
 */
void show_cursor(void){
    is_cursor_hidden = false;
    set_hardware_cursor(cursor_row, cursor_col);
}

/** @brief Clears the entire console and resets the cursor 
 *  to the home position (top left corner). 
 * 
 *  If the cursor is currently hidden it should stay hidden.
 *
 *  @return void.
 */
void clear_console(void) {
	char* start = (char*)CONSOLE_MEM_BASE;
	char* end = ((char*)(CONSOLE_MEM_BASE) + 2 * CONSOLE_HEIGHT * CONSOLE_WIDTH);

	while (start < end) {
		start[0] = CHAR_SPACE;
		start += 2;  // skips the color bytes
	}

    set_cursor(0, 0);
}

/** @brief Prints character ch with the specified color at position (row, col).
 * 
 *  If any argument is invalid, the function has no effect.
 *
 *  @param row the row in which to display the character.
 *  @param col the column in which to display the character.
 *  @param ch the character to display.
 *  @param color the color to use to display the character.
 *  @return void.
 */
void draw_char( int row, int col, int ch, int color ){
    if (row < 0 || row >= CONSOLE_HEIGHT || col < 0 || col >= CONSOLE_WIDTH ||
        (!isprint(ch)) || color > BLINK){
            return;
    }

    *(char *)(CONSOLE_MEM_BASE + 2 * (row * CONSOLE_WIDTH + col)) = ch;
    *(char *)(CONSOLE_MEM_BASE + 2 * (row * CONSOLE_WIDTH + col) + 1) = color;

}

/** @brief Returns the character displayed at position (row, col).
 * 
 *  If any argument is invalid, the function has no effect.
 *
 *  @param row the row of the character.
 *  @param col the column of the character.
 *  @return the character at (row, col).
 */
char get_char( int row, int col ){
    if (row < 0 || row >= CONSOLE_HEIGHT || col < 0 || col >= CONSOLE_WIDTH){
        return 0; // NULL char
    }
    return *(char *)(CONSOLE_MEM_BASE + 2 * (row * CONSOLE_WIDTH + col));
}

/** @brief Scrolls the screen up with number rows. 
 *  If the number of rows is positive, the screen scrolls up. 
 *  If the number of rows is zero/negtive, the screen does nothing.
 *
 *  @param num_rows the number of rows to scroll up.
 *  @return void.
 */
static void scroll_screen(int num_rows) {

    // a two-byte pair represents a character+color on console
	void *new_pos = (void*)(CONSOLE_MEM_BASE + 2*(num_rows * CONSOLE_WIDTH));
	int size = 2 *(CONSOLE_HEIGHT - num_rows)* CONSOLE_WIDTH ;

	memmove((void *)CONSOLE_MEM_BASE, new_pos, size);

    // clear the last row
    char *last_row = (char *)(CONSOLE_MEM_BASE + 2 * 
                     (CONSOLE_HEIGHT-1) * CONSOLE_WIDTH);
	for(int i=0; i < CONSOLE_WIDTH; i++){
		last_row[i * 2] = CHAR_SPACE;	
	}
}

/** @brief Sets the hardware cursor position on the console. 
 *
 *  If the hardware cursor is beyond the range of console size,
 *  it is hidden. 
 *  If the row and col is negative, the function does nothing.
 *  It doesn't change the global cursor_row and cursor_col.
 *
 *  @param row the row index to hardware cursor.
 *  @param col the col index to hardware cursor.
 *  @return void.
 */
static void set_hardware_cursor(int row, int col) {
    if (row < 0 || col < 0) return;

	unsigned short cursor_position = 
        (unsigned short)(CONSOLE_WIDTH * row + col);

	/* send higher 8 bits first, SMB = most sigificant bits */
	outb(CRTC_IDX_REG, CRTC_CURSOR_MSB_IDX);
	outb(CRTC_DATA_REG, cursor_position >> EIGHT_BITS);

	/* send lower 8 bits second, LSB = least sigificant bits */
	outb(CRTC_IDX_REG, CRTC_CURSOR_LSB_IDX);
	outb(CRTC_DATA_REG, cursor_position);
}
