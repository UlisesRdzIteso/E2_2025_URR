#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "event_groups.h"
#include "fsl_gpio.h"
#include "fsl_port.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include <stdio.h>
#include <string.h>
#include "LCD_nokia.h"
#include "SPI.h"
#include "alarm_clock.h"

#define SW2_PORT        PORTC
#define SW2_GPIO        GPIOC
#define SW2_PIN         6u
#define SW2_IRQ         PORTC_IRQn
#define SW2_ISR         PORTC_IRQHandler

void SW2_ISR(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Clear the interrupt flag */
    GPIO_PortClearInterruptFlags(SW2_GPIO, 1U << SW2_PIN);

    /* Set the silence bit in the event group */
    if (time_event_group != NULL) {
        xEventGroupSetBitsFromISR(time_event_group, SILENCE_BIT, &xHigherPriorityTaskWoken);
    }

    /* Trigger context switch if necessary */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void SW2_init(void)
{
    /* Enable clock for Port C */
    CLOCK_EnableClock(kCLOCK_PortC);

    /* Configure SW2 pin as GPIO */
    PORT_SetPinMux(SW2_PORT, SW2_PIN, kPORT_MuxAsGpio);

    /* Configure pin interrupt on falling edge */
    PORT_SetPinInterruptConfig(SW2_PORT, SW2_PIN, kPORT_InterruptFallingEdge);

    /* Configure GPIO as input */
    gpio_pin_config_t gpio_config = {
        .pinDirection = kGPIO_DigitalInput,
        .outputLogic = 0
    };
    GPIO_PinInit(SW2_GPIO, SW2_PIN, &gpio_config);

    /* Clear any pending interrupt flags */
    GPIO_PortClearInterruptFlags(SW2_GPIO, 1U << SW2_PIN);

    /* Enable NVIC interrupt */
    NVIC_SetPriority(SW2_IRQ, 6);
    EnableIRQ(SW2_IRQ);
}

int main(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();
    SPI_config();
    LCD_nokia_init();
    LCD_nokia_clear();
    SW2_init();
    __enable_irq();
    alarm_system_init();
    vTaskStartScheduler();

    while (1) {
        ;
    }

    return 0;
}
