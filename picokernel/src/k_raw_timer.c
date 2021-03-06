/**
 * 							ULIPE RTOS PICO
 *  @file k_raw_timer.h
 *
 *  @brief timer primitive kernel header file
 *
 */


#if (K_ENABLE_TICKER > 0)

#define K_TIMER_NO_WAKEUP_TASK (archtype_t)0xFFFFFFFF

#if(K_ENABLE_TIMERS > 0)
/* static variables */
static k_list_t k_timed_list = SYS_DLIST_STATIC_INIT(&k_timed_list);

#ifndef K_ENABLE_TIMER_GENERIC_SUPPORT
static archtype_t k_elapsed_time = 0;
static bool no_timers = true;
#else
static ktimer_t *actual_timer;
#endif

#endif

static uint32_t tick_count = 0;
static tcb_t * next_task_wake = NULL;
static k_list_t k_ticker_list = SYS_DLIST_STATIC_INIT(&k_ticker_list);





/** private functions */
/**
 *  @fn timer_period_sort()
 *  @brief	sort all active timers on list to find which has the least time to wait
 *  @param
 *  @return
 */
static void timer_step_tick(void) 
{
	tcb_t *ret = NULL;


	tick_count++;
	/* no timer to run */
	if(!sys_dlist_is_empty(&k_ticker_list)) {

		/* walk to list and check if task needs to be woken */
		SYS_DLIST_FOR_EACH_CONTAINER(&k_ticker_list, ret, thr_link) {
			if(next_task_wake == NULL) {
				next_task_wake = ret;
			}

			if((next_task_wake->wake_tick > ret->wake_tick) && (ret->wake_tick != 0)) {
				next_task_wake = ret;
			}
		}
	}

#if (K_ENABLE_TIMER_GENERIC_SUPPORT > 0)
	ktimer_t *tim = NULL;

	if(!sys_dlist_is_empty(&k_timed_list)) {

		SYS_DLIST_FOR_EACH_CONTAINER(&k_timed_list, tim, timer_list_link) {
			if(actual_timer == NULL) {
				actual_timer = tim;
			}

			if((actual_timer->load_val > tim->load_val) && (tim->load_val != 0)) {
				actual_timer = tim;
			}
		}
	}

#endif

	return;
}

/**
 *  @fn timer_period_sort()
 *  @brief	sort all active timers on list to find which has the least time to wait
 *  @param
 *  @return
 */

#if(K_ENABLE_TIMERS > 0)

#ifndef K_ENABLE_TIMER_GENERIC_SUPPORT
static ktimer_t *timer_period_sort(k_list_t *tlist)
{
	ktimer_t *ret = NULL;
	ktimer_t *tmp = NULL;
	k_list_t *head;

	if(sys_dlist_is_empty(tlist))
		goto cleanup;

	/*
	 * The objective here is to find the 
	 * timer which loads the less valued
	 * load to do it iterate list and compare each
	 * time against with reference, pick and keeps
	 * ret with this timer until interation ends
	 * or another load value fits in this rule
	 */
	head = sys_dlist_peek_head(tlist);
	ULIPE_ASSERT(head != NULL);	
	ret = CONTAINER_OF(head, ktimer_t, timer_list_link);
	ULIPE_ASSERT(ret != NULL);	

	SYS_DLIST_FOR_EACH_CONTAINER(tlist, tmp, timer_list_link) {
		if(ret->load_val > tmp->load_val) {
			ret = tmp;
		}
	}

cleanup:
	return(ret);
}

/**
 *  @fn timer_rebuild_timeline()
 *  @brief	rebuild the timer dispatch scheduling 
 *  @param
 *  @return
 */
static void timer_rebuild_timeline(ktimer_t *t, archtype_t *key)
{
	ULIPE_ASSERT(t != NULL);
	ULIPE_ASSERT(key != NULL);
	archtype_t cmd;
	(void)key;


	k_sched_lock();
	/* put the new timer on timeline list */
	sys_dlist_append(&k_timed_list, &t->timer_list_link);
	k_sched_unlock();

	/* check if timer is not running */
	if(no_timers) {
		cmd = K_TIMER_LOAD_FRESH;

		t->load_val = t->timer_to_wait;
		t->running  = true;
		t->expired  = false;


		
	} else {
		cmd = K_TIMER_REFRESH;
		/* memorize the point of timeline when timer was added, we
		 * will use it to calculate the amount of counts
		 * to be loaded on timer IP when this timer 
		 * will be selected
		 */
			uint32_t tim = port_timer_halt();


			t->load_val = tim + t->timer_to_wait;
			t->running  = true;
			t->expired  = false;

	}

	thread_set_signals(&timer_tcb,cmd);
}
#endif
#endif


/**
 *  @fn timer_tick_handler()
 *  @brief	evaluate if timer task needs to be woken
 *  @param
 *  @return
 */
