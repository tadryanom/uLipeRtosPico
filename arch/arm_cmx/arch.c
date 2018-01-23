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


/* organize the exception interrupts priorities*/
#define K_SVC_PRIO			0xEF
#define K_PENDSV_PRIO		0xFF
#define K_TICKER_PRIO		0x8F

/* priorities register offset */
#define K_SVC_OFFSET		7
#define K_PENDSV_OFFSET		10
#define K_SYSTICK_OFFSET	11


static uint32_t irq_lock_nest = 0;


/** \brief  Structure type to access the System Timer (SysTick).
 */
typedef struct
{
  volatile uint32_t CTRL;                    /*!< Offset: 0x000 (R/W)  SysTick Control and Status Register */
  volatile uint32_t LOAD;                    /*!< Offset: 0x004 (R/W)  SysTick Reload Value Register       */
  volatile uint32_t VAL;                     /*!< Offset: 0x008 (R/W)  SysTick Current Value Register      */
  volatile  uint32_t CALIB;                   /*!< Offset: 0x00C (R/ )  SysTick Calibration Register        */
} SysTick_Type;



/** \brief  Structure type to access the System Control Block (SCB).
 */
typedef struct
{
  volatile  uint32_t CPUID;                   /*!< Offset: 0x000 (R/ )  CPUID Base Register                                   */
  volatile uint32_t ICSR;                    /*!< Offset: 0x004 (R/W)  Interrupt Control and State Register                  */
  volatile uint32_t VTOR;                    /*!< Offset: 0x008 (R/W)  Vector Table Offset Register                          */
  volatile uint32_t AIRCR;                   /*!< Offset: 0x00C (R/W)  Application Interrupt and Reset Control Register      */
  volatile uint32_t SCR;                     /*!< Offset: 0x010 (R/W)  System Control Register                               */
  volatile uint32_t CCR;                     /*!< Offset: 0x014 (R/W)  Configuration Control Register                        */
  volatile uint8_t  SHP[12];                 /*!< Offset: 0x018 (R/W)  System Handlers Priority Registers (4-7, 8-11, 12-15) */
  volatile uint32_t SHCSR;                   /*!< Offset: 0x024 (R/W)  System Handler Control and State Register             */
  volatile uint32_t CFSR;                    /*!< Offset: 0x028 (R/W)  Configurable Fault Status Register                    */
  volatile uint32_t HFSR;                    /*!< Offset: 0x02C (R/W)  HardFault Status Register                             */
  volatile uint32_t DFSR;                    /*!< Offset: 0x030 (R/W)  Debug Fault Status Register                           */
  volatile uint32_t MMFAR;                   /*!< Offset: 0x034 (R/W)  MemManage Fault Address Register                      */
  volatile uint32_t BFAR;                    /*!< Offset: 0x038 (R/W)  BusFault Address Register                             */
  volatile uint32_t AFSR;                    /*!< Offset: 0x03C (R/W)  Auxiliary Fault Status Register                       */
  volatile  uint32_t PFR[2];                  /*!< Offset: 0x040 (R/ )  Processor Feature Register                            */
  volatile  uint32_t DFR;                     /*!< Offset: 0x048 (R/ )  Debug Feature Register                                */
  volatile  uint32_t ADR;                     /*!< Offset: 0x04C (R/ )  Auxiliary Feature Register                            */
  volatile  uint32_t MMFR[4];                 /*!< Offset: 0x050 (R/ )  Memory Model Feature Register                         */
  volatile  uint32_t ISAR[5];                 /*!< Offset: 0x060 (R/ )  Instruction Set Attributes Register                   */
  volatile  uint32_t RESERVED0[5];
  volatile  uint32_t CPACR;                   /*!< Offset: 0x088 (R/W)  Coprocessor Access Control Register                   */
} SCB_Type;

#define SysTick_BASE        (0xE000E000UL +  0x0010UL)
#define SCB_BASE            (0xE000E000UL +  0x0D00UL)

#define SCB                 ((SCB_Type       *)     SCB_BASE      )   /*!< SCB configuration struct           */
#define SysTick ((SysTick_Type *) SysTick_BASE ) /*!< SysTick configuration struct       */


static archtype_t *port_create_stack_frame(archtype_t *stack, thread_t thr_func, void *arg)
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

