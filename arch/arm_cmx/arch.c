/**
 * 							ULIPE RTOS PICO
 *
 *  @file k_port_cm3_4_7.c
 *
 *  @brief specific cortex M3+ system control block and timer file
 *
 *
 */


archtype_t *port_create_stack_frame(archtype_t *stack, thread_t thr_func, void *cookie)
{
	 arm_cm3_xcpt_contents_t *ptr = ((arm_cm3_xcpt_contents_t *)stack) - 1;

	ptr->lr = 0xFFFFFFFD;				//Adds exec return on link reg
	ptr->pc = (uint32_t)thr_func;		//task function at pc
	ptr->xpsr = 0x01000000;				//xPsr default value with interrupts enabled
	ptr->r0  = (uint32_t)cookie;		//Task arguments are passed thru R0

	/*Fill some known values at stack:*/
	ptr->r1 = 0x11111111;
	ptr->r2 = 0x22222222;
	ptr->r3 = 0x33333333;
	ptr->r4 = 0x44444444;
	ptr->r5 = 0x55555555;
	ptr->r6 = 0x66666666;
	ptr->r7 = 0x77777777;
	ptr->r8 = 0x88888888;
	ptr->r9 = 0x99999999;
	ptr->r10= 0xAAAAAAAA;
	ptr->r11= 0xBBBBBBBB;
	ptr->r12 =0xCCCCCCCC;

	return((archtype_t *)ptr);
}

void port_swap_req(void)
{
	/* triggers a sofware interrupt */
	SCB->ICSR |= (1 << 28);
}

void port_init_machine(void)
{
	/* sets the system interrupts on system control block */
	SCB->CCR = 0x200;
	SCB->SHP[10] = 0xFE;
	SCB->SHP[11] = 0xFF;
	SCB->SHP[7]  = 0xFF;

#if (K_ENABLE_TICKER > 0)
	#if !defined(K_MACHINE_CLOCK) || !defined(K_TICKER_RATE)
	#error "The SoC clock and ticker rate needs to be defined to use tick feature!"
	#endif

	SysTick->CTRL = 0;
	SysTick->LOAD = (K_MACHINE_CLOCK / K_TICKER_RATE);
	SysTick->CTRL = 0x07;
#endif
}

#if(K_ENABLE_TICKLESS_IDLE > 0)
void port_start_ticker(uint32_t reload_val)
{

}


uint32_t port_halt_ticker(void)
{

}

void port_low_power_engine(k_wakeup_info_t *info)
{

}

#endif



#if (K_ENABLE_TICKER > 0)

void timer_tick_handler(void)
{
	extern tcb_t timer_tcb;
	thread_set_signals(&timer_tcb, K_TIMER_TICK);	
}


void SysTick_Handler(void)
{
	kernel_irq_in();	
	timer_tick_handler();
	kernel_irq_out();
}
#endif


#if(K_ENABLE_TIMERS > 0)

void port_start_timer(archtype_t reload_val)
{
}

void port_timer_load_append(archtype_t append_val)
{
}

extern uint32_t port_timer_halt(void)
{
	return 0;
}

extern void port_timer_resume(void)
{
}


extern void timer_match_handler(void)
{
	extern tcb_t timer_tcb;
	kernel_irq_in();
	/* request timeline handling */
	thread_set_signals(&timer_tcb, K_TIMER_DISPATCH);
	kernel_irq_out();
}

extern void timer_ovf_handler(void)
{
	kernel_irq_in();
	kernel_irq_out();
}
