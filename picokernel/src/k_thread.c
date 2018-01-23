/**
 * 							ULIPE RTOS PICO
 *
 *  @file k_thread.c
 *
 *  @brief threading managament core file
 *
 *
 */




/** static functions */
/**
 *  @fn k_idle_thread()
 *  @brief idle thread to be executed when no other thread are ready to run
 *  @param
 *  @return
 */
static void k_idle_thread(void *kernel_info)
{

	for(;;) {
#if (K_ENABLE_TICKLESS_IDLE > 0)
		/* kernel can sleep ? */
		if((wu_info.next_thread_wake == NULL) && (wu_info.next_timer == NULL)) {
			/* simplest case, we need only to enter in sleep for user defined time
			 * the tasks sleep for defined time is future implementation
			 */
			port_low_power_engine(&wu_info);
		} else if ((wu_info.next_thread_wake == NULL) && (wu_info.next_timer != NULL)) {

			/* we have a next timer to expire, time to sleep? */
			if((wu_info.next_timer->load_val - *wu_info.tick_cntr) > K_MAX_LOW_POWER_PERIOD) {
				port_low_power_engine(&wu_info);
			}
		} else if ((wu_info.next_thread_wake != NULL) && (wu_info.next_timer == NULL)) {

			/* we have a next timer to expire, time to sleep? */
			if((wu_info.next_thread_wake->wake_tick - *wu_info.tick_cntr) > K_MAX_LOW_POWER_PERIOD) {
				port_low_power_engine(&wu_info);
			}

		} 	else if((wu_info.next_thread_wake != NULL) && (wu_info.next_timer != NULL)) {

			uint32_t thread_tick_step = wu_info.next_thread_wake->wake_tick - *wu_info.tick_cntr;
			uint32_t timer_tick_step = wu_info.next_timer->load_val - *wu_info.tick_cntr;

			/* both timers needs to allow us to enter in sleep mode */
			if((thread_tick_step >= K_MAX_LOW_POWER_PERIOD) && (timer_tick_step >= K_MAX_LOW_POWER_PERIOD) ) {
				port_low_power_engine(&wu_info);
			}
		}

#else
		(void)kernel_info;
#endif
	}
}



/**
 *  @fn thread_handle_signal_act()
 *  @brief wakes a waiting task which meet the signals conditions
 *  @param
 *  @return
 */
static bool thread_handle_signal_act(tcb_t *t)
{
	bool match = false;


	if(t->thread_wait & (K_THR_PEND_SIGNAL_ANY |  K_THR_PEND_SIGNAL_ANY_C)){
		/* just a signal that matches with this is sufficient to wake task */
		if((t->signals_actual) & (t->signals_wait))
			match = true;
	}

	if(t->thread_wait & (K_THR_PEND_SIGNAL_ALL |  K_THR_PEND_SIGNAL_ALL_C)){
		/* here the mask must be the same */
		if((t->signals_actual) == (t->signals_wait))
			match = true;
	}


	if(match == true){
		if(t->thread_wait & (K_THR_PEND_SIGNAL_ALL_C | K_THR_PEND_SIGNAL_ANY_C ))
			t->signals_actual = 0;
		/* on a match, clear the signal waiting flags */
		t->signals_wait = 0;
		t->thread_wait &= ~(K_THR_PEND_SIGNAL_ANY |  K_THR_PEND_SIGNAL_ALL|
				K_THR_PEND_SIGNAL_ALL_C | K_THR_PEND_SIGNAL_ANY_C);
	}

	return(match);
}


/**
 *  @fn thread_handle_signal_wait()
 *  @brief put task in wait state that matches user options about signals to wait
 *  @param
 *  @return
 */
static bool thread_handle_signal_wait(tcb_t *t, uint32_t wait_type, archtype_t signals)
{
	bool match = false;

	t->thread_wait |= wait_type;
	t->signals_wait = signals;

	/* evaluate if signals is aready asserted */
	match = thread_handle_signal_act(t);

	return(match);

}



/** public fnctions */
tid_t thread_create(uint32_t stack_size, uint8_t priority)
{
	tcb_t *ret = (tcb_t *)k_malloc(sizeof(tcb_t));
	if(ret == NULL)
		goto cleanup;

	archtype_t *stk = (archtype_t *)k_malloc(K_MINIMAL_STACK_VAL + (stack_size * sizeof(archtype_t)));
	if(stk == NULL) {
		k_free(ret);
		ret = NULL;
		goto cleanup;
	}

	/* populate tcb with default values */
	ret->stack_base=stk;
	ret->stack_size=K_MINIMAL_STACK_VAL+stack_size;
	ret->thread_prio = priority;
	ret->thread_wait = 0;
	ret->wake_tick = 0;
	ret->created = false;

cleanup:
	return ((tid_t)ret);
}