static archtype_t port_irq_lock(void)
{
	if(!irq_lock_nest)
		__disable_irq();
	
	if(irq_lock_nest < 0xFFFFFFFF)
		irq_lock_nest++;	

	return 0;
}

static void port_irq_unlock(archtype_t key)
{
	if(irq_lock_nest) {
		irq_lock_nest--;
		if(!irq_lock_nest)
			__enable_irq();		
	}
}


static bool port_from_isr()
{
	return(__get_IPSR() > 0 ? true : false);
}


static void port_swap_req(void)
{
	/* interrupts must bee enabled here! */
	if(irq_lock_nest)
		return;
	
	if(!k_running)
		return;
	
	SCB->ICSR |= (1 << 28);
}


static void port_init_machine(void)
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

static void port_start_kernel(void)
{
	/* enable the tick counting */
	SysTick->CTRL = 0x07;

	__asm volatile (
		"   movs r0, #2	                \n"
		"	msr	 control, r0			\n"
		"   cpsie  I                    \n"
		"   svc  #0						\n"
		"   nop							\n"		
 	);

	/* should never return from start */
	ULIPE_ASSERT(0);
}


static void port_set_break(void)
{
}


#if(K_ENABLE_TICKLESS_IDLE > 0)
static void port_start_ticker(uint32_t reload_val)
{

}


static uint32_t port_halt_ticker(void)
{

}

#endif

#if(K_ENABLE_TIMERS > 0)

static void port_start_timer(archtype_t reload_val)
{
}

static void port_timer_load_append(archtype_t append_val)
{
}

static uint32_t port_timer_halt(void)
{
	return(0);
}

static void port_timer_resume(void)
{
}


static void timer_match_handler(void)
{
}

static void timer_ovf_handler(void)
{
}


#endif

static __attribute__((naked )) uint8_t port_bit_fs_scan(archtype_t x)
{
	__asm volatile(
		"clz 	r0, r0		\n"
		"bx  	lr			\n"
	);
}

static __attribute__((naked )) uint8_t port_bit_ls_scan(archtype_t arg)
{
	__asm volatile(
		"rbit 	r0, r0		\n"
		"clz 	r0, r0		\n"
		"bx  	lr			\n"
	);
}



/* cpu specifics exceptions */
void __attribute__((naked )) SVC_Handler(void)
{

    __asm volatile (
        "   cpsid I                     	\n"
		"   isb                         	\n"
		"   dsb                         	\n"
        "   ldr   r0, =k_high_prio_task     \n"
        "   ldr   r1, =k_current_task       \n"
		"   ldr   r2, [r0]             		\n"
		"   ldr   r2, [r2]             		\n"
        "   ldmia r2!, {r4 - r11} 		  	\n"
        "   msr psp, r2                  	\n"
		"   ldr	r0, [r0]                  	\n"
		"   str r0, [r1]					\n"
		"	ldr r0, =k_running				\n"
		"	movs r1, #1						\n"
		"	strb r1, [r0]					\n"
		"   orrs lr, lr, #4              	\n"
		"	cpsie I							\n"
        "   bx lr                        	\n"
    );

}

void __attribute__(( naked )) PendSV_Handler(void)
{
    __asm volatile (
        "   cpsid I                     	\n"		
		"   isb                         	\n"
		"   dsb                         	\n"
        "   ldr   r0, =k_high_prio_task     \n"
        "   ldr   r1, =k_current_task       \n"
		"   ldr   r2, [r1]             		\n"
		"   ldr   r3, [r2]             		\n"		
        "   mrs   r3, psp                  	\n"		
		"   stmdb r3!, {r4 - r11}		   	\n"
		"	str   r3, [r2]					\n"
		"   ldr   r2, [r0]             		\n"
		"   ldr   r2, [r2]             		\n"		
        "   ldmia r2!, {r4 - r11}		   	\n"
        "   msr psp, r2                  	\n"
		"   str r0, [r1]					\n"		
		"   orrs lr, lr, #4              	\n" 
		"	cpsie I							\n"   
        "   bx lr                        	\n"
    );
}


#if (K_ENABLE_TICKER > 0)	

static void SysTick_Handler(void)
{

	kernel_irq_in();	
	timer_tick_handler();
	kernel_irq_out();

}

#endif	
