/**
 * 							ULIPE RTOS PICO
 *
 *  @file k_mk.c
 *
 *  @brief this file is the glue module and packs all kernel files in a single source file 
 *
 *
 *
 */

/* prevents recursive inclusion */
#ifndef __K_MK_C
#define __K_MK_C


/* add configuration */
#include "../ulipe_rtos_pico.h"
#include "../include/utils/k_list.h"


/* internal kernel information */
#include "include/k_priv.h"
#include "include/k_port.h"


/* adds kernel source file */
#include "src/k_kernel.c"
#include "src/k_thread.c"
#include "src/k_message.c"
#include "src/k_raw_timer.c"
#include "src/k_sema.c"
#include "src/k_mutex.c"
#include "src/k_malloc.c"
#include "../arch/arch_mk.c"

#endif

