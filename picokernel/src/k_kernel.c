/**
 * 							ULIPE RTOS PICO
 *
 *  @file k_kernel.c
 *
 *  @brief multithreading core file
 *
 */

/* idle thread control block */ 
static tid_t idle_thread;

/* current and highest priority tasks obtained from scheduler */
tcb_t *k_current_task;
tcb_t *k_high_prio_task;
bool k_running = false;



static k_work_list_t k_rdy_list;
static k_list_t waiting_to_delete_list;
static k_list_t k_timed_list;

static bool k_configured;
static uint32_t irq_counter;


static k_wakeup_info_t wu_info;
static uint32_t sched_lock_nest=0;


#if((K_ENABLE_TICKER > 0) || (K_ENABLE_TIMERS > 0))
static tid_t timer_tcb;
#endif

/** private functions **/
/**
 *  @fn k_sched()
 *  @brief gets the highest priority and most recet task to run
 *  @param
 *  @return
 */
static tcb_t *k_sched(k_work_list_t *l)
{
	tcb_t *ret = (tcb_t *)idle_thread;
	k_list_t *head;

#if K_DEBUG > 0
	if(k_running) {
		k_current_task->stk_usage = (k_current_task->stack_top - k_current_task->stack_base);
		/* stack monitor used during debug */
		ULIPE_ASSERT( k_current_task->stk_usage * sizeof(archtype_t) <=  k_current_task->stack_size * sizeof(archtype_t));
	}
#endif



	ULIPE_ASSERT(l != NULL);

	archtype_t key = port_irq_lock();
	

	/* no tasks ready, just hint to kernel to load the idle task */
	if(!l->bitmap)
		goto cleanup;

	/*
	 * The scheduling alghoritm uses a classical multilevel
	 * fifo ensuring a O(1) time complexity, the way to obtain the highest priority and first in
	 * task is performing a bit search on priority bitmap, since the
	 * priority levels is 4 + some sys priority only a simple
	 * lead zero table perform this action, with the prio obtained we
	 * get the head of the list indexed to this prio and
	 * finally access the tcb
	 */
	uint8_t prio = (K_PRIORITY_LEVELS - 1) - port_bit_fs_scan(l->bitmap);
	
	head = sys_dlist_peek_head(&l->list_head[prio]);
	if(head != NULL)
		ret = CONTAINER_OF(head, tcb_t, thr_link);

	port_irq_unlock(key);

cleanup:
	return(ret);
}



/** public functions **/
static k_status_t k_pend_obj(tcb_t *thr, k_work_list_t *obj_list)
{
	k_status_t err = k_status_ok;
	ULIPE_ASSERT(thr != NULL);
	ULIPE_ASSERT(obj_list != NULL);

#if K_DEBUG > 0
	if(k_running) {
		k_current_task->stk_usage = (k_current_task->stack_top - k_current_task->stack_base);
		/* stack monitor used during debug */
		ULIPE_ASSERT( k_current_task->stk_usage * sizeof(archtype_t) <=  k_current_task->stack_size * sizeof(archtype_t));
	}
#endif



	/*
	 * The insertion on obj_list consider that tasks are using priority ceilling
	 * or system threads, which priorities are < 0 , so we need to handle
	 * both cases, for negative priority we remove the signal and use its upper
	 * 3 bits to insert in system level priorities list
	 */
	archtype_t key = port_irq_lock();

	obj_list->bitmap |= (1 << thr->thread_prio);
	sys_dlist_append(&obj_list->list_head[thr->thread_prio], &thr->thr_link);

	port_irq_unlock(key);

	return(err);

}

static tcb_t * k_unpend_obj(k_work_list_t *obj_list)
{
	tcb_t *thr = k_status_ok;
	ULIPE_ASSERT(obj_list != NULL);

#if K_DEBUG > 0
	if(k_running) {
		k_current_task->stk_usage = (k_current_task->stack_top - k_current_task->stack_base);
		/* stack monitor used during debug */
		ULIPE_ASSERT( k_current_task->stk_usage * sizeof(archtype_t) <=  k_current_task->stack_size * sizeof(archtype_t));
	}
#endif

	/* before to unpend, we need to scan the pendable list
	 * to find possible lists with bitmap set but with no entries,
	 * this is a specific case and only occurs when the user
	 * deletes a thread, the entry is removed in delete function
	 * calling, but the bitmap of pendable list cannot be changed
	 * so to avoid the use of a couple of pointers tracking objects
	 * we add this exception of the real time execution rule
	 */

	archtype_t key = port_irq_lock();

	uint32_t bit_finder = 0x00000001;

	for ( uint8_t i = 0; i < K_PRIORITY_LEVELS; i++) {
		if(bit_finder & obj_list->bitmap) {
			if (sys_dlist_is_empty(&obj_list->list_head[(K_PRIORITY_LEVELS - 1) + i]))
				obj_list->bitmap &= ~(1 <<  i);
		}
	}

	port_irq_unlock(key);

	/*
	 * unpend a obj is a little bit complex relative to a
	 * make not ready, before to remove a item from a work list
	 * we need to know which object is the desired, to keep the
	 * kernel execution policy, we execute the fifo-sched on
	 * work list to obtain which is the highest priority task
	 * waiting for a kernel object and remove it from list
	 */
	thr = k_sched(obj_list);

	if(thr == (tcb_t *)idle_thread) {
		thr = NULL;
		goto cleanup;
	}

	key = port_irq_lock();

	sys_dlist_remove(&thr->thr_link);
	if(sys_dlist_is_empty(&obj_list->list_head[thr->thread_prio])){
		obj_list->bitmap &= ~(1 << thr->thread_prio);
	}

	port_irq_unlock(key);

cleanup:
	return(thr);
}


