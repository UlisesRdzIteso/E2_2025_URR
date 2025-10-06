#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_uart.h"
#include "FreeRTOS.h"
#include "task.h"

#define DEMO_UART            UART0
#define DEMO_UART_CLKSRC     UART0_CLK_SRC
#define DEMO_UART_CLK_FREQ   CLOCK_GetFreq(UART0_CLK_SRC)
#define DEMO_UART_IRQn       UART0_RX_TX_IRQn
#define DEMO_UART_IRQHandler UART0_RX_TX_IRQHandler

#define DEMO_RING_BUFFER_SIZE 16

void Th0(void *pvParameters);
void Th1(void *pvParameters);
void Th2(void *pvParameters);

uint8_t demoRingBuffer[DEMO_RING_BUFFER_SIZE];
volatile uint16_t txIndex;
volatile uint16_t rxIndex;

void DEMO_UART_IRQHandler(void)
{
    uint8_t data;

    if ((kUART_RxDataRegFullFlag | kUART_RxOverrunFlag) & UART_GetStatusFlags(DEMO_UART))
    {
        data = UART_ReadByte(DEMO_UART);

        if (((rxIndex + 1) % DEMO_RING_BUFFER_SIZE) != txIndex)
        {
            demoRingBuffer[rxIndex] = data;
            rxIndex++;
            rxIndex %= DEMO_RING_BUFFER_SIZE;
        }
    }
    SDK_ISR_EXIT_BARRIER;
}

int main(void)
{
    uart_config_t config;

    BOARD_InitBootPins();
    BOARD_InitBootClocks();

    UART_GetDefaultConfig(&config);
    config.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;
    config.enableTx     = true;
    config.enableRx     = true;

    UART_Init(DEMO_UART, &config, DEMO_UART_CLK_FREQ);
    UART_EnableInterrupts(DEMO_UART, kUART_RxDataRegFullInterruptEnable | kUART_RxOverrunInterruptEnable);
    EnableIRQ(DEMO_UART_IRQn);

    xTaskCreate(Th0, "Thread 0", 1024, NULL, 1, NULL);
    xTaskCreate(Th1, "Thread 1", 1024, NULL, 2, NULL);
    xTaskCreate(Th2, "Thread 2", 1024, NULL, 3, NULL);

    vTaskStartScheduler();
}

void Th0(void *pvParameters){
	while(1){
		UART_WriteBlocking(DEMO_UART, "Hola desde el Thread 0\n\r", 25);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void Th1(void *pvParameters){
	while(1){
		UART_WriteBlocking(DEMO_UART, "Hola desde el Thread 1\n\r", 25);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void Th2(void *pvParameters){
	while(1){
		UART_WriteBlocking(DEMO_UART, "Hola desde el Thread 2\n\r", 25);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
