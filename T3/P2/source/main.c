#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_gpio.h"
#include "fsl_port.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// Definiciones de pines
#define BOARD_SW2_GPIO     GPIOC
#define BOARD_SW2_PORT     PORTC
#define BOARD_SW2_PIN      6U
#define BOARD_SW2_IRQ      PORTC_IRQn
#define BOARD_SW2_IRQ_HANDLER PORTC_IRQHandler

#define BOARD_SW3_GPIO     GPIOA
#define BOARD_SW3_PORT     PORTA
#define BOARD_SW3_PIN      4U
#define BOARD_SW3_IRQ      PORTA_IRQn
#define BOARD_SW3_IRQ_HANDLER PORTA_IRQHandler

// LED RGB
#define LED_RED_GPIO       GPIOB
#define LED_RED_PORT       PORTB
#define LED_RED_PIN        22U
#define LED_GREEN_GPIO     GPIOE
#define LED_GREEN_PORT     PORTE
#define LED_GREEN_PIN      26U
#define LED_BLUE_GPIO      GPIOB
#define LED_BLUE_PORT      PORTB
#define LED_BLUE_PIN       21U

// Configuración del estacionamiento
#define MAX_PARKING_SPACES 10
#define YELLOW_THRESHOLD   6
#define RED_THRESHOLD      0

// Declaración de funciones
void EntryTask(void *pvParameters);
void ExitTask(void *pvParameters);
void LedControlTask(void *pvParameters);
void InitGPIO(void);

// Semáforos contadores
SemaphoreHandle_t xEntrySemaphore;
SemaphoreHandle_t xExitSemaphore;
SemaphoreHandle_t xMutex;

// Variable para control de espacios disponibles
volatile int availableSpaces = MAX_PARKING_SPACES;