static void timer_tick_handler(void)
{

}


/**
 *  @fn timer_dispatcher()
 *  @brief	thread which dispatches services when expiration occur
 *  @param
 *  @return
 */
static void timer_dispatcher(void *args)
{
	(void)args;
	k_status_t err;
	archtype_t signals = 0, clear_msk =0;

#if(K_ENABLE_TIMERS > 0)
	actual_timer = NULL;
#endif
	next_task_wake = NULL;


	for(;;){
		/* The dispatcher manages the incoming commands for kernel timer, 
		 * when the timer is not running and a fresh load was issued, 	
		 * the timer places the first point of timelinte. when timer expires
		 * the dispatcher is invoked and perform handling of current timer
		 * either dispatching or waking up a waiting thread (which called)
		 * a timer poll, and after sorts the timer list for the next wakeup
		 */
		if(!signals) {
			signals = thread_wait_signals(NULL, K_TIMER_LOAD_FRESH | K_TIMER_DISPATCH | K_TIMER_REFRESH | K_TIMER_TICK,
					k_wait_match_any, &err);
			ULIPE_ASSERT(err != k_status_error);

		}

		if(signals & K_TIMER_TICK) {
			signals &= ~(K_TIMER_TICK);
			clear_msk |= K_TIMER_TICK;

			/* check if exist some thread to wake */
			timer_step_tick();


			/* check if time to wakeup */
			if((tick_count >= next_task_wake->wake_tick) && (next_task_wake != NULL)) {
				/* put this thread on ready list making it ready*/
				next_task_wake->thread_wait &= ~(K_THR_PEND_TICKER);
				sys_dlist_remove(&next_task_wake->thr_link);
				k_status_t err = k_make_ready(next_task_wake);
				ULIPE_ASSERT(err == k_status_ok);

				/* get the next task */
				k_list_t *head = sys_dlist_peek_head(&k_ticker_list);
				if(!head) {
					next_task_wake = NULL;
				} else {
					next_task_wake = CONTAINER_OF(head, tcb_t, thr_link);
				}


				/* remove all expired tasks from delayed list */
				while((tick_count >= next_task_wake->wake_tick) && (next_task_wake != NULL)) {
					/* put this thread on ready list making it ready*/
					next_task_wake->thread_wait &= ~(K_THR_PEND_TICKER);
					sys_dlist_remove(&next_task_wake->thr_link);
					k_status_t err = k_make_ready(next_task_wake);
					ULIPE_ASSERT(err == k_status_ok);

					/* get the next task */
					k_list_t *head = sys_dlist_peek_head(&k_ticker_list);
					if(!head) {
						next_task_wake = NULL;
					} else {
						next_task_wake = CONTAINER_OF(head, tcb_t, thr_link);
					}
				}
			}

#if(K_ENABLE_TIMER_GENERIC_SUPPORT > 0)

			if((tick_count >= actual_timer->load_val) && (actual_timer != NULL)) {
				/* put this thread on ready list making it ready*/
				sys_dlist_remove(&actual_timer->timer_list_link);

				if(actual_timer->cb != NULL)
					actual_timer->cb(actual_timer->user_data, actual_timer);

				actual_timer->running = false;
				actual_timer->expired = true;

				/* get the next task */
				k_list_t *head = sys_dlist_peek_head(&k_timed_list);
				if(!head) {
					actual_timer = NULL;
				} else {
					actual_timer = CONTAINER_OF(head, ktimer_t, timer_list_link);
				}



				while((tick_count >= actual_timer->load_val) && (actual_timer != NULL)) {
					/* put this thread on ready list making it ready*/
					sys_dlist_remove(&actual_timer->timer_list_link);

					if(actual_timer->cb != NULL)
						actual_timer->cb(actual_timer->user_data, actual_timer);

					actual_timer->running = false;
					actual_timer->expired = true;

					/* get the next task */
					k_list_t *head = sys_dlist_peek_head(&k_timed_list);
					if(!head) {
						actual_timer = NULL;
					} else {
						actual_timer = CONTAINER_OF(head, ktimer_t, timer_list_link);
					}
				}

			}
#endif

		}

#if((K_ENABLE_TIMERS > 0) && (K_ENABLE_TIMER_GENERIC_SUPPORT<= 0))
		/* select commands with priority */
		if(signals & K_TIMER_DISPATCH) {
			signals &= ~(K_TIMER_DISPATCH);
			clear_msk |= K_TIMER_DISPATCH;
			port_timer_halt();

			/*
			 * we know how timer needs to be woken
			 */
			if(actual_timer->cb)
				actual_timer->cb(actual_timer->user_data,actual_timer);

			/*
			 * thread waiting for it adds to ready list 
			 */
			if(actual_timer->threads_pending.bitmap != 0) {
				key = port_irq_lock();
				tcb_t *thr = k_unpend_obj(&actual_timer->threads_pending);
				ULIPE_ASSERT(thr != NULL);

				thr->thread_wait &= ~(K_THR_PEND_TMR);

				k_status_t err = k_make_ready(thr);
				ULIPE_ASSERT(err == k_status_ok);
			}


			/* drops the current timer */
			sys_dlist_remove(&actual_timer->timer_list_link);
			actual_timer->running = false;
			actual_timer->expired = true;

	
			/* find next timer to pend */
			actual_timer= timer_period_sort(&k_timed_list);
			if(actual_timer == NULL) {
				/* no timers to run */
				no_timers = true;
			} else {
				port_timer_load_append(actual_timer->load_val);
				port_timer_resume();
			}

		}


		if(signals & K_TIMER_REFRESH) {
			signals &= ~(K_TIMER_REFRESH);
			clear_msk |= K_TIMER_REFRESH;
			no_timers = false;

			port_timer_halt();


			/* iterate list and schedule a new timer on timeline */
			ktimer_t *tmp = timer_period_sort(&k_timed_list);
			if(tmp != actual_timer) {
				actual_timer = tmp;
				port_timer_load_append(actual_timer->load_val);

			}
			port_timer_resume();

		}

		if(signals & K_TIMER_LOAD_FRESH) {
			signals &= ~(K_TIMER_LOAD_FRESH);
			clear_msk |= K_TIMER_LOAD_FRESH;
			k_list_t *head;
			no_timers = false;

			/* gets the only available container of timed list */
			head = sys_dlist_peek_head(&k_timed_list);
			ULIPE_ASSERT(head != NULL);	
			actual_timer = CONTAINER_OF(head, ktimer_t, timer_list_link);
			port_irq_unlock(key);


			/* no timers running so put a new match value */
			port_start_timer(actual_timer->load_val);
			/* start the timeline running */
			no_timers = false;
		}
#endif
		thread_clr_signals(&timer_tcb, clear_msk);
		clear_msk = 0;


		wu_info.next_thread_wake = next_task_wake;
		wu_info.next_timer = actual_timer;
		wu_info.tick_cntr = &tick_count;

	}
}



