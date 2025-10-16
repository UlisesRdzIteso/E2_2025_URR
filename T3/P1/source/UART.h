#ifndef UART_H_
#define UART_H_

#include <stdint.h>
#include "fsl_uart.h"
#include <stdbool.h>
#include "Bits.h"

#define UART_CLK_FREQ   CLOCK_GetFreq(UART0_CLK_SRC)
#define UART_BAUDRATE   115200
#define PIN16           16u
#define PIN17           17u
#define PIN14           14u
#define PIN15           15u

typedef struct{
    uint8_t flag;
    uint8_t mail_box;
} uart_mail_box_t;

void init_UART();

void init_UART_4();
uint32_t UART4_get();
uint32_t UART4_return_num();
BooleanType UART4_set();

#endif /* UART_H_ */