static k_status_t k_make_ready(tcb_t *thr)
{
	k_status_t err = k_status_ok;
	ULIPE_ASSERT(thr != NULL);

#if K_DEBUG > 0
	if(k_running) {
		k_current_task->stk_usage = (k_current_task->stack_top - k_current_task->stack_base);
		/* stack monitor used during debug */
		ULIPE_ASSERT( k_current_task->stk_usage * sizeof(archtype_t) <=  k_current_task->stack_size * sizeof(archtype_t));
	}
#endif

	/* to make ready evaluate if task does not waits for
	 * any more objects
	 */
	if((thr->thread_wait != 0))
		goto cleanup;

	/*
	 * The insertion on ready list consider that tasks are using priority ceilling
	 * or system threads, which priorities are < 0 , so we need to handle
	 * both cases, for negative priority we remove the signal and use its upper
	 * 3 bits to insert in system level priorities list
	 */

	archtype_t key = port_irq_lock(); 

	k_rdy_list.bitmap |= (1 << thr->thread_prio);
	sys_dlist_append(&k_rdy_list.list_head[thr->thread_prio], &thr->thr_link);

	port_irq_unlock(key);

cleanup:
	return(err);
}

static k_status_t k_make_not_ready(tcb_t *thr)
{
	k_status_t err = k_status_ok;
	ULIPE_ASSERT(thr != NULL);

	/*
	 * for remotion we need care both priority cases as well,
	 * and before clear a priorty level bitmap, we need to make sure
	 * there is no further tasks which share the priority of thr
	 *
	 */

#if K_DEBUG > 0
	if(k_running) {
		k_current_task->stk_usage = (k_current_task->stack_top - k_current_task->stack_base);
		/* stack monitor used during debug */
		ULIPE_ASSERT( k_current_task->stk_usage * sizeof(archtype_t) <=  k_current_task->stack_size * sizeof(archtype_t));
	}
#endif


	archtype_t key = port_irq_lock();

	sys_dlist_remove(&thr->thr_link);
	if(sys_dlist_is_empty(&k_rdy_list.list_head[thr->thread_prio])) {
		k_rdy_list.bitmap &= ~(1 << thr->thread_prio);
	}

	port_irq_unlock(key);

	return(err);

}

static bool k_yield(tcb_t *t)
{
	k_status_t err = k_status_ok;
	ULIPE_ASSERT(t != NULL);

#if K_DEBUG > 0
	if(k_running) {
		k_current_task->stk_usage = (k_current_task->stack_top - k_current_task->stack_base);
		/* stack monitor used during debug */
		ULIPE_ASSERT( k_current_task->stk_usage * sizeof(archtype_t) <=  k_current_task->stack_size * sizeof(archtype_t));
	}
#endif

	bool resched = false;


	err = k_make_not_ready(t);
	ULIPE_ASSERT(err == k_status_ok);


	err = k_make_ready(t);
	ULIPE_ASSERT(err == k_status_ok);

	/* check if anything changed durring send to back
	 * operation
	 */
	if(k_sched(&k_rdy_list) != t) {
		resched = true;
	}

	return(resched);
}

static k_status_t k_sched_and_swap(void)
{
	k_status_t ret = k_status_ok;

	if(!k_running){
		ret = k_status_error;
		goto cleanup;
	}

	/*
	 * the sched and swap has the possibility to perform a
	 * context switch, so we need ensure the if in a ISR
	 * that all ISRs was processed and scheduling is the
	 * last ISR to process
	 */

	if(sched_lock_nest > 0) {
		goto cleanup;		
	}

	if(irq_counter > 0){
		goto cleanup;
	}


	k_high_prio_task = k_sched(&k_rdy_list);
	if(k_high_prio_task == NULL) {
		/* no other tasks ready to run, puts the idle thread */
		k_high_prio_task = (tcb_t *)idle_thread;
	}


#if K_DEBUG > 0
	if(k_running) {
		k_current_task->stk_usage = (k_current_task->stack_top - k_current_task->stack_base);
		/* stack monitor used during debug */
		ULIPE_ASSERT( k_current_task->stk_usage * sizeof(archtype_t) <=  k_current_task->stack_size * sizeof(archtype_t));
	}
#endif



	if(k_high_prio_task != k_current_task) {
		/* new high priority task, perform a swap request */
		port_swap_req();
	}

cleanup:
	return(ret);
}



