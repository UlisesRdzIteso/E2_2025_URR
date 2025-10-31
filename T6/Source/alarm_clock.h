#ifndef ALARM_CLOCK_H_
#define ALARM_CLOCK_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include "timers.h"

typedef enum {
    seconds_type,
    minutes_type,
    hours_type
} time_types_t;

typedef struct {
    time_types_t time_type;
    uint8_t value;
} time_msg_t;

#define ALARM_HOURS    1
#define ALARM_MINUTES  0
#define ALARM_SECONDS  0

#define SECONDS_BIT    ( 1 << 0 )
#define MINUTES_BIT    ( 1 << 1 )
#define HOURS_BIT      ( 1 << 2 )
#define SILENCE_BIT    ( 1 << 3 )

#define ALL_TIME_BITS  ( SECONDS_BIT | MINUTES_BIT | HOURS_BIT )

#define STACK_SIZE_SMALL         ( configMINIMAL_STACK_SIZE + 50 )
#define STACK_SIZE_MEDIUM        ( configMINIMAL_STACK_SIZE + 100 )
#define STACK_SIZE_LARGE         ( configMINIMAL_STACK_SIZE + 150 )

#define TIMER_PERIOD_MS          1000

#define TIME_QUEUE_SIZE          10

extern QueueHandle_t time_queue;
extern SemaphoreHandle_t seconds_semaphore;
extern SemaphoreHandle_t minutes_semaphore;
extern SemaphoreHandle_t hours_semaphore;
extern EventGroupHandle_t time_event_group;
extern TimerHandle_t seconds_timer;

void alarm_system_init(void);
void seconds_task(void *pvParameters);
void minutes_task(void *pvParameters);
void hours_task(void *pvParameters);
void print_task(void *pvParameters);
void alarm_task(void *pvParameters);
void seconds_timer_callback(TimerHandle_t xTimer);

#endif /* ALARM_CLOCK_H_ */
