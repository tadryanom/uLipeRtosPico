/**
 * 							ULIPE RTOS PICO
 *  @file k_message.h
 *
 *  @brief basic messaging usage header file
 *
 */
#ifndef __K_MESSAGE_H
#define __K_MESSAGE_H

#if(K_ENABLE_MESSAGING > 0)

/* options for queue usage */
typedef enum {
	k_msg_block = 0,
	k_msg_accept,
}msg_opt_t;


#if(K_ENABLE_DYNAMIC_ALLOCATOR > 0)
/**
 *  @fn message_create_dynamic()
 *  @brief creates a fully initialized message control block
 *
 *  @param noof_slots - number of elements of this message
 *  @param slot_size_val - size in bytes of this message slot
 *
 *  @return a ktimer_t control structure ready to use
 */
kmsg_t * message_create_dynamic(uint32_t noof_slots, uint32_t slot_size_val);


/**
 *  @fn message_delete_dynamic()
 *  @brief destroys a previous allocated message control block
 *
 *  @param msg - message to be destroyed
 *
 *  @return k_status_ok or error code in case of invalid use
 */
k_status_t message_delete_dynamic(kmsg_t * msg);

#endif

/**
 *  @fn message_insert()
 *  @brief insert data on message queue and optionally block if there is no space free
 *
 *  @param m - message control block will hold the data inserted
 *  @param data - data to be inserted on message queue
 *  @param size - size of data amount to be inserted, it cannot be more than slot_size_val of control block
 *  @param opt - hints to kernel to block current task if no space left if k_msg_block is passed
 *
 *  @return k_status_ok or error for invalid values/queue full
 */
k_status_t message_insert(kmsg_t *m, void *data, uint32_t size, msg_opt_t opt);



/**
 *  @fn message_remove()
 *  @brief remove or peek a message from head of queue and optionally blocks if no space left
 *
 *  @param msg - message control block which holds the messages
 *  @param data - pointer to store data extracted from queue
 *  @param size - pointer to store the size of data extracted from queue
 *  @param peek - if true the data is copied from queue but is not removed from it
 *  @param opt  - hints to kernel to block current task if queue is empty  if k_msg_block is passed
 *
 *  @return k_status_ok or error for invalid values/queue empty
 */
k_status_t message_remove(kmsg_t *msg, void *data, uint32_t *size,bool peek, msg_opt_t opt);




#endif


#endif


