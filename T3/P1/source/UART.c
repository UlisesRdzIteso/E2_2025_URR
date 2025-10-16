#include "UART.h"
#include "MK64F12.h"
#include "fsl_uart.h"
#include "fsl_port.h"
#include "NVIC.h"
#include "Bits.h"

uart_mail_box_t g_mail_box_uart_4 = {0, 0};

void init_UART()
{
    uart_config_t config;
    uint32_t uart_clock;

    CLOCK_EnableClock(kCLOCK_PortB);

    PORT_SetPinMux(PORTB, PIN16, kPORT_MuxAlt3);
    PORT_SetPinMux(PORTB, PIN17, kPORT_MuxAlt3);

    UART_GetDefaultConfig(&config);
    config.baudRate_Bps = UART_BAUDRATE;
    config.enableTx     = true;
    config.enableRx     = true;

    uart_clock = CLOCK_GetFreq(UART0_CLK_SRC);

    UART_Init(UART0, &config, uart_clock);

    /* Esta línea es importante: habilita la fuente de interrupción en el periférico UART */
    UART_EnableInterrupts(UART0, kUART_RxDataRegFullInterruptEnable | kUART_RxOverrunInterruptEnable);
}

void init_UART_4()
{
    uart_config_t config_4;
    uint32_t uart_clock_4;

    CLOCK_EnableClock(kCLOCK_PortC);

    PORT_SetPinMux(PORTC, PIN14, kPORT_MuxAlt3);
    PORT_SetPinMux(PORTC, PIN15, kPORT_MuxAlt3);

    UART_GetDefaultConfig(&config_4);
    config_4.baudRate_Bps = UART_BAUDRATE;
    config_4.enableTx     = true;
    config_4.enableRx     = true;

    uart_clock_4 = CLOCK_GetFreq(UART4_CLK_SRC);

    UART_Init(UART4, &config_4, uart_clock_4);

    UART_EnableInterrupts(UART4, kUART_RxDataRegFullInterruptEnable | kUART_RxOverrunInterruptEnable);
}

/* * SE ELIMINÓ:
 * - La función UART0_RX_TX_IRQHandler() completa.
 * - Las funciones UART0_return_num(), UART0_get() y UART0_set().
*/


/* Funciones para UART4 (se mantienen si las necesitas) */
void UART4_RX_TX_IRQHandler(void)
{
    /* If new data arrived. */
    if ((kUART_RxDataRegFullFlag | kUART_RxOverrunFlag) & UART_GetStatusFlags(UART4))
    {
        g_mail_box_uart_4.mail_box = UART_ReadByte(UART4);
        g_mail_box_uart_4.flag = true;
    }
    SDK_ISR_EXIT_BARRIER;
}

uint32_t UART4_return_num()
{
    return g_mail_box_uart_4.mail_box;
}

uint32_t UART4_get()
{
    return g_mail_box_uart_4.flag = false;
}

BooleanType UART4_set()
{
    return g_mail_box_uart_4.flag;
}
