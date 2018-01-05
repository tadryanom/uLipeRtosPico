/**
 * 							ULIPE RTOS PICO
 *
 *  @file k_wqueue.h
 *
 *  @brief workqueues job deferrable server  interface
 *
 *
 */

#ifndef __K_WQUEUE_H
#define __K_WQUEUE_H

#if(K_ENABLE_WORKQUEUES > 0)

/**
 *  workqueue deferred job callback
 *
 *  @param work - job data deferred by current workqueue instance
 *  @return none
 *
 */
typedef void (*wqueue_handler_t) (void* work);


/**
 *  @fn wqueue_init()
 *  @brief initializes and starts the created workqueue deferrable server
 *
 *  @param wq - workqueue instance user wants to start, previously created
 *
 *  @return k_status_ok or error code in case of invalid value
 */
k_status_t wqueue_init(wqueue_t *wq);



/**
 *  @fn wqueue_submit()
 *  @brief submits a work to be processed by workqueue
 *
 *  @param wq - workqueue to process the job
 *  @param work - work to be handlered by workqueue it can contain custom user data
 *
 *  @return k_status_ok or error code in case of invalid value
 */
k_status_t wqueue_submit(wqueue_t *wq, wqueue_job_t *work);


#endif
#endif /* INCLUDE_PICOKERNEL_WQUEUE_H_ */
