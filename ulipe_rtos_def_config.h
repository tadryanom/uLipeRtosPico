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
#define K_DEBUG							1

/* architecture definition */
#define ARCH_TYPE_ARM_CM0				1


/* architecture data width */
#define K_ARCH_MEM_WIDTH_WORD			1

/* general kernel configuration */
#define K_MINIMAL_STACK_VAL				16
#define K_TIMER_DISPATCHER_PRIORITY		-1
#define K_TIMER_DISPATCHER_STACK_SIZE	256
#define K_ENABLE_SEMAPHORE				1
#define K_ENABLE_MESSAGING				1
#define K_SVC_MAX_PRIO					0xFF


#endif