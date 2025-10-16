#include "NVIC.h"

void NVIC_enable_interrupt_and_priotity(interrupt_t interrupt_number, priority_level_t priority)
{
	/**This functions are part of CMSIS Core functions*/
	/**It enables the IRQ*/
	NVIC_EnableIRQ(interrupt_number);
	/**It Sets the priority of the IRQ*/
	NVIC_SetPriority(interrupt_number, priority);
}


void NVIC_set_basepri_threshold(priority_level_t priority)
{
	/**Sets the threshold level to attend interrupts*/
	/**A shift is needed to align in a correct manner the data in priority inside BASEPRI register*/
	__set_BASEPRI(priority << (8 - __NVIC_PRIO_BITS));
}

void NVIC_init(void)
{
	NVIC_set_basepri_threshold(PRIORITY_10);
	NVIC_enable_interrupt_and_priotity(PORTD_IRQ,PRIORITY_3);

    NVIC_enable_interrupt_and_priotity(UART0_IRQ, PRIORITY_1);
    NVIC_global_enable_interrupts;
}
