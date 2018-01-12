/**
 * 							ULIPE RTOS PICO
 *
 *  @file arch.c
 *
 *  @brief specific machine / arch code to for ulipe kernel
 *
 *
 */

#include "../../include/ext/cmsis/CMSIS/Core/Include/cmsis_gcc.h"
#include "../../include/ext/cmsis/CMSIS/Core/Include/core_cm7.h"

/* organize the exception interrupts priorities*/
#define K_SVC_PRIO			0xEF
#define K_PENDSV_PRIO		0xFF
#define K_TICKER_PRIO		0x8F

/* priorities register offset */
#define K_SVC_OFFSET		7
#define K_PENDSV_OFFSET		10
#define K_SYSTICK_OFFSET	11


static uint32_t irq_lock_nest = 0;


archtype_t *port_create_stack_frame(archtype_t *stack, thread_t thr_func, void *arg)
{

	archtype_t *ptr = stack;
	
	ULIPE_ASSERT(stack != NULL);
	ULIPE_ASSERT(thr_func != NULL);

    /* saved automatically by hardware */
    *--ptr = 0x01000000;
    *--ptr = (uint32_t)thr_func;
    *--ptr = 0xFFFFFFFD;
    *--ptr = 0x12121212;
    *--ptr = (uint32_t)arg;    
    *--ptr = 0x01010101;
    *--ptr = 0x02020202;
    *--ptr = 0x03030303;

    /* saved by the context switch engine */
    *--ptr = 0xCAFEBEEF;
    *--ptr = 0xCAFEBEEF;
    *--ptr = 0xCAFEBEEF;
    *--ptr = 0xCAFEBEEF;
    *--ptr = 0xCAFEBEEF;
    *--ptr = 0xCAFEBEEF;
    *--ptr = 0xCAFEBEEF;
    *--ptr = 0xCAFEBEEF;
    
	return(ptr);
}

archtype_t port_irq_lock(void)
{
	if(!irq_lock_nest)
		__disable_irq();
	
	if(irq_lock_nest < 0xFFFFFFFF)
		irq_lock_nest++;	

	return 0;
}

void port_irq_unlock(archtype_t key)
{
	if(irq_lock_nest) {
		irq_lock_nest--;
		if(!irq_lock_nest)
			__enable_irq();		
	}
}


bool port_from_isr()
{
	return(__get_IPSR() > 0 ? true : false);
}


void port_swap_req(void)
{
	/* interrupts must bee enabled here! */
	if(irq_lock_nest)
		return;
	
	if(!k_running)
		return;
	
	SCB->ICSR |= (1 << 28);
}


void port_init_machine(void)
{
	/* configure systick counting */
    SysTick->CTRL = 0;
	SysTick->LOAD = K_MACHINE_CLOCK/K_TICKER_RATE; 
	
	/* configure the stack to be 8 byte aligned */
	SCB->CCR = 0x200;
	
	/* setup the kernel exception interrupt */
    SCB->SHP[K_SVC_OFFSET] =  K_SVC_OFFSET;	
	SCB->SHP[K_PENDSV_OFFSET] = K_PENDSV_PRIO;
	SCB->SHP[K_SYSTICK_OFFSET] = K_TICKER_PRIO;
}

void port_start_kernel(void)
{
	/* enable the tick counting */
	SysTick->CTRL = 0x07;

	__asm volatile (
		"   movs r0, #2	                \n"
		"	msr	 control, r0			\n"
		"   movs r0, #0	                \n"
		"	msr	 primask, r0			\n"
		"   cpsie  I                    \n"
		"   svc  #0						\n"
		"   nop							\n"		
 	);

	/* should never return from start */
	ULIPE_ASSERT(0);
}


void port_set_break(void)
{
}


#if(K_ENABLE_TICKLESS_IDLE > 0)
void port_start_ticker(uint32_t reload_val)
{

}


uint32_t port_halt_ticker(void)
{

}

#endif

#if(K_ENABLE_TIMERS > 0)

void port_start_timer(archtype_t reload_val)
{
}

void port_timer_load_append(archtype_t append_val)
{
}

uint32_t port_timer_halt(void)
{
	return(0);
}

void port_timer_resume(void)
{
}


void timer_match_handler(void)
{
}

