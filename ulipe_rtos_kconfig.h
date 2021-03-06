/**
 * 							ULIPE RTOS PICO
 *
 *  @file ulipe_rtos_kconfig.h
 *
 *  @brief kernel configuration file
 *
 *
 */
#ifndef __ULIPE_RTOS_KCONFIG_H
#define __ULIPE_RTOS_KCONFIG_H

#include "include/utils/arch_supported.h"


/* if no configuration was provided by user, load the default one */
#ifndef K_USER_CONFIG

/* kernel debugging */
#define K_DEBUG 						1
/* architecture definition */
#define ARCH_TYPE					    ARM_CMX

#define K_MACHINE_CLOCK					12000000
#define K_TICKER_RATE					1000

/* architecture data width */
#define K_ARCH_MEM_WIDTH_WORD	        1

/* general kernel configuration */
#define K_HEAP_SIZE						16 * 1024
#define K_MINIMAL_STACK_VAL	            64
#define K_TIMER_DISPATCHER_PRIORITY	    24
#define K_TIMER_DISPATCHER_STACK_SIZE	128
#define K_ENABLE_SEMAPHORE				1
#define K_ENABLE_MESSAGING				1
#define K_ENABLE_TICKER					1
#define K_ENABLE_TIMERS					1
#define K_ENABLE_TIMER_GENERIC_SUPPORT	1
#define K_ENABLE_MUTEX					1

/* Tickless idle support */
#define K_ENABLE_TICKLESS_IDLE			0
#define K_MAX_LOW_POWER_PERIOD			(500)


#else
#include "ulipe_rtos_user_cfg.h"
#endif

#endif
