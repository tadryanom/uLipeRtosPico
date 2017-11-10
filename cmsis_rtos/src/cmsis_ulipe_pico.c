/*
 * Copyright (c) 2013-2017 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ----------------------------------------------------------------------
 *
 * $Date:        10. January 2017
 * $Revision:    V1.2
 *
 * Project:      CMSIS-RTOS API V1
 * Title:        cmsis_os_v1.c V1 module file
 *---------------------------------------------------------------------------*/

#include "../../ulipe_rtos_pico.h"
#include "../cmsis_os.h"


#if (K_ENABLE_CMSIS_RTOS2_SUPPORT > 0)
#if (osCMSIS >= 0x20000U)


osThreadId osThreadCreate (const osThreadDef_t *thread_def, void *argument) {

}




int32_t osSignalSet (osThreadId thread_id, int32_t signals) {

}

int32_t osSignalClear (osThreadId thread_id, int32_t signals) {
}

osEvent osSignalWait (int32_t signals, uint32_t millisec) {
}


osTimerId osTimerCreate (const osTimerDef_t *timer_def, os_timer_type type, void *argument) {

}


osMutexId osMutexCreate (const osMutexDef_t *mutex_def) {

}



osSemaphoreId osSemaphoreCreate (const osSemaphoreDef_t *semaphore_def, int32_t count) {

}

int32_t osSemaphoreWait (osSemaphoreId semaphore_id, uint32_t millisec) {
}


osPoolId osPoolCreate (const osPoolDef_t *pool_def) {

}

void *osPoolAlloc (osPoolId pool_id) {
}

void *osPoolCAlloc (osPoolId pool_id) {
}

osStatus osPoolFree (osPoolId pool_id, void *block) {
}




// Message Queue

osMessageQId osMessageCreate (const osMessageQDef_t *queue_def, osThreadId thread_id) {
}

osStatus osMessagePut (osMessageQId queue_id, uint32_t info, uint32_t millisec) {
}

osEvent osMessageGet (osMessageQId queue_id, uint32_t millisec) {
}



// Mail Queue



osMailQId osMailCreate (const osMailQDef_t *queue_def, osThreadId thread_id) {
}

void *osMailAlloc (osMailQId queue_id, uint32_t millisec) {
}

void *osMailCAlloc (osMailQId queue_id, uint32_t millisec) {
}

osStatus osMailPut (osMailQId queue_id, const void *mail) {
}

osEvent osMailGet (osMailQId queue_id, uint32_t millisec) {
}

osStatus osMailFree (osMailQId queue_id, void *mail) {
}

#endif
#endif  // K_ENABLE_CMSIS_RTOS2_SUPPORT