void timer_ovf_handler(void)
{
}


#endif

uint8_t port_bit_fs_scan(archtype_t x)
{
	/* clz not implemented for this architecture */
	uint8_t ret = 32;

	if(!x)
		goto cleanup;


	static uint8_t const clz_lkup[] = {
		32, 31, 30, 30, 29, 29, 29, 29,
		28, 28, 28, 28, 28, 28, 28, 28
	};

    uint32_t n;
	
    /*
     * Scan if bit is in top word
     */
    if (x >= (1 << 16)) {
		if (x >= (1 << 24)) {
			if (x >= (1 << 28)) {
				n = 28;
			}
			else {
				n = 24;
			}
		}
		else {
			if (x >= (1 << 20)) {
				n = 20;
			}
			else {
				n = 16;
			}
		}
    }
    else {
        /* now scan if the bit is on rightmost byte */
		if (x >= (1 << 8)) {
			if (x >= (1 << 12)) {
				n = 12;
			}
			else {
				n = 8;
			}
		}
        else {
            if (x >= (1 << 4)) {
                n = 4;
            }
            else {
                n = 0;
            }
        }
    }

	ret = (uint8_t)(clz_lkup[x >> n] - n);
cleanup:
	return(ret);
}

uint8_t port_bit_ls_scan(archtype_t arg)
{
	/* ctz is not as well */
	return(31 - port_bit_fs_scan(arg & -arg));
}



/* cpu specifics exceptions */
void SVC_Handler(void)  __attribute__ (( naked ))
{
    __asm volatile (
        "   cpsid I                     	\n"		
		"   isb                         	\n"
		"   dsb                         	\n"
        "   ldr   r0, =__hpt      			\n"
        "   ldr   r1, =__cpt              	\n"
		"   ldr   r2, [r0]             		\n"
		"   ldr   r2, [r2]             		\n"		
        "   ldmia r2!, {r4 - r11, lr}   	\n"
        "   tst  lr, #0x10              	\n"
        "   it eq                       	\n"
        "   vldmiaeq r2!, {s16-s31}     	\n"        
        "   msr psp, r2                  	\n"
		"   ldr	r0, [r0]                  	\n"
		"   str r0. [r1]					\n"		
		"	ldr r0, =__run					\n"
		"	ldrb r1, #1						\n"
		"	strb r1, [r0]					\n"
		"   orrs lr, lr, #4              	\n" 
		"	cpsie I							\n"   
        "   bx lr                        	\n"
        "   .align 4                     	\n"
		"   __hpt: .word k_high_prio_task	\n"
		"   __cpt: .word k_current_task		\n"
		"   __run: .word k_running			\n"		
    );
}

void PendSV_Handler(void) __attribute__ (( naked ))
{
    __asm volatile (
        "   cpsid I                     	\n"		
		"   isb                         	\n"
		"   dsb                         	\n"
        "   ldr   r0, =__hpt      			\n"
        "   ldr   r1, =__cpt              	\n"
		"   ldr   r2, [r1]             		\n"
		"   ldr   r3, [r2]             		\n"		
        "   mrs   r3, psp                  	\n"		
        "   tst  lr, #0x10              	\n"
        "   it eq                       	\n"
        "   vstmdbeq r3!, {s16-s31}     	\n"        
		"   stmdb r3!, {r4 - r11, lr}   	\n"
		"	str   r3, [r2]					\n"
		"   ldr   r2, [r0]             		\n"
		"   ldr   r2, [r2]             		\n"		
        "   ldmia r2!, {r4 - r11, lr}   	\n"
        "   tst  lr, #0x10              	\n"
        "   it eq                       	\n"
        "   vldmiaeq r2!, {s16-s31}     	\n"        
        "   msr psp, r2                  	\n"
		"   str r0, [r1]					\n"		
		"   orrs lr, lr, #4              	\n" 
		"	cpsie I							\n"   
        "   bx lr                        	\n"
        "   .align 4                     	\n"
		"   __hpt: .word k_high_prio_task	\n"
		"   __cpt: .word k_current_task		\n"
    );
}


#if (K_ENABLE_TICKER > 0)	

void SysTick_Handler(void)
{

	kernel_irq_in();	
	timer_tick_handler();
	kernel_irq_out();

}

#endif	