/**
 * 							ULIPE RTOS PICO
 *
 *  @file k_kernel.h
 *
 *  @brief multithreading core header file
 *
 *
 */

#ifndef __K_KERNEL_H
#define __K_KERNEL_H

/**
 *  @fn k_bit_set()
 *  @brief bit set primitive
 *
 *  @param reg - word to perform bit set
 *  @param bit - bit index to set
 *
 *  @return word with newly set bit
 */
static inline archtype_t k_bit_set(archtype_t reg, uint8_t bit)
{
	return(reg |= (1 << bit));
}



/**
 *  @fn k_bit_clr()
 *  @brief bit clear primitive
 *
 *  @param reg - word to perform bit clear
 *  @param bit - bit index to clear
 *
 *  @return word with the newly cleared bit
 */
static inline archtype_t k_bit_clr(archtype_t reg, uint8_t bit)
{
	return((reg &= ~(1 << bit)));
}


/**
 *  @fn kernel_init()
 *  @brief initializes kernel to initial state, needs to be called before any other kernel api
 *
 *  @param none
 *
 *  @return k_status_ok on succesful kernel initialization
 */
k_status_t kernel_init(void);


/**
 *  @fn kernel_start()
 *  @brief starts the scheduler and gives CPU control to kernel
 *
 *  @param	none
 *
 *  @return this routine should not return, but if does it returns a error code
 */
k_status_t kernel_start(void);


/**
 *  @fn kernel_irq_in()
 *  @brief creates a rtos irq safe area needs to be called at every ISR entry before use any kernal API
 *
 *  @param none
 *
 *  @return none
 */
void kernel_irq_in(void);


/**
 *  @fn kernel_irq_out()
 *  @brief destroys a rtos irq safe area and request a scheduling, needs to be called after all apis of kernel was invoked in ISR
 *
 *  @param	none
 *
 *  @return	none
 */
void kernel_irq_out(void);


#endif
