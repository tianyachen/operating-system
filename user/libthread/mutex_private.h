/** @file mutex_private.h
 *  @brief designator for mutex struct/union declarations, linker function
 *         references, and defines
 *
 * @author Carlos Montemayor (andrewid: cmontema, email: carl6256@gmail.com)
 * @author Tianya Chen (andrewid: tianyac, email: tianyac@andrew.cmu.edu)
 */

/***** ASM FUNCTION REFERENCES *****/
/** @brief a function that increments pointer atomically
 *  @param ticket_num pointer to integer to increment atomically
 *  @return previous value of integer in the pointer
 */
unsigned int atomic_increment(unsigned int* ticket_num);

