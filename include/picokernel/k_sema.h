/**
 * 							ULIPE RTOS PICO
 *
 *  @file k_sema.h
 *
 *  @brief basic semaphore usage header file
 *
 */
#ifndef __K_SEMA_H
#define __K_SEMA_H

#if K_ENABLE_SEMAPHORE > 0

#if(K_ENABLE_DYNAMIC_ALLOCATOR > 0)


/**
 *  @fn semaphore_create_dynamic()
 *  @brief creates a fully initialized semaphore control block
 *
 *  @param name - name of sempahore block control structure, used as parameters on Semaphore API
 *  @param initial - initial counting available of semaphore
 *  @param limit_val - counting of maximum available acquisitions of semaphore
 *
 *  @return a ksema_t control structure ready to use
 */
ksema_t * semaphore_create_dynamic(uint32_t initial, uint32_t limit);


/**
 *  @fn semaphore_delete_dynamic()
 *  @brief destroys a previous allocated semaphore control block
 *
 *  @param sem - semaphore to be destroyed
 *
 *  @return k_status_ok or error code in case of invalid use
 */
k_status_t semaphore_delete_dynamic(ksema_t * sem);






#endif

/**
 *  @fn semaphore_take()
 *  @brief take a semaphore, if not available blocks the current task
 *
 *  @param s - semaphore to be acquired
 *
 *  @return k_status_ok or error code in case of invalid value
 */
k_status_t semaphore_take(ksema_t *s);


/**
 *  @fn semaphore_give()
 *  @brief release a previously acquired semaphore
 *
 *  @param s- semaphore acquired
 *  @param count - amount of acquisitions to be released (cannot be greater than limit value)
 *
 *  @return k_status_ok or error code in case of invalid value
 */
k_status_t semaphore_give(ksema_t *s, uint32_t count);






#endif
#endif
