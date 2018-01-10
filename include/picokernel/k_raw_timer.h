/**
 * 							ULIPE RTOS PICO
 *  @file k_raw_timer.h
 *
 *  @brief timer primitive kernel header file
 *
 */

#ifndef __K_RAW_TIMER_H
#define __K_RAW_TIMER_H


#if(K_ENABLE_TICKER > 0)
#if(K_ENABLE_TIMERS > 0)


/** timer id typed */
typedef void* timer_id_t;


/**
 * timer callback function type
 *
 * @param user_data - custom data passed by user
 * @param timer - instance of expired timer
 *
 * @return none
 *
 */
typedef void (*ktimer_callback_t) (void * user_data, void *timer);


/**
 *  @fn timer_create()
 *  @brief creates a fully initialized timer control block
 *  @param load_value - initial load value (can be changed using timer API)
 *
 *  @return a ktimer_t control structure ready to use
 */
timer_id_t timer_create_dynamic(uint32_t load_value);


/**
 *  @fn timer_delete()
 *  @brief destroys a previous allocated timer control block
 *
 *  @param tim - timer to be destroyed
 *
 *  @return k_status_ok or error code in case of invalid use
 */
k_status_t timer_delete_dynamic(timer_id_t tim);


/**
 *  @fn timer_start()
 *  @brief starts the specified timer to count
 *
 *  @param t - timer instance to be started
 *
 *  @return k_status_ok or error in case of invalid values
 *
 */
k_status_t timer_start(timer_id_t t);


/**
 *  @fn timer_stop()
 *  @brief stops a timer to count and remove it from active timers list
 *
 *  @param t - timer to be stopped
 *
 *  @return k_status_ok or error in case of invalid values
 */
k_status_t timer_stop(timer_id_t t);


/**
 *  @fn timer_poll()
 *  @brief check if a timer expired, blocks if not expired yet
 *
 *  @param t - timer to be polled
 *
 *  @return k_status_ok or error in case of invalid values
 */
k_status_t timer_poll(timer_id_t t);


/**
 *  @fn timer_set_callback()
 *  @brief sets a callback for execution when timer expires
 *
 *  @param t - timer to be callback set
 *  @param cb - callback to be called when timer expires
 *  @param user_data - custom data to be passed to callback
 *
 *  @return k_status_ok or error in case of invalid values / timmer already running
 */
k_status_t timer_set_callback(timer_id_t t, ktimer_callback_t cb, void *user_data);

/**
 *  @fn timer_set_load()
 *  @brief set counting time value
 *
 *  @param t - timer to have counting modified
 *  @param load_val - counting value
 *
 *  @return k_status_ok or error in case of invalid values / timmer already running
 */
k_status_t timer_set_load(timer_id_t t, uint32_t load_val);
#endif


/**
 *  @fn ticker_timer_wait()
 *  @brief Sleeps the current thread by some ticks amount
 *
 *  @param ticks - ticks to sleep the current thread
 *
 *  @return k_status_ok or error in case of invalid values
 */ 
k_status_t ticker_timer_wait(uint32_t ticks);


/**
 *  @fn timer_get_tick_count()
 *  @brief gets the current tick count
 *
 *  @param none
 *
 *  @return current tick count value
 */
uint32_t timer_get_tick_count(void);


#endif
#endif