static void k_work_list_init(k_work_list_t *l)
{

#if K_DEBUG > 0
	if(k_running) {
		k_current_task->stk_usage = (k_current_task->stack_top - k_current_task->stack_base);
		/* stack monitor used during debug */
		ULIPE_ASSERT( k_current_task->stk_usage * sizeof(archtype_t) <=  k_current_task->stack_size * sizeof(archtype_t));
	}
#endif

	archtype_t key = port_irq_lock();

	l->bitmap = 0;
	for(uint8_t i=0; i < K_PRIORITY_LEVELS ; i++)
		sys_dlist_init(&l->list_head[i]);

	port_irq_unlock(key);	
}

static void k_sched_lock(void)
{
	archtype_t key = port_irq_lock();
	if(sched_lock_nest < 0xFFFFFFFF)
		sched_lock_nest++;
	port_irq_unlock(key);
}


static void k_sched_unlock(void)
{
	archtype_t key = port_irq_lock();
	if(sched_lock_nest < 0xFFFFFFFF)
		sched_lock_nest--;
	port_irq_unlock(key);
}


k_status_t kernel_init(void)
{
	archtype_t key = port_irq_lock();

	/* kernel memory always needs to set first */
	k_heap_init();


	/* no priority ready and no tasks waiting*/
	k_work_list_init(&k_rdy_list);

	/* init timed list */
	sys_dlist_init(&k_timed_list);

	/* irq counter zeroed, default state */
	irq_counter = 0;

	/* current and hpt holding no task */
	k_current_task = NULL;
	k_high_prio_task = NULL;

	port_irq_unlock(key);


	/* creates the idle thread */
	idle_thread = thread_create(64, 0);
	ULIPE_ASSERT(idle_thread != NULL);
	k_status_t err = thread_start(&k_idle_thread, &wu_info, idle_thread);
	ULIPE_ASSERT(err == k_status_ok);

	k_make_not_ready((tcb_t *)idle_thread);
	((tcb_t *)(idle_thread))->thread_prio = K_IDLE_THREAD_PRIO;

#if(K_ENABLE_TICKER > 0)
	timer_tcb = thread_create(K_TIMER_DISPATCHER_STACK_SIZE,K_TIMER_DISPATCHER_PRIORITY);
	ULIPE_ASSERT(timer_tcb != NULL);
	err = thread_start(&timer_dispatcher, NULL, timer_tcb);
	ULIPE_ASSERT(err == k_status_ok);
#endif

	/* os configured and ready to be started */
	k_configured = true;

	return(k_status_ok);
}

k_status_t kernel_start(void)
{
	if(!k_configured)
		/* kernel must be configured before start */
		goto cleanup;

	/* init architecture stuff */
	port_init_machine();

	/* gets the first task to run */
	k_high_prio_task = k_sched(&k_rdy_list);

	ULIPE_ASSERT(k_high_prio_task != NULL);

	/* start kernel (this function will never return) */
	port_start_kernel();

cleanup:
	return(k_status_error);
}


void kernel_irq_in(void)
{

#if K_DEBUG > 0
	if(k_running) {
		k_current_task->stk_usage = (k_current_task->stack_top - k_current_task->stack_base);
		/* stack monitor used during debug */
		ULIPE_ASSERT( k_current_task->stk_usage * sizeof(archtype_t) <=  k_current_task->stack_size * sizeof(archtype_t));
	}
#endif

	/* this function only can be called if os is executing  and from an isr*/
	if(!k_running)
		return;

	if(!port_from_isr())
		return;

	if(irq_counter < (archtype_t)0xFFFFFFFF)
		irq_counter++;
}

void kernel_irq_out(void)
{

#if K_DEBUG > 0
	if(k_running) {
		k_current_task->stk_usage = (k_current_task->stack_top - k_current_task->stack_base);
		/* stack monitor used during debug */
		ULIPE_ASSERT( k_current_task->stk_usage * sizeof(archtype_t) <=  k_current_task->stack_size * sizeof(archtype_t));
	}
#endif

	/* this function only can be called if os is executing  and from an isr*/
	if(!k_running)
		return;

	if(!port_from_isr())
		return;


	if(irq_counter > (archtype_t)0) {
		irq_counter--;
		/* all isrs attended, perform a system reeschedule if it is unlocked*/
		if(!irq_counter)
			k_sched_and_swap();
	}

}
