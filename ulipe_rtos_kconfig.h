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

/* kernel debugging */
#define K_DEBUG 	1

/* architecture definition */
#define ARCH_TYPE_AVR_TINY	1

/* enable avr tiny specific code (disabled for use with atmega 64 or below)
 */
#define ARCH_ENABLE_AVR_TINY_SPECS	0


/* architecture data width */
#define K_ARCH_MEM_WIDTH_BYTE	1

/* general kernel configuration */
#define K_MINIMAL_STACK_VAL	64
#define K_TIMER_DISPATCHER_PRIORITY	-1
#define K_TIMER_DISPATCHER_STACK_SIZE	128
#define K_ENABLE_SEMAPHORE	1
#define K_ENABLE_MESSAGING	1
#define K_ENABLE_TIMERS		1

#endif
