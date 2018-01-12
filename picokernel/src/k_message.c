/**
 * 							ULIPE RTOS PICO
 *
 *  @file k_message.c
 *
 *  @brief basic messaging usage file
 *
 */

#if (K_ENABLE_MESSAGING > 0)

/** private functions */


/**
 *  @fn message_handle_pend()
 *  @brief check kmsgt internal thread list for a insertion waiting task
 *  @param
 *  @return
 */
static k_status_t message_handle_pend(kmsg_t *m,bool insert, msg_opt_t opt, archtype_t key)
{
	k_status_t ret = false;
	tcb_t *thr = thread_get_current();
	ULIPE_ASSERT(thr != NULL);



	switch(opt) {
		case k_msg_accept:
			/* simplest case, accept there is no space left
			 * on message queue and reports error
			 */
			if(insert)
				ret = k_queue_full;
			else
				ret = k_queue_empty;


			break;

		case k_msg_block:
			/* thread will wait until one slot become free */
			k_make_not_ready(thr);
			thr->thread_wait |= K_THR_PEND_MSG;

			if(insert)
				k_pend_obj(thr, &m->wr_threads_pending);
			else
				k_pend_obj(thr, &m->rd_threads_pending);


			port_irq_unlock(key);
			ret = k_sched_and_swap();
			ULIPE_ASSERT(ret == k_status_ok);

			/* thread woken, lock acess to queue again */
			ret = k_status_ok;
			key = port_irq_lock();
			break;

		default:
			ret = k_status_invalid_param;
			break;
	}

	return(ret);
}

/** Public functions */


k_status_t message_insert(msg_id_t m, void *data, uint32_t size, msg_opt_t opt)
{
	k_status_t ret = k_status_ok;

	if(m == NULL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	kmsg_t *msg = (kmsg_t *)m;


	if(data == NULL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	if(size > msg->slot_size) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	if(!size) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	k_sched_lock();

	if(!msg->created) {
		msg->created = true;
		k_work_list_init(&msg->wr_threads_pending);
		k_work_list_init(&msg->rd_threads_pending);
	}

	if(msg->items >= msg->slots_number) {
		/* message queue is actually full,
		 * so check the wait options passed by user
		 */
		ret = message_handle_pend(msg,true, opt, key);

		if(ret != k_status_ok) {
			k_sched_unlock();
			goto cleanup;
		}
	}

	


	/* regular use case, insert a new frame on message
	 * queue and wake up a possible thread set
	 */
	archtype_t *ptr = (archtype_t *)&msg->data[msg->wr_ptr * (msg->slot_size)];
	ULIPE_ASSERT(ptr != NULL);
	*ptr++ = size;

	/* just a ordinary copy */
	memcpy(&msg->data[(msg->wr_ptr * (msg->slot_size)) + sizeof(archtype_t)], data, size);

	msg->wr_ptr = (msg->wr_ptr + 1) % msg->slots_number;
	msg->items++;



	tcb_t *thr = k_unpend_obj(&msg->rd_threads_pending);
	if(thr == NULL) {
		/* no need to reeschedule task list */
		k_sched_unlock();
		goto cleanup;
	}

	thr->thread_wait &= ~(K_THR_PEND_MSG);
	ret = k_make_ready(thr);
	ULIPE_ASSERT(ret == k_status_ok);
	k_sched_unlock();
	

	/* reeschedule task set */
	ret = k_sched_and_swap();
	ULIPE_ASSERT(ret == k_status_ok);

cleanup:
	return(ret);
}



k_status_t message_remove(msg_id_t msg, void *data, uint32_t *size, bool peek, msg_opt_t opt)
{
	k_status_t ret = k_status_ok;

	if(msg== NULL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	kmsg_t *m = (kmsg_t *)m;

	if(data == NULL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	if(size == NULL) {
		ret = k_status_invalid_param;
		goto cleanup;
	}

	k_sched_lock();

	if(!m->created) {
		m->created = true;
		k_work_list_init(&m->wr_threads_pending);
		k_work_list_init(&m->rd_threads_pending);
	}



	if(m->items == 0) {
		/* message queue is actually empty,
		 * so check the wait options passed by user
		 */
		ret = message_handle_pend(m,false, opt,key);

		if(ret != k_status_ok) {
			k_sched_unlock();
			goto cleanup;
		}

	}

	archtype_t *ptr = (archtype_t *)&m->data[m->rd_ptr * (m->slot_size )];
	ULIPE_ASSERT(ptr != NULL);
	archtype_t data_size = *ptr++;

	/*
	 * we have at least one slot pending for remotion, lets pick
	 * it
	 */

	/* just a ordinary copy */
	memcpy(data, &m->data[(m->rd_ptr * m->slot_size) + sizeof(archtype_t)  ], data_size);
	*size = data_size;


	/*
	 * if a peek was requested remove the entry, otherwise, nothing changes
	 * just exit
	 */
	if(peek) {
		k_sched_unlock();
		goto cleanup;
	}

	m->rd_ptr = (m->rd_ptr + 1) % m->slots_number;
	m->items--;

	tcb_t *thr = k_unpend_obj(&m->wr_threads_pending);
	if(thr == NULL) {
		k_sched_unlock();
		goto cleanup;
	}


	ret = k_make_ready(thr);
	ULIPE_ASSERT(ret == k_status_ok);
	k_sched_unlock();
	
	ret = k_sched_and_swap();
	ULIPE_ASSERT(ret == k_status_ok);
cleanup:
	return(ret);
}



msg_id_t message_create(uint32_t noof_slots, uint32_t slot_size_val)
{

	kmsg_t *ret = NULL;

	/* both slot size and its quantity needs to be valid */
	if((noof_slots == 0) || (slot_size_val == 0))
		goto cleanup;


	ret = k_malloc(sizeof(kmsg_t));

	/* maybe we have not enough memory */
	if(ret == NULL);
		goto cleanup;


	uint8_t *slots = k_malloc(noof_slots * (slot_size_val + sizeof(archtype_t)));

	/* not enough memory, cleanup and exit */
	if(slots == NULL) {
		k_free(ret);
		ret = NULL;
		goto cleanup;
	}


	/* initialize block */

	ret->data = slots;
	ret->items = 0;
	ret->slots_number = noof_slots;
	ret->wr_ptr = 0;
	ret->rd_ptr = 0;
	ret->slot_size = slot_size_val;
	ret->wr_threads_pending.bitmap=0;
	ret->rd_threads_pending.bitmap=0;
	ret->created=false;



cleanup:
	return((msg_id_t)ret);
}


k_status_t message_delete(msg_id_t msg)
{
	k_status_t ret = k_status_ok;

	if(msg == NULL){
		ret = k_status_invalid_param;
		goto cleanup;
	}


	kmsg_t* m = (kmsg_t *)msg;


	k_sched_lock();

	/* cannot delete a message with waiting tasks
	 */
	if((m->rd_threads_pending.bitmap) || (m->wr_threads_pending.bitmap)) {
		ret = k_status_error;
		k_sched_unlock();
		goto cleanup;
	}

	k_sched_unlock();
	
	/* release memory and exit */
	k_free(m->data);
	k_free(m);

cleanup:
	return(ret);
}


#endif
