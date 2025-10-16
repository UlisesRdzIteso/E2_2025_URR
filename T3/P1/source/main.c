#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_uart.h"
#include "UART.h"
#include "NVIC.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define DEMO_UART UART0
#define DEMO_UART_IRQn UART0_RX_TX_IRQn
#define DEMO_UART_IRQHandler UART0_RX_TX_IRQHandler

#define DEMO_RING_BUFFER_SIZE 16
#define RX_TASK_PRIORITY (configMAX_PRIORITIES - 1)

static void rx_echo_task(void *pvParameters);

uint8_t g_tipString[] =
    "UART Echo.\r\n"
    "Escribe un caracter.\r\n";

uint8_t demoRingBuffer[DEMO_RING_BUFFER_SIZE];
volatile uint16_t txIndex = 0;
volatile uint16_t rxIndex = 0;
SemaphoreHandle_t xUartSemaphore;

void DEMO_UART_IRQHandler(void)
{
    uint8_t data;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if ((kUART_RxDataRegFullFlag | kUART_RxOverrunFlag) & UART_GetStatusFlags(DEMO_UART))
    {
        data = UART_ReadByte(DEMO_UART);

        if (((rxIndex + 1) % DEMO_RING_BUFFER_SIZE) != txIndex)
        {
            demoRingBuffer[rxIndex] = data;
            rxIndex = (rxIndex + 1) % DEMO_RING_BUFFER_SIZE;
            xSemaphoreGiveFromISR(xUartSemaphore, &xHigherPriorityTaskWoken);
        }
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    SDK_ISR_EXIT_BARRIER;
}

static void rx_echo_task(void *pvParameters)
{
    uint8_t ch;
    UART_WriteBlocking(DEMO_UART, g_tipString, sizeof(g_tipString) - 1);

    for (;;)
    {
        if (xSemaphoreTake(xUartSemaphore, portMAX_DELAY) == pdPASS)
        {
            ch = demoRingBuffer[txIndex];
            txIndex = (txIndex + 1) % DEMO_RING_BUFFER_SIZE;
            UART_WriteBlocking(DEMO_UART, &ch, 1);
        }
    }
}

int main(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    init_UART();

    NVIC_enable_interrupt_and_priotity(UART0_IRQ, PRIORITY_5);

    xUartSemaphore = xSemaphoreCreateBinary();
    if (xUartSemaphore == NULL)
    {
        for (;;);
    }

    xTaskCreate(rx_echo_task, "RX_Echo_Task", configMINIMAL_STACK_SIZE + 100, NULL, RX_TASK_PRIORITY, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
