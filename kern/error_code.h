/** @file error_code.h
 *  @brief header file for all error code macros
 *
 */

/**** Error Codes ****/
/// indicates a successful return
#define SUCCESS_RETURN 0
/// indicates an input was NULL
#define ERROR_NULL_POINTER -1
/// indicates a synchronization primitive was initialized twice
#define ERROR_DOUBLE_INITIALIZATION -2
/// indicates the semaphore count is inan invalid state
#define ERROR_SEM_ILLEGAL_CNT -3
/// indicates the thread initialization failed
#define ERROR_THR_INIT_FAILED -4
/// indicates thread creation failed
#define ERROR_THR_CREATE_FAILED -5
/// indicates stack metadata initialization failed
#define ERROR_INIT_STACK_META_FAILED -6
/// indicates the tid passed in is invalid
#define ERROR_INVALID_TID -7
/// indicates the TID passed in to yield is suspended
#define ERROR_YIELD_SUSPENDED_TID -8
/// indicates a malloc call failed
#define ERROR_MALLOC_FAILED -9
/// indicates multiple threads tried to join the same thread
#define ERROR_MULTIPLE_JOINS -10
/// indicates a synchronization primitive was initialized while occupied
#define ERROR_INIT_ON_USE -11