/** public functions */
#if(K_ENABLE_TIMERS > 0)

k_status_t timer_start(timer_id_t t)
{
	k_status_t ret = k_status_ok;

	ktimer_t *tim = (ktimer_t *)t;

	if(tim == NULL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}
	
	if(tim->running){
		ret = k_timer_running;
		goto cleanup;
	}

	if(!tim->timer_to_wait) {
		/* timer not loaded cannot start */
		ret = k_status_invalid_param;
		goto cleanup;
	}

	k_sched_lock();

	if(!tim->created) {
		tim->created = true;
		k_work_list_init(&tim->threads_pending);
		sys_dlist_init(&tim->timer_list_link);

	}


	tim->expired = false;


#if (K_ENABLE_TIMER_GENERIC_SUPPORT > 0)
	tim->load_val = tim->timer_to_wait + tick_count;
	sys_dlist_append(&k_timed_list, &tim->timer_list_link);

#else

	/* new valid timer added to list 
	 * insert it on timeline
	 */
	k_sched_unlock();
	timer_rebuild_timeline(tim, &key);
#endif


cleanup:
	return(ret);

}


k_status_t timer_stop(timer_id_t t)
{
	k_status_t ret = k_status_ok;
	ktimer_t *tim = (ktimer_t *)t;


	if(tim == NULL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	if(tim->expired){
		ret = k_timer_expired;
		goto cleanup;
	}



	k_sched_lock();

	if(!tim->created) {
		tim->created = true;
		k_work_list_init(&tim->threads_pending);
		sys_dlist_init(&tim->timer_list_link);

	}


	tim->running = false;
	tim->expired = true;


#if (K_ENABLE_TIMER_GENERIC_SUPPORT > 0)
	sys_dlist_remove(&tim->timer_list_link);

	/* get the next task */
	k_list_t *head = sys_dlist_peek_head(&k_timed_list);
	if(!head) {
		actual_timer = NULL;
	} else {
		actual_timer = CONTAINER_OF(head, ktimer_t, timer_list_link);
	}

#else
	port_timer_halt();
	sys_dlist_remove(&tim->timer_list_link);

	/* new valid timer added to list
	 * insert it on timeline
	 */
	actual_timer= timer_period_sort(&k_timed_list);
	if(actual_timer == NULL) {
		/* no timers to run */
		no_timers = true;
	} else {
		port_timer_load_append(actual_timer->load_val);
		port_timer_resume();
	}

#endif

	k_sched_unlock();

cleanup:
	return(ret);

}


k_status_t timer_poll(timer_id_t t)
{
	k_status_t ret = k_status_ok;

/* timer in generic support not allows the poll function,
 * use the ticker timer wait
 */
#ifndef K_ENABLE_TIMER_GENERIC_SUPPORT
	ktimer_t *tim = (ktimer_t *)t;


	if(tim == NULL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	if(!tim->running) {
		/* timer must be running */
		ret = k_timer_stopped;
		goto cleanup;
	}

	k_sched_lock();

	if(!tim->created) {
		tim->created = true;
		k_work_list_init(&tim->threads_pending);
		sys_dlist_init(&tim->timer_list_link);
	}


	/* only one thread per timer can poll */
	if(tim->threads_pending.bitmap) {
		ret = k_timer_busy;
		k_sched_unlock();
		goto cleanup;
	}

	/* check if timer is already expired */
	if(tim->expired){
		ret = k_timer_expired;
		k_sched_unlock();
		goto cleanup;
	}	

	/* 
	 * If it safe to poll timer, sleep thread
	 * until the dispatcher wakes it up.
	 */


	tcb_t *thr = thread_get_current();
	ULIPE_ASSERT(thr != NULL);

	ret = k_make_not_ready(thr);
	ULIPE_ASSERT(ret == k_status_ok);
	thr->thread_wait |= K_THR_PEND_TMR;

	ret = k_pend_obj(thr, &tim->threads_pending);
	ULIPE_ASSERT(ret == k_status_ok);
	k_sched_unlock();
	
	/* reescheduling is needed */
	ret = k_sched_and_swap();
	ULIPE_ASSERT(ret == k_status_ok);	

cleanup:
#endif

	return(ret);

}



k_status_t timer_set_callback(timer_id_t t, ktimer_callback_t cb, void *user_data)
{
	k_status_t ret = k_status_ok;
	ktimer_t *tim = (ktimer_t *)t;


	if(tim == NULL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	if(cb == NULL) {
		ret = k_status_invalid_param;
		goto cleanup;		
	}


	k_sched_lock();

	if(!tim->created) {
		tim->created = true;
		k_work_list_init(&tim->threads_pending);
		sys_dlist_init(&tim->timer_list_link);

	}


	if(tim->running) {
		ret = k_timer_running;
		k_sched_unlock();
		goto cleanup;
	}

	tim->cb = cb;
	if(user_data)
		tim->user_data = user_data;

	k_sched_unlock();

cleanup:
	return(ret);
}


k_status_t timer_set_load(timer_id_t t, uint32_t load_val)
{
	k_status_t ret = k_status_ok;
	ktimer_t *tim = (ktimer_t *)t;

	if(tim == NULL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	k_sched_lock();
	
	if(!tim->created) {
		tim->created = true;
		k_work_list_init(&tim->threads_pending);
		sys_dlist_init(&tim->timer_list_link);

	}

	if(tim->running){
		/* cannot set load value of a running timer */
		ret = k_timer_running;
		k_sched_unlock();
		goto cleanup;
	}

	tim->timer_to_wait = load_val;

	k_sched_unlock();
	
cleanup:
	return(ret);
}


timer_id_t timer_create(uint32_t load_value)
{
	ktimer_t *ret = NULL;

	if(!load_value)
		goto cleanup;

	ret = (ktimer_t *)k_malloc(sizeof(ktimer_t));

	if(ret) {
		ret->timer_to_wait=load_value;
		ret->running=false;
		ret->expired=true;
		ret->threads_pending.bitmap=0;
		ret->created=false;
	}

cleanup:
	return((timer_id_t)ret);
}


k_status_t timer_delete(timer_id_t tim)
{
	k_status_t ret = k_status_ok;
	ktimer_t *t = (ktimer_t *)tim;


	if(t == NULL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	k_sched_lock();

	if(t->expired == false) {
		ret = k_timer_busy;
		k_sched_unlock();
		goto cleanup;
	}

	k_sched_unlock();	
	k_free(t);


cleanup:
	return(ret);
}

#endif


k_status_t ticker_timer_wait(uint32_t ticks)
{
	k_status_t ret = k_status_ok;
	if(!ticks) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	tcb_t *thr = thread_get_current();
	ULIPE_ASSERT(thr != NULL);

	k_sched_lock();

	ret = k_make_not_ready(thr);
	ULIPE_ASSERT(ret == k_status_ok);
	thr->wake_tick = tick_count + ticks;
	thr->thread_wait |= K_THR_PEND_TICKER;

	/* put the new timer on ticker list */
	sys_dlist_append(&k_ticker_list, &thr->thr_link);

	k_sched_unlock();
	/* reescheduling is needed */
	ret = k_sched_and_swap();
	ULIPE_ASSERT(ret == k_status_ok);	
cleanup:
	return(ret);
}


uint32_t timer_get_tick_count(void)
{
	return(tick_count);
}


#endif
