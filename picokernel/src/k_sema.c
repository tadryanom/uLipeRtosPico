/**
 * 							ULIPE RTOS PICO
 *
 *  @file k_sema.h
 *
 *  @brief basic semaphore usage file
 *
 */

/** public functions */
k_status_t semaphore_take(sema_id_t s)
{
	k_status_t ret = k_status_ok;
	bool reesched = false;
	tcb_t *t = (tcb_t *)thread_get_current();
	ULIPE_ASSERT(t != NULL);


	if(s == NULL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	ksema_t *sem = (ksema_t *)s;


	if(port_from_isr()){
		/* take cannot be called from ISR */
		ret = k_status_illegal_from_isr;
		goto cleanup;
	}

	k_sched_lock();

	if(!sem->created) {
		/* handle first time usage */
		k_work_list_init(&sem->threads_pending);
		sem->created = true;
	}


	if(!sem->cnt) {
		/*
		 * if no key available, we need to insert the waiting thread
		 * on semaphore pending list, when a key will become available
		 * the task will be woken as well
		 */
		ret = k_make_not_ready(t);
		ULIPE_ASSERT(ret == k_status_ok);

		t->thread_wait |= K_THR_PEND_SEMA;

		ret = k_pend_obj(t, &sem->threads_pending);
		ULIPE_ASSERT(ret == k_status_ok);

		reesched = true;
	} else {
		if(sem->cnt > 0)
			sem->cnt--;
	}


	if(!reesched){
		k_sched_unlock();
		goto cleanup;
	}

	k_sched_unlock();
	/*
	 * if current thread entered on pending state, we need to reesched the
	 * thread set and find a new thread to execute, otherwise, dispatch idle
	 */
	k_sched_and_swap();

cleanup:
	return(ret);
}


k_status_t semaphore_give(sema_id_t s, uint32_t count)
{
	k_status_t ret = k_status_ok;
	tcb_t *t = NULL;

	if(s == NULL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	if(!count) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	ksema_t *sema = (ksema_t *)s;

	k_sched_lock();

	if(!sema->created) {
		/* handle first time usage */
		k_work_list_init(&sema->threads_pending);
		sema->created = true;
	}

	sema->cnt+= count;
	if(sema->cnt > sema->limit)
		sema->cnt = sema->limit;


	/*
	 * once a semaphore was updated its keys
	 * we need to verify if a new highprio task is available to
	 * run
	 */
	t = k_unpend_obj(&sema->threads_pending);
	if(t == NULL) {
		/* no tasks pendings, just get out here */
		k_sched_unlock();
		goto cleanup;
	} else {

		if(sema->cnt > 0)
			sema->cnt--;
	}

	t->thread_wait &= ~(K_THR_PEND_SEMA);

	ret = k_make_ready(t);
	ULIPE_ASSERT(ret == k_status_ok);
	k_sched_unlock();
	
	k_sched_and_swap();

cleanup:
	return(ret);
}


sema_id_t semaphore_create(uint32_t initial, uint32_t limit)
{
	ksema_t *ret = (ksema_t*)k_malloc(sizeof(ksema_t));

	if(ret) {
		ret->cnt=initial;
		ret->limit=limit;
		ret->created=false;
	}

	return((sema_id_t)ret);
}


k_status_t semaphore_delete(sema_id_t sem)
{
	k_status_t ret = k_status_ok;

	if(sem == NULL)
		goto cleanup;

	ksema_t *s = (ksema_t *)sem;

	k_sched_lock();

	/*
	 * Semaphores only can be deleted if no tasks are pending, otherwise
	 * returns a busy error to hint application to signal to all waiting
	 * tasks
	 */
	if(s->threads_pending.bitmap){
		ret = k_sema_not_available;
		k_sched_unlock();
		goto cleanup;
	}

	k_sched_unlock();	
	/* release the memory */
	k_free(sem);
	
cleanup:
	return(ret);
}

