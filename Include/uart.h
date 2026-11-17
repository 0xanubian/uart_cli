#include <stdint.h>

#define MAX_SIZE 128
#define MAX_SIZE_MSK (MAX_SIZE - 1)

void uart_puts(const char *str);
void uart_init();
void USART1_IRQHandler();
void uart_putc(char ch);
void uart_puts(const char *str);
int uart_gets(char line[]);
