/**
 * 							ULIPE RTOS PICO
 *
 *  @file arch_mk.c
 *
 *  @brief architecture specific code glue module
 *
 *
 */

#ifndef __ARCH_MK_C
#define __ARCH_MK_C


/** Select the architecture code added to support the ulipe kernel */

#if(ARCH_TYPE == AVR_MEGA)
	#include "avr_mega/arch.c"
#elif (ARCH_TYPE == ARM_CM0)
	#include "arm_cm0/arch.c"
#elif (ARCH_TYPE == ARM_CMX)
	#include "arm_cmx/arch.c"
#else
	#error "Fatal: architecture unknown or not supported, please check your ulipe_rtos_kconfig file"
#endif


#endif

