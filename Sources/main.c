#include "stm32f4xx.h"
#include "uart.h"
#include "command.h"
#include "init.h"

int main(void)
{
    clock_init();
    uart_init();
    led_init();

    char line[MAX_SIZE];
    char *msg = "uart cli.... enter your command\r\n";

    /* Loop forever */
    uart_puts(msg);
    uart_puts("-> ");
	while (1) {
        if (uart_gets(line)) {
            struct command cmd;
            if (!tokenize(line, &cmd)) {
                uart_puts("Max args exceeded. only 6 args are permitted\r\n");
                continue;
            }

            //dbg_print_args(cmd);

            command_dispatch(cmd);
            uart_puts("-> ");
        }
    }
    return 0;
}
