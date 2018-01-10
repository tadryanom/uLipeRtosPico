/**
 * 							ULIPE RTOS PICO
 *
 *  @file k_mutex.h
 *
 *  @brief mutual exclusion semaphore stuff
 *
 */
#ifndef __K_MUTEX_H
#define __K_MUTEX_H

#if K_ENABLE_MUTEX > 0

typedef void* mtx_id_t;


/**
 *  @fn mutex_create()
 *  @brief creates a fully initialized mutex control block
 *
 *
 *  @return a kmutex_t control structure ready to use
 */
mtx_id_t mutex_create(void);


/**
 *  @fn mutex_delete()
 *  @brief destroys a previous allocated mutex control block
 *
 *  @param mutex - mutex to be destroyed
 *
 *  @return k_status_ok or error code in case of invalid use
 */
k_status_t mutex_delete(mtx_id_t mtx);


/**
 *  @fn mutex_take()
 *  @brief take a mutex, block if not currently available or returns if try is set to true
 *
 *  @param	m - mutex to be acquired
 *  @param  try - if true, this function returns imediatelly if no mutex available
 *
 *  @return k_status_ok or error in case of invalid value/mutex not available
 */
k_status_t mutex_take(mtx_id_t m, bool try);


/**
 *  @fn mutex_give()
 *  @brief release a previously acquired mutex, only the onwer of mutex can release it!
 *
 *  @param m - Mutex to be released
 *
 *  @return k_status_ok or error in case of invalid value
 */
k_status_t mutex_give(mtx_id_t m);



#endif
#endif
