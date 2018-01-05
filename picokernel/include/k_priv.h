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
#define K_PRIORITY_LEVELS	 	 32
#define K_SYS_THREAD_PRIO   	 	 31
#define K_IDLE_THREAD_PRIO		 0xFF


/* define the stask status, not used in user application*/
#define K_THR_SUSPENDED  			(0x01)
#define K_THR_PEND_SEMA  			(0x02)
#define K_THR_PEND_MSG	 			(0x04)
#define K_THR_PEND_TMR	 			(0x08)
#define K_THR_PEND_SIGNAL_ALL 			(0x10)
#define K_THR_PEND_SIGNAL_ANY 			(0x20)
#define K_THR_PEND_SIGNAL_ALL_C 		(0x40)
#define K_THR_PEND_SIGNAL_ANY_C 		(0x80)
#define K_THR_PEND_TICKER	 		(0x100)
#define K_THR_PEND_MTX		 		(0x200)


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


/* work data structure */
typedef struct wqueue_job {
	wqueue_handler_t handler;
	k_list_t node;
}wqueue_job_t;

/* workqueue data structure */
typedef struct wqueue {
	tcb_t *thr;
	k_list_t fifo;
}wqueue_t;


/* defines the structure which controls the memory pool */
typedef struct {
	uint32_t bitmap_h;
	uint32_t bitmap_l[32];
	uint32_t block_size;
	uint16_t numblocks;
	void *mem_pool;
}pool_info_t;

#endif
