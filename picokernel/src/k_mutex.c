/**
 * 							ULIPE RTOS PICO
 *
 *  @file k_mutex.h
 *
 *  @brief mutual exclusion semaphore stuff
 *
 */

 #if(K_ENABLE_MUTEX > 0)

/** internal functions */


/** public functions */
k_status_t mutex_take(mtx_id_t m, bool try)
{
	k_status_t ret = k_status_ok;
	bool reesched = false;
	tcb_t *t = (tcb_t *)thread_get_current();
	ULIPE_ASSERT(t != NULL);


	if(m == NULL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	if(port_from_isr()){
		/* take cannot be called from ISR */
		ret = k_status_illegal_from_isr;
		goto cleanup;
	}

	kmutex_t *mt = (kmutex_t *)m;
	archtype_t key = port_irq_lock();


	if(!mt->created) {
		/* handle first time usage */
		k_work_list_init(&mt->threads_pending);
		mt->created = true;
	}


	if(mt->thr_owner == NULL && (try)) {
		port_irq_unlock(key);
		goto cleanup;
	}

	if(mt->thr_owner != NULL ) {
		/*
		 * if no key available, we need to insert the waiting thread
		 * on mutex pending list, when a key will become available
		 * the task will be woken as well
		 */

		ret = k_make_not_ready(t);
		ULIPE_ASSERT(ret == k_status_ok);
		t->thread_wait |= K_THR_PEND_MTX;
		ret = k_pend_obj(t, &mt->threads_pending);
		ULIPE_ASSERT(ret == k_status_ok);

		/* but in mutex case, if the owner has a priority
		 * too low, raise it with the priority of current task
		 */
		reesched = true;

	} else {

		mt->thr_owner = t;
		mt->owner_prio = t->thread_prio;
		thread_set_prio((tid_t)t, K_MUTEX_PRIO_CEIL_VAL);
	}


	if(!reesched){
		port_irq_unlock(key);
		goto cleanup;
	}

	port_irq_unlock(key);
	/*
	 * if current thread entered on pending state, we need to reesched the
	 * thread set and find a new thread to execute, otherwise, dispatch idle
	 */
	k_sched_and_swap();

cleanup:
	return(ret);
}



k_status_t mutex_give(kmutex_t *m)
{
	k_status_t ret = k_status_ok;
	tcb_t *t = NULL;
	tcb_t *cur = thread_get_current();

	if(port_from_isr()){
		/* take cannot be called from ISR */
		ret = k_status_illegal_from_isr;
		goto cleanup;
	}


	if(m == NULL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	kmutex_t *mt = (kmutex_t *)m;


	if(cur != mt->thr_owner) {
		/* only the mutex owner can release it */
		ret = k_status_invalid_param;
		goto cleanup;
	}

	if(mt->thr_owner == NULL) {
		ret = k_mutex_already_available;
		goto cleanup;

	}

	archtype_t key = port_irq_lock();
	if(!mt->created) {
		/* handle first time usage */
		k_work_list_init(&mt->threads_pending);
		mt->created = true;
	}

	/* restore thread original priority */

	/*
	 * once a mutex was updated its keys
	 * we need to verify if a new highprio task is available to
	 * run
	 */
	t = k_unpend_obj(&mt->threads_pending);
	if(t == NULL) {
		/* no tasks pendings, just get out here */
		uint8_t tmp = mt->owner_prio;
		mt->owner_prio = 0;
		mt->thr_owner  = NULL;

		/* restore thread original priority */
		thread_set_prio((tid_t)cur, tmp);

		port_irq_unlock(key);
		goto cleanup;
	} else {
		uint8_t tmp = mt->owner_prio;
		mt->thr_owner = t;
		mt->owner_prio = t->thread_prio;
		t->thread_wait &= ~(K_THR_PEND_MTX);

		ret = k_make_ready(t);
		ULIPE_ASSERT(ret == k_status_ok);
		/*restore last owner original prio */
		thread_set_prio((tid_t)cur, tmp);


		port_irq_unlock(key);
		k_sched_and_swap();
	}


cleanup:
	return(ret);
}


mtx_id_t mutex_create(void)
{
	kmutex_t *ret = (kmutex_t*)k_malloc(sizeof(kmutex_t));

	if(ret) {
		ret->thr_owner=NULL;
		ret->owner_prio=0;
		ret->created=false;
	}

	return((mtx_id_t)ret);
}


k_status_t mutex_delete(mtx_id_t mtx)
{
	k_status_t ret = k_status_ok;
	kmutex_t *m = (kmutex_t *)mtx;

	if(mtx == NULL)
		goto cleanup;

	archtype_t key = port_irq_lock();

	/*
	 * Mutexes only can be deleted if no tasks are pending, otherwise
	 * returns a busy error to hint application to signal to all waiting
	 * tasks
	 */
	if(m->threads_pending.bitmap){
		ret = k_status_error;
		port_irq_unlock(key);
		goto cleanup;
	}

	/* release the memory */
	k_free(m);


	port_irq_unlock(key);

cleanup:
	return(ret);
}

#endif