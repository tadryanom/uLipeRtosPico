/**
 * 							ULIPE RTOS PICO
 *
 *  @file k_thread.h
 *
 *  @brief threading managament core header file
 *
 *
 */

#ifndef __K_THREAD_H
#define __K_THREAD_H


/* define signals options */
typedef enum {
	k_wait_match_pattern = 0,
	k_wait_match_any,
	k_match_pattern_consume,
	k_match_any_consume,
}thread_signal_opt_t;

/* thread function definition
 *
 * @param arg - custom data provided by application during thread creation
 *
 * @return threads should never return
 *
 */
typedef void (*thread_t) (void *arg);


/** thread id object  to use the thread control functions */
typedef void* tid_t;


/**
 *  @fn thread_create()
 *  @brief obtains a full initialized thread control block ready to be started
 *
 *  @param stack_size_val - size of stack in archtype_t entries (not in bytes!)
 *  @param priority - priority of the thread after created range from 0 to 31
 *
 *  @return a tcb_t control structure ready to use
 */
tid_t thread_create(uint32_t stack_size, uint8_t priority);


/**
 *  @fn thread_delete()
 *  @brief stops thread execution and make it not executable again (only using create) and delete the TCB
 *
 *  @param t - thread to be stopped, NULL or the thread itself is not allowed
 *
 *  @return k_status_ok or error code in case of invalid value
 *
 *  WARNING: AFTER abort a thread its tcb pointer must be cleared or a accidental
 *  		 calling of this function again will result in unexpected behavior!
 *
 */
k_status_t thread_delete(tid_t t);


/**
 *  @fn thread_startd()
 *  @brief begins thread execution when possible
 *
 *  @param func - thread entry point
 *  @param arg - custom argument to pass to the thread after its creation
 *  @param tcb - previously created thread control block NOTE: cannot be NULL!
 *
 *  @return k_status_ok or error code in case of invalid value
 */
k_status_t thread_start(thread_t func, void *arg, tid_t t);



/**
 *  @fn thread_suspend()
 *  @brief suspends thread execution until a thread_resume is invoked
 *
 *  @param t - thread to be suspended, if NULL passed, it suspends the current thread
 *
 *  @return k_status_ok or error code in case of invalid value
 *
 *  NOTE: thread suspend does not nest successive calls, so event if suspend was called
 *        multiple times, the thread will be woken if resume is called once
 *
 */
k_status_t thread_suspend(tid_t t);


/**
 *  @fn thread_resume()
 *  @brief resume a previous suspended thread execution and places it on ready list
 *
 *  @param  t - thread to be resumed
 *
 *  @return k_status_ok or error code in case of invalid value
 *
 */
k_status_t thread_resume(tid_t t);


/**
 *  @fn thread_wait_signals()
 *  @brief suspend a thread exeecution and wait for a single or combination of signals
 *
 *  @param t - thread to wait signals, if NULL is passed suspend the current thread
 *  @param signals - single or a bitmask of signals to be awaited
 *  @param opt - thread will block and woke up if:
 *  				@k_wait_match_pattern - woke only if receive the same signals combined
 *					@Wk_wait_match_any - woke if at least 1 signal is asserted
 *					@k_match_pattern_consume - woke by a combination and consume the signals
 *					@k_match_any_consume - woke at lest 1 signal assertion and consume it
 *
 *  @param err - pointer to a error variable to receive k_status_ok or error code
 *
 *  @return signals asserted if this task was woken
 */
uint32_t thread_wait_signals(tid_t t, uint32_t signals, thread_signal_opt_t opt, k_status_t *err);



/**
 *  @fn thread_set_signals()
 *  @brief set specific signals for a thread
 *
 *  @param t - thread which wait signals
 *  @param signals - signals to set can be  bitmask of combined signals
 *
 *  @return k_status_ok or error code in case of invalid value
 */
k_status_t thread_set_signals(tid_t t, uint32_t signals);

/**
 *  @fn thread_clr_signals()
 *  @brief clear specific signals for a thread
 *
 *  @param t - thread which wait signals or need to consume it
 *  @param signals - signals to clear can be  bitmask of combined signals
 *
 *  @return k_status_ok or error code in case of invalid value
 */
k_status_t thread_clr_signals(tid_t t, uint32_t signals);


/**
 *  @fn thread_yield()
 *  @brief send task to the back of ready list allowing another thread that shares priority level to run

 *  @param none
 *
 *  @return k_status_ok or error code in case of invalid value
 *
 *  NOTE: Different of k_yield() function this task will perform the yielding and if
 *        needed it will perform automatic reescheduling
 *
 */
k_status_t thread_yield(void);


/**
 *  @fn thread_set_prio()
 *  @brief change the priority of a thread in runtime
 *
 *  @param t - thread who wants to change its priority, if NULL changes the priority of current thread
 *  @param prio - new priority value ranged from 0 to 31
 *
 *  @return k_status_ok or error code in case of invalid value
 *
 *  NOTE: After to call this routine the preemption of the kernel will
 *        be triggered, so if the priority of current thread reduces and a new
 *        highest priority is found by scheduler the current thread will be
 *        suspended immediately until this new highest priority task blocks.
 *        In same form if the priority of a ready but not running task arises
 *        in form to be the new highest priority ready to run, the caller
 *        of this function is suspended immediatelly and the changed priority
 *        thread is placed on execution
 */
k_status_t thread_set_prio(tid_t t, uint8_t prio);


/**
 *  @fn thread_get_current()
 *  @brief gets the current running thread id
 *
 *  @param none
 *
 *  @return tcb pointer to the current thread id
 */
tid_t thread_get_current(void);


#endif
