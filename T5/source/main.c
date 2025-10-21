#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Definición de la estructura del mensaje */
typedef struct {
    uint8_t thread_id;      /* ID del thread (0, 1, 2) */
    uint16_t data;          /* Datos de 16 bits */
} Message_t;

/* Handler de la cola de mensajes */
QueueHandle_t xMessageQueue;

/* Prototipos de tareas */
void Tx0_Task(void *pvParameters);
void Tx1_Task(void *pvParameters);
void Tx2_Task(void *pvParameters);
void Rx_Task(void *pvParameters);

/* Tx0_Task: Envía ID=0 con datos incrementando de 0 a 65535 */
void Tx0_Task(void *pvParameters) {
    Message_t msg;
    msg.thread_id = 0;

    for (uint16_t i = 0; i <= 65535; i++) {
        msg.data = i;

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelete(NULL);
}

/* Tx1_Task: Envía ID=1 con datos decrementando de 65535 a 0 */
void Tx1_Task(void *pvParameters) {
    Message_t msg;
    msg.thread_id = 1;

    for (int32_t i = 65535; i >= 0; i--) {
        msg.data = (uint16_t)i;

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelete(NULL);
}

/* Tx2_Task: Envía ID=2 solo con números pares de 0 a 65535 */
void Tx2_Task(void *pvParameters) {
    Message_t msg;
    msg.thread_id = 2;

    for (uint16_t i = 0; i <= 65535; i += 2) {  /* Incrementa de 2 en 2 */
        msg.data = i;

        vTaskDelay(pdMS_TO_TICKS(20));
    }

    vTaskDelete(NULL);
}

/* Rx_Task: Recibe mensajes de la cola y los despliega identificando el thread */
void Rx_Task(void *pvParameters) {
    Message_t rxMsg;

    while (1) {
        /* Recibir mensaje de la cola */
        if (xQueueReceive(xMessageQueue, &rxMsg, portMAX_DELAY) == pdTRUE) {
            /* Desplegar mensaje según el ID del thread */
            PRINTF("Datos recibidos del Th%d = %d\r\n", rxMsg.thread_id, rxMsg.data);
        }
    }
}

int main(void) {

    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    /* Crear la cola de mensajes */
    xMessageQueue = xQueueCreate(10, sizeof(Message_t));

    BaseType_t xReturn;

    /* Crear Tx0_Task con prioridad 2 */
    xReturn = xTaskCreate(Tx0_Task, "Tx0", configMINIMAL_STACK_SIZE + 100, NULL, 2, NULL);

    /* Crear Tx1_Task con prioridad 3 */
    xReturn = xTaskCreate(Tx1_Task, "Tx1", configMINIMAL_STACK_SIZE + 100, NULL, 3, NULL);

    /* Crear Tx2_Task con prioridad 1 */
    xReturn = xTaskCreate(Tx2_Task, "Tx2", configMINIMAL_STACK_SIZE + 100, NULL, 1, NULL);

    /* Crear Rx_Task con prioridad 4 */
    xReturn = xTaskCreate(Rx_Task, "Rx", configMINIMAL_STACK_SIZE + 100, NULL, 4, NULL);

    /* Iniciar scheduler */
    vTaskStartScheduler();

    for (;;) {
        ;
    }

    return 0;
}
