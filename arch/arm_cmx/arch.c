/**
 * 							ULIPE RTOS PICO
 *
 *  @file arch.c
 *
 *  @brief specific machine / arch code to for ulipe kernel
 *
 *
 */


archtype_t *port_create_stack_frame(archtype_t *stack, thread_t thr_func, void *cookie)
{
	return(0);
}

archtype_t port_irq_lock(void)
{
	return 0;
}

void port_irq_unlock(archtype_t k)
{

}

bool port_from_isr()
{
}

void port_swap_req(void)
{
}

void port_init_machine(void)
{
}

void port_start_kernel(void)
{

}

void port_set_break(void)
{
}


#if (K_ENABLE_TICKER > 0)

void timer_tick_handler(void)
{
}

void SysTick_Handler(void)
{
	kernel_irq_in();	
	timer_tick_handler();
	kernel_irq_out();
}
#endif


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