// ISR para SW3 (Entradas)
void BOARD_SW3_IRQ_HANDLER(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Obtener las banderas de interrupción
    uint32_t flags = GPIO_PortGetInterruptFlags(BOARD_SW3_GPIO);

    // Verificar si es el pin SW3
    if (flags & (1U << BOARD_SW3_PIN))
    {
        // Limpiar la bandera de interrupción
        GPIO_PortClearInterruptFlags(BOARD_SW3_GPIO, 1U << BOARD_SW3_PIN);

        // Señalizar entrada de vehículo
        xSemaphoreGiveFromISR(xEntrySemaphore, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    SDK_ISR_EXIT_BARRIER;
}

// ISR para SW2 (Salidas)
void BOARD_SW2_IRQ_HANDLER(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Obtener las banderas de interrupción
    uint32_t flags = GPIO_PortGetInterruptFlags(BOARD_SW2_GPIO);

    // Verificar si es el pin SW2
    if (flags & (1U << BOARD_SW2_PIN))
    {
        // Limpiar la bandera de interrupción
        GPIO_PortClearInterruptFlags(BOARD_SW2_GPIO, 1U << BOARD_SW2_PIN);

        // Señalizar salida de vehículo
        xSemaphoreGiveFromISR(xExitSemaphore, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    SDK_ISR_EXIT_BARRIER;
}

int main(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();

    // Inicializar GPIO
    InitGPIO();

    // Crear semáforos contadores (máximo 10 eventos en cola)
    xEntrySemaphore = xSemaphoreCreateCounting(10, 0);
    xExitSemaphore = xSemaphoreCreateCounting(10, 0);
    xMutex = xSemaphoreCreateMutex();

    if (xEntrySemaphore == NULL || xExitSemaphore == NULL || xMutex == NULL)
    {
        while(1);
    }

    // Crear threads
    xTaskCreate(EntryTask, "Entry Control", 256, NULL, 2, NULL);
    xTaskCreate(ExitTask, "Exit Control", 256, NULL, 2, NULL);
    xTaskCreate(LedControlTask, "LED Control", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while(1);
}

void InitGPIO(void)
{
    gpio_pin_config_t led_config = {
        kGPIO_DigitalOutput,
        1,
    };

    gpio_pin_config_t sw_config = {
        kGPIO_DigitalInput,
        0,
    };

    // Configurar LEDs RGB
    CLOCK_EnableClock(kCLOCK_PortB);
    CLOCK_EnableClock(kCLOCK_PortE);

    PORT_SetPinMux(LED_RED_PORT, LED_RED_PIN, kPORT_MuxAsGpio);
    PORT_SetPinMux(LED_GREEN_PORT, LED_GREEN_PIN, kPORT_MuxAsGpio);
    PORT_SetPinMux(LED_BLUE_PORT, LED_BLUE_PIN, kPORT_MuxAsGpio);

    GPIO_PinInit(LED_RED_GPIO, LED_RED_PIN, &led_config);
    GPIO_PinInit(LED_GREEN_GPIO, LED_GREEN_PIN, &led_config);
    GPIO_PinInit(LED_BLUE_GPIO, LED_BLUE_PIN, &led_config);

    // Configurar SW3
    CLOCK_EnableClock(kCLOCK_PortA);

    PORT_SetPinInterruptConfig(BOARD_SW3_PORT, BOARD_SW3_PIN, kPORT_InterruptFallingEdge);
    PORT_SetPinMux(BOARD_SW3_PORT, BOARD_SW3_PIN, kPORT_MuxAsGpio);
    PORT_SetPinConfig(BOARD_SW3_PORT, BOARD_SW3_PIN, &(port_pin_config_t){
        .pullSelect = kPORT_PullUp,
        .mux = kPORT_MuxAsGpio,
    });

    GPIO_PinInit(BOARD_SW3_GPIO, BOARD_SW3_PIN, &sw_config);

    NVIC_SetPriority(BOARD_SW3_IRQ, 5);
    EnableIRQ(BOARD_SW3_IRQ);

    // Configurar SW2
    CLOCK_EnableClock(kCLOCK_PortC);

    PORT_SetPinInterruptConfig(BOARD_SW2_PORT, BOARD_SW2_PIN, kPORT_InterruptFallingEdge);
    PORT_SetPinMux(BOARD_SW2_PORT, BOARD_SW2_PIN, kPORT_MuxAsGpio);
    PORT_SetPinConfig(BOARD_SW2_PORT, BOARD_SW2_PIN, &(port_pin_config_t){
        .pullSelect = kPORT_PullUp,
        .mux = kPORT_MuxAsGpio,
    });

    GPIO_PinInit(BOARD_SW2_GPIO, BOARD_SW2_PIN, &sw_config);

    NVIC_SetPriority(BOARD_SW2_IRQ, 5);
    EnableIRQ(BOARD_SW2_IRQ);
}

void EntryTask(void *pvParameters)
{
    while(1)
    {
        // Esperar señalización de entrada
        if (xSemaphoreTake(xEntrySemaphore, portMAX_DELAY) == pdTRUE)
        {
            // Proteger acceso a variable compartida
            if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE)
            {
                // Verificar si hay espacio disponible
                if (availableSpaces > 0)
                {
                    availableSpaces--;
                }

                xSemaphoreGive(xMutex);
            }

            // Delay para debounce
            vTaskDelay(pdMS_TO_TICKS(300));
        }
    }
}

void ExitTask(void *pvParameters)
{
    while(1)
    {
        // Esperar señalización de salida
        if (xSemaphoreTake(xExitSemaphore, portMAX_DELAY) == pdTRUE)
        {
            // Proteger acceso a variable compartida
            if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE)
            {
                // Verificar que no exceda el máximo
                if (availableSpaces < MAX_PARKING_SPACES)
                {
                    availableSpaces++;
                }

                xSemaphoreGive(xMutex);
            }

            // Delay para debounce
            vTaskDelay(pdMS_TO_TICKS(300));
        }
    }
}

void LedControlTask(void *pvParameters)
{
    int currentSpaces;

    while(1)
    {
        // Leer el valor actual de espacios disponibles
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE)
        {
            currentSpaces = availableSpaces;
            xSemaphoreGive(xMutex);
        }

        // Apagar todos los LEDs
        GPIO_PinWrite(LED_RED_GPIO, LED_RED_PIN, 1);
        GPIO_PinWrite(LED_GREEN_GPIO, LED_GREEN_PIN, 1);
        GPIO_PinWrite(LED_BLUE_GPIO, LED_BLUE_PIN, 1);

        if (currentSpaces >= 7 && currentSpaces <= 10)
        {
            // Verde: 7-10 espacios disponibles
            GPIO_PinWrite(LED_GREEN_GPIO, LED_GREEN_PIN, 0);
        }
        else if (currentSpaces >= 1 && currentSpaces <= 6)
        {
            // Amarillo: 1-6 espacios disponibles
            GPIO_PinWrite(LED_RED_GPIO, LED_RED_PIN, 0);
            GPIO_PinWrite(LED_GREEN_GPIO, LED_GREEN_PIN, 0);
        }
        else if (currentSpaces == 0)
        {
            // Rojo: 0 espacios disponibles
            GPIO_PinWrite(LED_RED_GPIO, LED_RED_PIN, 0);
        }

        // Actualizar cada 100ms
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
