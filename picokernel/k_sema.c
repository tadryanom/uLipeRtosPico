/**
 * 							ULIPE RTOS PICO
 *
 *  @file k_sema.h
 *
 *  @brief basic semaphore usage file
 *
 */
#include "ulipe_rtos_pico.h"


/** public functions */
k_status_t semaphore_take(ksema_t *s)
{
	k_status_t ret = k_status_ok;
	bool reesched = false;
	tcb_t *t = thread_get_current();
	ULIPE_ASSERT(t != NULL);


	if(s == NULL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	if(port_from_isr()){
		/* take cannot be called from ISR */
		ret = k_status_illegal_from_isr;
		goto cleanup;
	}

	archtype_t key = port_irq_lock();

	if(!s->created) {
		/* handle first time usage */
		k_work_list_init(&s->threads_pending);
		s->created = true;
	}


	if(!s->cnt) {
		/*
		 * if no key available, we need to insert the waiting thread
		 * on semaphore pending list, when a key will become available
		 * the task will be woken as well
		 */
		ret = k_make_not_ready(t);
		ULIPE_ASSERT(ret == k_status_ok);

		t->thread_wait |= K_THR_PEND_SEMA;

		ret = k_pend_obj(t, &s->threads_pending);
		ULIPE_ASSERT(ret == k_status_ok);

		reesched = true;
	} else {
		if(s->cnt > 0)
			s->cnt--;
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


k_status_t semaphore_give(ksema_t *s, uint32_t count)
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

	archtype_t key = port_irq_lock();
	if(!s->created) {
		/* handle first time usage */
		k_work_list_init(&s->threads_pending);
		s->created = true;
	}

	s->cnt+= count;
	if(s->cnt > s->limit)
		s->cnt = s->limit;


	/*
	 * once a semaphore was updated its keys
	 * we need to verify if a new highprio task is available to
	 * run
	 */
	t = k_unpend_obj(&s->threads_pending);
	if(t == NULL) {
		/* no tasks pendings, just get out here */
		port_irq_unlock(key);
		goto cleanup;
	} else {

		if(s->cnt > 0)
			s->cnt--;
	}

	t->thread_wait &= ~(K_THR_PEND_SEMA);

	ret = k_make_ready(t);
	ULIPE_ASSERT(ret == k_status_ok);
	port_irq_unlock(key);


	k_sched_and_swap();

cleanup:
	return(ret);
}



#if(K_ENABLE_DYNAMIC_ALLOCATOR > 0)

ksema_t * semaphore_create_dynamic(uint32_t initial, uint32_t limit)
{
	ksema_t *ret = (ksema_t*)k_malloc(sizeof(ksema_t));

	if(ret) {
		ret->cnt=initial;
		ret->limit=limit;
		ret->created=false;
	}

	return(ret);
}


k_status_t semaphore_delete_dynamic(ksema_t * sem)
{
	k_status_t ret = k_status_ok;

	if(sem == NULL)
		goto cleanup;

	archtype_t key = port_irq_lock();

	/*
	 * Semaphores only can be deleted if no tasks are pending, otherwise
	 * returns a busy error to hint application to signal to all waiting
	 * tasks
	 */
	if(sem->threads_pending.bitmap){
		ret = k_sema_not_available;
		port_irq_unlock(key);
		goto cleanup;
	}

	/* release the memory */
	k_free(sem);


	port_irq_unlock(key);

cleanup:
	return(ret);
}

#endif