k_status_t thread_start(thread_t func, void *arg,tid_t tcb)
{
	k_status_t ret = k_status_ok;

	/* thread create must not be called from isr,
	 * thread func must be valid and tcb as well
	 */
	if(port_from_isr()){
		ret = k_status_illegal_from_isr;
		goto cleanup;
	}

	if(func == NULL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	if(tcb == NULL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	tcb_t *t = (tcb_t *)tcb;


	if(k_current_task == t) {
		/* cannot re-create the current running task */
		ret = k_status_invalid_param;
		goto cleanup;
	}

	/*
	 * The contents of tcb must represent a initialized one
	 * allocated with THREAD_CONTROL_BLOCK_DECLARE() otherwise
	 * the thread will not created
	 */
	if(t->thread_prio > (K_PRIORITY_LEVELS - 1)){
		ret = k_status_invalid_param;
		goto cleanup;
	}


	if(t->stack_size < K_MINIMAL_STACK_VAL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	if(t->stack_base == NULL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	/* disable scheduling durint task creation */
	k_sched_lock();

	t->signals_wait = 0;
	t->thread_wait = 0;
	t->timer_wait = 0;
	t->created = true;
	sys_dlist_init(&t->thr_link);

	/* initialize stack contents */
	t->stack_top = port_create_stack_frame(t->stack_base + (archtype_t)t->stack_size, func, arg);
	ULIPE_ASSERT(t->stack_top != NULL);
	t->stk_usage = t->stack_top - t->stack_base;


	/* insert the created thread on ready list */
	ret=k_make_ready(t);
	ULIPE_ASSERT(ret == k_status_ok);


	/* allow kernel to run, perform reeschedule */
	k_sched_unlock();
	k_sched_and_swap();
	ret = k_status_ok;

cleanup:
	return(ret);
}


k_status_t thread_delete(tid_t t)
{
	k_status_t ret = k_status_ok;

	/* disable scheduling during task abortion */
	k_sched_lock();

	tcb_t *tcb = (tcb_t *)t;

	if(tcb == NULL) {
		/* null thread can be the current */
		tcb = k_current_task;
		ULIPE_ASSERT(t!= NULL);
	}


	/* this is a ready task so, remove it from ready list */
	ret=k_make_not_ready(tcb);
	ULIPE_ASSERT(ret == k_status_ok);

	/* de init the thread  but keeps some parameters
	 * allowing user to re-create it if necessary
	 */
	tcb->created = false;

	/* perform reesched to find next task to run */
	k_sched_unlock();
	k_sched_and_swap();
	ret = k_status_ok;

	return(ret);
}

k_status_t thread_suspend(tid_t t)
{
	k_status_t ret = k_status_ok;

	if(port_from_isr()){
		/* suspend cannot be called from ISR */
		ret = k_status_illegal_from_isr;
		goto cleanup;
	}


	tcb_t *tcb = (tcb_t *)t;


	k_sched_lock();

	if(tcb == NULL) {
		/* null thread can be the current */
		tcb = k_current_task;
		ULIPE_ASSERT(t!= NULL);
	}



	if(tcb->thread_wait & K_THR_SUSPENDED) {
		/* thread is already suspended */
		k_sched_unlock();
		ret = k_thread_susp;
		goto cleanup;
	}


	/* to suspend a thread, only remove it from ready list and
	 * mark with suspended
	 */
	ret=k_make_not_ready(tcb);
	ULIPE_ASSERT(ret == k_status_ok);

	tcb->thread_wait |= K_THR_SUSPENDED;


	/* perform reesched to find next task to run */
	k_sched_unlock();
	k_sched_and_swap();
	ret = k_status_ok;


cleanup:
	return(ret);
}

k_status_t thread_resume(tid_t t)
{
	k_status_t ret = k_status_ok;

	if(t == NULL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	tcb_t *tcb = (tcb_t *)t;


	if((tcb->thread_wait & K_THR_SUSPENDED) == 0) {
		/* thread is not suspended */
		ret = k_status_invalid_param;
		goto cleanup;
	}

	k_sched_lock();

	tcb->thread_wait &= ~(K_THR_SUSPENDED);

	ret=k_make_ready(tcb);
	ULIPE_ASSERT(ret == k_status_ok);

	/* perform reesched to find next task to run */
	k_sched_unlock();
	k_sched_and_swap();
	ret = k_status_ok;

cleanup:
	return(ret);
}

uint32_t thread_wait_signals(tid_t t, uint32_t signals, thread_signal_opt_t opt, k_status_t *err)
{
	uint32_t rcvd = 0xFFFFFFFF;
	bool match = false;
	k_status_t ret;


	tcb_t *tcb = (tcb_t *)t;

	if(tcb == NULL) {
		/* null thread can be the current */
		tcb = k_current_task;
		ULIPE_ASSERT(tcb!= NULL);
	}



	if(port_from_isr()){
		/* wait cannot be called from ISR */
		ret = k_status_illegal_from_isr;
		goto cleanup;
	}

	k_sched_lock();

	/*
	 * Select and prepare task to wait for the conditions imposed to signaling
	 */
	switch(opt) {
		case k_wait_match_pattern:
			match = thread_handle_signal_wait(tcb, K_THR_PEND_SIGNAL_ALL, signals);
		break;

		case k_wait_match_any:
			match = thread_handle_signal_wait(tcb, K_THR_PEND_SIGNAL_ANY, signals);
		break;

		case k_match_pattern_consume:
			match = thread_handle_signal_wait(tcb, K_THR_PEND_SIGNAL_ALL_C, signals);
		break;

		case k_match_any_consume:
			match = thread_handle_signal_wait(tcb, K_THR_PEND_SIGNAL_ANY_C, signals);
		break;

		default:
			ret = k_status_invalid_param;
			k_sched_unlock();
			goto cleanup;
	}



	/* if signals ware already asserted theres no need to reesched */
	if(match) {
		rcvd = tcb->signals_actual;
		k_sched_unlock();
		goto cleanup;
	}

	ret = k_make_not_ready(tcb);
	ULIPE_ASSERT(ret == k_status_ok);


	rcvd = tcb->signals_actual;	
	
	/* perform reesched to find next task to run */
	k_sched_unlock();	
	k_sched_and_swap();
	ret = k_status_ok;


cleanup:
	if(err != NULL)
		*err = ret;

	return(rcvd);
}

k_status_t thread_set_signals(tid_t t, uint32_t signals)
{
	k_status_t ret = k_status_ok;
	bool match =false;

	tcb_t *tcb = (tcb_t *)t;

	if(tcb == NULL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	k_sched_lock();

	tcb->signals_actual |= signals;
	match = thread_handle_signal_act(tcb);


	if(!match) {
		k_sched_unlock();
		goto cleanup;
	}


	ret = k_make_ready(tcb);
	ULIPE_ASSERT(ret == k_status_ok);
	

	/* perform reesched to find next task to run */
	k_sched_unlock();	
	k_sched_and_swap();
	ret = k_status_ok;

cleanup:
	return(ret);
}

k_status_t thread_clr_signals(tid_t t, uint32_t signals)
{

	k_status_t ret = k_status_ok;
	bool match =false;

	tcb_t *tcb = (tcb_t *)t;


	if(tcb == NULL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	k_sched_lock();

	tcb->signals_actual &= ~signals;
	match = thread_handle_signal_act(tcb);



	if(!match) {
		k_sched_unlock();
		goto cleanup;
	}

	ret = k_make_ready(tcb);
	ULIPE_ASSERT(ret == k_status_ok);
	

	/* perform reesched to find next task to run */
	k_sched_unlock();	
	k_sched_and_swap();
	ret = k_status_ok;

cleanup:
	return(ret);
}


k_status_t thread_yield(void)
{
	k_status_t ret = k_status_ok;
	bool resched = false;

	k_sched_lock();
	tcb_t *t = k_current_task;


	/*
	 * Yielding a task is very simple, we force a fifo remotion, then
	 * pick the removed entry and send back to the fifo, the kernel
	 * scheduling deal with case that only thread as the top priority and try to yield
	 * itself
	 */
	resched = k_yield(t);

	if(!resched) {
		k_sched_unlock();
		goto cleanup;
	}

	k_sched_unlock();
	k_sched_and_swap();

cleanup:
	return(ret);
}


k_status_t thread_set_prio(tid_t t, uint8_t prio)
{
	k_status_t ret = k_status_ok;
	tcb_t *tcb = (tcb_t *)t;

	if(prio > (K_PRIORITY_LEVELS - 1)){
		ret = k_status_invalid_param;
		goto cleanup;
	}

	k_sched_lock();


	if(tcb == NULL) {
		/* null thread can be the current */
		tcb = k_current_task;
		ULIPE_ASSERT(t!= NULL);
	}



	if(tcb->thread_prio == prio) {
		/* same prio, no need to process it */
		ret = k_status_invalid_param;
		k_sched_unlock();
		goto cleanup;
	}




	/* the priority change impacts on moving the task to another level
	 * on ready fifo
	 */
	if(!tcb->thread_wait) {
		ret = k_make_not_ready(tcb);
		ULIPE_ASSERT(ret == k_status_ok);
	}

	tcb->thread_prio = prio;


	if(tcb->thread_wait) {
		k_sched_unlock();
		goto cleanup;
	}

	ret = k_make_ready(tcb);
	ULIPE_ASSERT(ret == k_status_ok);

	k_sched_unlock();	
	k_sched_and_swap();
	ret = k_status_ok;

cleanup:
	return(ret);
}

tid_t thread_get_current(void)
{
	return((tid_t)k_current_task);
}








