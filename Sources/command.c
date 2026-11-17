#include <string.h>

#include "stm32f4xx.h"
#include "command.h"
#include "uart.h"

// table for command dispatch and iteration
const struct cmd_entry handler_table[] = {
    {"help", "print the help message", help_handler},
    {"led", "turn the internal led on or off", led_handler},
    {"echo", "echo or print a word", echo_handler},
    {"EXTI_btn_pressed", "press the button on the breadtboard to increase its counter", btn_handler},
    {"uptime", "prints the time since which the program has been running", uptime_handler},
    {"adc", "does something with adc.... to be implemented", adc_handler},
};

// no of commands
const uint16_t NUM_CMDS = sizeof(handler_table)/sizeof(handler_table[0]);

// handler for help command
int help_handler(int argc, char *argv[]) {
    // iterate over all the commands in table and print its help msg
    for (int i = 0; i < NUM_CMDS; i++) {
        uart_puts(handler_table[i].name);
        uart_puts(" : ");
        uart_puts(handler_table[i].help_msg);
        uart_puts("\r\n");
    }

    return 0;
}

// handler for led command
int led_handler(int argc, char *argv[]) {
    if (argc < 2 || argc > 2) {
        uart_puts("too many or too few arguments. USAGE: led on or led off");
        return 1;
    }

    // on command
    if (strcmp(argv[1], "on") == 0) {
        GPIOC->BSRR = GPIO_BSRR_BR13;   // resrts the pin thus turning it on
        return 0;
    }

    // off command
    else if (strcmp(argv[1], "off") == 0) {
        GPIOC->BSRR = GPIO_BSRR_BS13;   // sets the pin thus turning it off
        return 0;
    }
    
    // status command
    else if (strcmp(argv[1], "status") == 0) {
        // checks the ODR register to get its status
        uint32_t status = GPIOC->ODR & GPIO_ODR_OD13_Msk;
        if (status) uart_puts("led is turned off\r\n");
        else uart_puts("led is turned on\r\n");
        return 0;
    }

    else {
        uart_puts("invalid command\r\n");
        return 1;
    }
}

// handler of echo command. it only prints one argment so you cannot have a 
// space separated string
int echo_handler(int argc, char *argv[]) {
    if (argc > 2) {
        uart_puts("you can only echo one word at a time\r\n");
        return 1;
    }

    if (argc < 2) {
        uart_puts("please enter a word to echo\r\n");
        return 1;
    }

    uart_puts(argv[1]);
    uart_puts("\r\n");
    return 0;
}

// handler for EXTI_btn
int btn_handler(int argc, char *argv[]) {
    uart_puts("sorry... to be implemented\r\n");
    return 0;
}

// handler for uptime command
int uptime_handler(int argc, char *argv[]) {
    uart_puts("sorry... to be implemented\r\n");
    return 0;
}

// handler for adc command
int adc_handler(int argc, char *argv[]) {
    uart_puts("sorry... to be implemented\r\n");
    return 0;
}

// tokenize the line entered by user and fills the command struct
int tokenize(char *line, struct command *cmd) {
    uint32_t argc = 0;
    char in_token_flag = 0;

    cmd->argv[argc] = line;

    while (*line) {
        if (argc > MAX_ARGC) return 0;

        // breaks the line and create a token if it encounters a space or tab
        if (*line == ' ' || *line == '\t') {
            if (in_token_flag) {
                *line = '\0';
                in_token_flag = 0;
            }
        }

        // increments the argc and fills a argv entry if a new token if found
        else if (!in_token_flag) {
            cmd->argv[argc++] = line;
            in_token_flag = 1;
        }

        line++;
    }

    cmd->argc = argc;

    return 1;
}

// dispatces the command if the user entered data matches with a command
void command_dispatch(struct command cmd) {
    if (cmd.argc == 0) {
        uart_puts("too few arguments... type help for more info\r\n");
        return;
    }
    for (int i = 0; i < NUM_CMDS; i++) {
        if (strcmp(cmd.argv[0], handler_table[i].name) == 0) {
            handler_table[i].handler(cmd.argc, cmd.argv);
            return;
        } 
    }

    uart_puts("Invalid command... type help for more info\r\n");
    return;
}
