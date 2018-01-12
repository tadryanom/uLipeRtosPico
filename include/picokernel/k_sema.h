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

/** semaphore id type to use semaphore objects */
typedef void* sema_id_t;


/**
 *  @fn semaphore_create()
 *  @brief creates a fully initialized semaphore control block
 *
 *  @param name - name of sempahore block control structure, used as parameters on Semaphore API
 *  @param initial - initial counting available of semaphore
 *  @param limit_val - counting of maximum available acquisitions of semaphore
 *
 *  @return a ksema_t control structure ready to use
 */
sema_id_t semaphore_create(uint32_t initial, uint32_t limit);


/**
 *  @fn semaphore_delete()
 *  @brief destroys a previous allocated semaphore control block
 *
 *  @param sem - semaphore to be destroyed
 *
 *  @return k_status_ok or error code in case of invalid use
 */
k_status_t semaphore_delete(sema_id_t sem);



/**
 *  @fn semaphore_take()
 *  @brief take a semaphore, if not available blocks the current task
 *
 *  @param s - semaphore to be acquired
 *
 *  @return k_status_ok or error code in case of invalid value
 */
k_status_t semaphore_take(sema_id_t s);


/**
 *  @fn semaphore_give()
 *  @brief release a previously acquired semaphore
 *
 *  @param s- semaphore acquired
 *  @param count - amount of acquisitions to be released (cannot be greater than limit value)
 *
 *  @return k_status_ok or error code in case of invalid value
 */
k_status_t semaphore_give(sema_id_t s, uint32_t count);



#endif
#endif
