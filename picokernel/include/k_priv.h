/**
 * 							ULIPE RTOS PICO
 *
 *  @file k_priv.h
 *
 *  @brief Internal data structures and private kernel functions 
 *
 *
 */

#ifndef __K_PRIV_H
#define __K_PRIV_H


/* kernel always will have this priority level */
#define K_PRIORITY_LEVELS	 	 	32
#define K_SYS_THREAD_PRIO   	 	31
#define K_IDLE_THREAD_PRIO		 	0xFF


/* define the stask status, not used in user application*/
#define K_THR_SUSPENDED  				(0x01)
#define K_THR_PEND_SEMA  				(0x02)
#define K_THR_PEND_MSG	 				(0x04)
#define K_THR_PEND_TMR	 				(0x08)
#define K_THR_PEND_SIGNAL_ALL 			(0x10)
#define K_THR_PEND_SIGNAL_ANY 			(0x20)
#define K_THR_PEND_SIGNAL_ALL_C 		(0x40)
#define K_THR_PEND_SIGNAL_ANY_C 		(0x80)
#define K_THR_PEND_TICKER	 			(0x100)
#define K_THR_PEND_MTX		 			(0x200)


/* timer commands, not used for user application  */
#define K_TIMER_LOAD_FRESH		0x01
#define K_TIMER_DISPATCH 		0x02
#define K_TIMER_REFRESH			0x04
#define K_TIMER_TICK			0x08



/* define kernel system/pendable list */
typedef struct k_work_list{
	/* contains the bits sets corresponding to populated lists*/
	uint32_t bitmap;

	/* simple multilevel list which receives tcb in fifo form */
	k_list_t list_head[K_PRIORITY_LEVELS];

}k_work_list_t;



/* thread control block data structure */
typedef struct ktcb{
	archtype_t *stack_top;
	archtype_t *stack_base;
	archtype_t stk_usage;
	uint16_t thread_wait;
	uint8_t thread_prio;
	bool created;
	uint32_t stack_size;
	uint32_t wake_tick;
	archtype_t signals_wait;
	archtype_t signals_actual;
	archtype_t timer_wait;
	k_list_t thr_link;

}tcb_t;


/** define mutex celing priority value */
#define K_MUTEX_PRIO_CEIL_VAL	(K_PRIORITY_LEVELS - 8)


/* semaphore control block structure */
typedef struct ksema{
	archtype_t cnt;
	archtype_t limit;
	bool created;
	k_work_list_t threads_pending;
}ksema_t;


/* semaphore control block structure */
typedef struct kmutex{
	bool created;
	uint8_t owner_prio;
	tcb_t *thr_owner;
	k_work_list_t threads_pending;
}kmutex_t;



/* timer control block structure */
typedef struct ktimer{
	uint32_t load_val;
	uint32_t timer_to_wait;
	ktimer_callback_t cb;
	void *user_data;
	bool expired;
	bool created;
	bool running;
	k_work_list_t threads_pending;
	k_list_t timer_list_link;
}ktimer_t;


/* kernel execution information */
typedef struct k_wakeup_info {
	tcb_t *next_thread_wake;
	ktimer_t *next_timer;
	uint32_t *tick_cntr;
}k_wakeup_info_t;


/* message control block structure */
typedef struct kmsg{
	uint8_t *data;
	archtype_t items;
	archtype_t slots_number;
	archtype_t wr_ptr;
	archtype_t rd_ptr;
	archtype_t slot_size;
	bool created;
	k_work_list_t rd_threads_pending;
	k_work_list_t wr_threads_pending;
}kmsg_t;



/* forward declaration to internal functions used by kernel */
#if((K_ENABLE_TICKER > 0) || (K_ENABLE_TIMERS > 0))
static void timer_dispatcher(void *args);
#endif

/** forward declaration of some external functions needed by os core */
static void k_idle_thread(void *args);
static void k_heap_init(void);



/**
 *  @fn k_pend_obj()
 *  @brief pend a task for a particular objects inserting in its list
 *
 *  @param thr - thread to pend
 *  @param obj_list - kernel object wait list
 *
 *  @return k_status_ok or error
 */
static k_status_t k_pend_obj(tcb_t *thr, k_work_list_t *obj_list);

/**
 *  @fn k_unpend_obj()
 *  @brief remove the highest priority task from a pendable object list
 *
 *  @param obj_list - kernel object wait list which contains threads
 *
 *  @return tcb with highest priority task or NULL if is empty
 */
static tcb_t *k_unpend_obj(k_work_list_t *obj_list);

/**
 *  @fn k_make_ready()
 *  @brief makes a particular tcb ready inserting into ready list task
 *
 *  @param thr - tcb containing the thread who wants to become ready
 *
 *  @return k_status_ok or error in case of invalid value
 */

static k_status_t k_make_ready(tcb_t *thr);

/**
 *  @fn k_make_not_ready()
 *  @brief remove a particular tcb from ready list
 *
 *  @param thr - tcb containing the thread who wants to remove
 *
 *  @return k_status_ok or error in case of invalid value
 */
static k_status_t k_make_not_ready(tcb_t *thr);


/**
 *  @fn k_yield()
 *  @brief send the specified tcb to the back of its current FIFO
 *
 *  @param t - tcb containing the thread to perform yielding
 *
 *  @return true if reeschedule is needed
 *
 *  NOTE: this task does not perform reescheduling automatically!!
 */
static bool k_yield(tcb_t *t);


/**
 *  @fn k_sched_and_swap()
 *  @brief schedule the task set and swap to the next ready to run
 *
 *  @param none
 *
 *  @return k_status_ok if scheduler is perfomred or error if scheduler is locked
 *
 */
static k_status_t k_sched_and_swap(void);



/**
 *  @fn k_sched_lock()
 *  @brief locks the scheduler, no new task will put in execution until unlocked not be called
 *
 *  @param none
 *
 *  @return none
 *
 */
static void k_sched_lock(void);


/**
 *  @fn k_sched_unlock()
 *  @brief unlocks the scheduling allowing the next task to be selected
 *
 *  @param none
 *
 *  @return none
 *
 */
static void k_sched_unlock(void);


#endif
