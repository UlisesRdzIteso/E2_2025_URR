#include "alarm_clock.h"
#include "LCD_nokia.h"
#include "fsl_gpio.h"
#include "fsl_port.h"
#include "board.h"
#include <stdio.h>

QueueHandle_t time_queue = NULL;
SemaphoreHandle_t seconds_semaphore = NULL;
SemaphoreHandle_t minutes_semaphore = NULL;
SemaphoreHandle_t hours_semaphore = NULL;
EventGroupHandle_t time_event_group = NULL;
TimerHandle_t seconds_timer = NULL;

static SemaphoreHandle_t lcd_mutex = NULL;
static volatile uint8_t alarm_active = 0;

void seconds_timer_callback(TimerHandle_t xTimer)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Release the seconds semaphore from ISR context */
    xSemaphoreGiveFromISR(seconds_semaphore, &xHigherPriorityTaskWoken);

    /* Context switch if necessary */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void seconds_task(void *pvParameters)
{
    uint8_t seconds = 59;
    time_msg_t time_msg;

    for (;;) {
        /* Wait for seconds semaphore to be released by timer callback */
        xSemaphoreTake(seconds_semaphore, portMAX_DELAY);

        seconds++;

        if (seconds >= 60) {
            seconds = 0;
            /* Release minutes semaphore to trigger minutes task */
            xSemaphoreGive(minutes_semaphore);
        }

        /* Check if current seconds match alarm seconds */
        if (seconds == ALARM_SECONDS) {
            xEventGroupSetBits(time_event_group, SECONDS_BIT);
        } else if (seconds != ALARM_SECONDS) {
            xEventGroupClearBits(time_event_group, SECONDS_BIT);
        }

        time_msg.time_type = seconds_type;
        time_msg.value = seconds;

        xQueueSend(time_queue, &time_msg, portMAX_DELAY);
    }
}

void minutes_task(void *pvParameters)
{
    uint8_t minutes = 58;
    time_msg_t time_msg;

    for (;;) {
        /* Wait for minutes semaphore to be released by seconds task */
        xSemaphoreTake(minutes_semaphore, portMAX_DELAY);

        minutes++;

        if (minutes >= 60) {
            minutes = 0;
            /* Release hours semaphore to trigger hours task */
            xSemaphoreGive(hours_semaphore);
        }

        /* Check if current minutes match alarm minutes */
        if (minutes == ALARM_MINUTES) {
            xEventGroupSetBits(time_event_group, MINUTES_BIT);
        } else if (minutes != ALARM_MINUTES) {
            xEventGroupClearBits(time_event_group, MINUTES_BIT);
        }

        time_msg.time_type = minutes_type;
        time_msg.value = minutes;

        xQueueSend(time_queue, &time_msg, portMAX_DELAY);
    }
}

void hours_task(void *pvParameters)
{
    uint8_t hours = 23;
    time_msg_t time_msg;

    for (;;) {
        /* Wait for hours semaphore to be released by minutes task */
        xSemaphoreTake(hours_semaphore, portMAX_DELAY);

        hours++;

        if (hours >= 24) {
            hours = 0;
        }

        /* Check if current hours match alarm hours */
        if (hours == ALARM_HOURS) {
            xEventGroupSetBits(time_event_group, HOURS_BIT);
        } else if (hours != ALARM_HOURS) {
            xEventGroupClearBits(time_event_group, HOURS_BIT);
        }

        time_msg.time_type = hours_type;
        time_msg.value = hours;

        xQueueSend(time_queue, &time_msg, portMAX_DELAY);
    }
}

void print_task(void *pvParameters)
{
    time_msg_t received_msg;
    static uint8_t current_hours = 0;
    static uint8_t current_minutes = 0;
    static uint8_t current_seconds = 0;
    char time_str[16];

    for (;;) {
        /* Wait for message from queue */
        if (xQueueReceive(time_queue, &received_msg, portMAX_DELAY) == pdTRUE) {

            /* Update local time based on message type */
            switch (received_msg.time_type) {
                case seconds_type:
                    current_seconds = received_msg.value;
                    break;
                case minutes_type:
                    current_minutes = received_msg.value;
                    break;
                case hours_type:
                    current_hours = received_msg.value;
                    break;
                default:
                    break;
            }

            if (xSemaphoreTake(lcd_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {

                LCD_nokia_clear_range_FrameBuffer(0, 0, 84);
                LCD_nokia_clear_range_FrameBuffer(0, 1, 84);

                sprintf(time_str, "%02d:%02d:%02d", current_hours, current_minutes, current_seconds);

                LCD_nokia_write_string_xy_FB(15, 0, (uint8_t *)time_str, 8);

                LCD_nokia_sent_FrameBuffer();

                xSemaphoreGive(lcd_mutex);
            }
        }
    }
}

void alarm_task(void *pvParameters)
{
    EventBits_t uxBits;
    char alarm_str[] = "ALARM!!!";

    for (;;) {
        uxBits = xEventGroupWaitBits(
            time_event_group, ALL_TIME_BITS, pdFALSE, pdTRUE, pdMS_TO_TICKS(100));

        /* If all bits are set, trigger alarm */
        if ((uxBits & ALL_TIME_BITS) == ALL_TIME_BITS) {
            alarm_active = 1;

            if (xSemaphoreTake(lcd_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {

                LCD_nokia_clear_range_FrameBuffer(0, 2, 84);
                LCD_nokia_clear_range_FrameBuffer(0, 3, 84);

                LCD_nokia_write_string_xy_FB(10, 2, (uint8_t *)alarm_str, 8);
                LCD_nokia_write_string_xy_FB(5, 3, (uint8_t *)"Press SW2", 9);

                LCD_nokia_sent_FrameBuffer();

                xSemaphoreGive(lcd_mutex);
            }

            /* Wait for silence bit to be set (SW2 press) with timeout */
            while (alarm_active) {
                uxBits = xEventGroupWaitBits(time_event_group, SILENCE_BIT, pdTRUE, pdFALSE, pdMS_TO_TICKS(100));

                if ((uxBits & SILENCE_BIT) == SILENCE_BIT) {
                    break;
                }
            }

            /* Clear all time bits to stop alarm and reset for next alarm */
            xEventGroupClearBits(time_event_group, ALL_TIME_BITS);
            alarm_active = 0;

            /* Clear alarm message from display */
            if (xSemaphoreTake(lcd_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {

                LCD_nokia_clear_range_FrameBuffer(0, 2, 84);
                LCD_nokia_clear_range_FrameBuffer(0, 3, 84);
                LCD_nokia_sent_FrameBuffer();

                xSemaphoreGive(lcd_mutex);
            }
        }
    }
}

void GPIO_Button_ISR(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (alarm_active) {
        /* Set the silence bit from ISR context */
        xEventGroupSetBitsFromISR(time_event_group, SILENCE_BIT, &xHigherPriorityTaskWoken);

        /* Context switch if necessary */
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void alarm_system_init(void)
{
    seconds_semaphore = xSemaphoreCreateBinary();
    if (seconds_semaphore != NULL) {
        xSemaphoreGive(seconds_semaphore);
    }

    minutes_semaphore = xSemaphoreCreateBinary();
    if (minutes_semaphore != NULL) {
        xSemaphoreGive(minutes_semaphore);
    }

    hours_semaphore = xSemaphoreCreateBinary();
    if (hours_semaphore != NULL) {
        xSemaphoreGive(hours_semaphore);
    }

    lcd_mutex = xSemaphoreCreateMutex();

    time_event_group = xEventGroupCreate();

    time_queue = xQueueCreate(TIME_QUEUE_SIZE, sizeof(time_msg_t));

    seconds_timer = xTimerCreate("Seconds Timer", pdMS_TO_TICKS(TIMER_PERIOD_MS), pdTRUE, NULL, seconds_timer_callback);

    xTaskCreate(seconds_task, "Seconds Task", STACK_SIZE_SMALL, NULL, 1, NULL);

    xTaskCreate(minutes_task, "Minutes Task", STACK_SIZE_SMALL, NULL, 1, NULL);

    xTaskCreate(hours_task, "Hours Task", STACK_SIZE_SMALL, NULL, 1, NULL);

    xTaskCreate(print_task, "Print Task", STACK_SIZE_MEDIUM, NULL, 2, NULL);

    xTaskCreate(alarm_task, "Alarm Task", STACK_SIZE_MEDIUM, NULL, 3, NULL);

    xTimerStart(seconds_timer, portMAX_DELAY);
}
