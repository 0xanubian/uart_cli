#include <stdint.h>
#include <string.h>
#include "stm32f4xx.h"

#define MAX_SIZE 128
#define MAX_SIZE_MSK (MAX_SIZE - 1)
#define MAX_ARGC 6

struct command {
    uint32_t argc;
    char *argv[MAX_ARGC];
};

typedef int (*cmd_handler_t)(int argc, char *argv[]);

struct cmd_entry {
    const char *name;
    const char *help_msg;
    cmd_handler_t handler;
};

void uart_puts(const char *str);

extern const struct cmd_entry handler_table[];
extern const uint16_t NUM_CMDS;

int help_handler(int argc, char *argv[]) {
    for (int i = 0; i < NUM_CMDS; i++) {
        uart_puts(handler_table[i].name);
        uart_puts(" : ");
        uart_puts(handler_table[i].help_msg);
        uart_puts("\r\n");
    }

    return 0;
}

int led_handler(int argc, char *argv[]) {
    if (argc < 2 || argc > 2) {
        uart_puts("too many or too few arguments. USAGE: led on or led off");
        return 1;
    }

    if (strcmp(argv[1], "on") == 0) {
        GPIOC->BSRR = GPIO_BSRR_BR13;
        return 0;
    }

    else if (strcmp(argv[1], "off") == 0) {
        GPIOC->BSRR = GPIO_BSRR_BS13;
        return 0;
    }
    
    else if (strcmp(argv[1], "status") == 0) {
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

int echo_handler(int argc, char *argv[]) {
    if (argc > 2) {
        uart_puts("you can only echo one string at a time\r\n");
        return 1;
    }

    if (argc < 2) {
        uart_puts("please enter a string to echo\r\n");
        return 1;
    }

    uart_puts(argv[1]);
    uart_puts("\r\n");
    return 0;
}

int btn_handler(int argc, char *argv[]) {
    uart_puts("sorry... to be implemented\r\n");
    return 0;
}

int uptime_handler(int argc, char *argv[]) {
    uart_puts("sorry... to be implemented\r\n");
    return 0;
}

int adc_handler(int argc, char *argv[]) {
    uart_puts("sorry... to be implemented\r\n");
    return 0;
}

const struct cmd_entry handler_table[] = {
    {"help", "print the help message", help_handler},
    {"led", "turn the internal led on or off", led_handler},
    {"echo", "echo or print a line", echo_handler},
    {"EXTI_btn_pressed", "press the button on the breadtboard to increase its counter", btn_handler},
    {"uptime", "prints the time since which the program has been running", uptime_handler},
    {"adc", "does something with adc.... to be implemented", adc_handler},
};

const uint16_t NUM_CMDS = sizeof(handler_table)/sizeof(handler_table[0]);

char rx_ring_buf[MAX_SIZE];
volatile uint32_t rx_head = 0;     // write pointer
volatile uint32_t rx_tail = 0;     // read pointer

char tx_ring_buf[MAX_SIZE];
volatile uint32_t tx_head = 0;
volatile uint32_t tx_tail = 0;

static void clock_init(void)
{
    /* 1. HSE on (25 MHz crystal) */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) { }

    /* 2. Flash: 2 wait states + prefetch + caches, BEFORE raising the clock */
    FLASH->ACR = FLASH_ACR_LATENCY_2WS | FLASH_ACR_PRFTEN |
                 FLASH_ACR_ICEN | FLASH_ACR_DCEN;

    /* 3. PLL: HSE/25 = 1 MHz, x336 = 336 MHz, /4 = 84 MHz, Q = 7 */
    RCC->PLLCFGR = (25u  << RCC_PLLCFGR_PLLM_Pos) |
                   (336u << RCC_PLLCFGR_PLLN_Pos) |
                   (1u   << RCC_PLLCFGR_PLLP_Pos) |   /* 01 = /4 */
                   (7u   << RCC_PLLCFGR_PLLQ_Pos) |
                   RCC_PLLCFGR_PLLSRC_HSE;

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) { }

    /* 4. Bus prescalers: AHB /1, APB1 /2, APB2 /1 */
    RCC->CFGR = RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1;

    /* 5. Switch SYSCLK to the PLL and wait for confirmation */
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) { }

    SystemCoreClock = 84000000u;
}

static void uart_init() {
    // enable rcc clock signal for uart1 and GPIOA
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    // set GPIOA to alternate function
    GPIOA->MODER &= ~((3u << 18) | (3u << 20));
    GPIOA->MODER |= ((2u << 18) | (2u << 20));

    // USART1_TX is PA9 and AF07 and USART1_RX is PA10
    GPIOA->AFR[1] &= ~((0xfu << 4) | (0xfu << 8));
    GPIOA->AFR[1] |= ((7u << 4) | (7u << 8));

    // set the baud rate register
    USART1->BRR = (84000000u + 115200u/2) / 115200u;

    // enable uart and TX and RX and RX interrupt
    USART1->CR1 |= USART_CR1_UE | USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;

    // enable the exception handler in NVIC and set its priority
    NVIC_SetPriority(USART1_IRQn, 1);
    NVIC_EnableIRQ(USART1_IRQn);
}

void USART1_IRQHandler() {
    uint32_t sr = USART1->SR;

    // receive handler
    if (sr & (USART_SR_RXNE | USART_SR_ORE)) {
        uint8_t data = USART1->DR;
        uint32_t nxt_head = (rx_head + 1u) & MAX_SIZE_MSK;

        if (nxt_head != rx_tail) {
            rx_ring_buf[rx_head] = data;
            rx_head = nxt_head;
        }
    }

    // transmit handler
    if ((sr & USART_SR_TXE) && (USART1->CR1 & USART_CR1_TXEIE)) {
        if (tx_tail != tx_head) {
            USART1->DR = tx_ring_buf[tx_tail];
            tx_tail = (tx_tail + 1u) & MAX_SIZE_MSK;
        }
        else {
            USART1->CR1 &= ~USART_CR1_TXEIE;
        }
    }
}

void uart_putc(char ch) {
    uint32_t nxt_head = (tx_head + 1u) & MAX_SIZE_MSK;
    
    if (nxt_head != tx_tail) {
        tx_ring_buf[tx_head] = ch;
        tx_head = nxt_head;

        USART1->CR1 |= USART_CR1_TXEIE;
    }
}

void uart_puts(const char *str) {
    while (*str) {
        uart_putc(*str);
        str++;
    }
}

int uart_gets(char line[]) {
    static uint32_t idx = 0;
    static char prev = 0;

    while (rx_tail != rx_head) {
        char ch = rx_ring_buf[rx_tail];
        rx_tail = (rx_tail + 1u) & MAX_SIZE_MSK;

        if (ch == '\r' || ch == '\n') {
            if (ch == '\n' && prev == '\r') {
                prev = ch;
                continue;
            }

            prev = ch;
            uart_puts("\r\n");
            line[idx] = '\0';
            idx = 0;
            return 1;
        }

        prev = ch;

        if (ch == '\b' || ch == 0x7f) {
            if (idx > 0) {
                idx--;
                uart_puts("\b \b");
            }
            continue;
        }

        if (idx < MAX_SIZE - 1) {
            line[idx++] = ch;
            uart_putc(ch);
        }
    }
    return 0;
}

int tokenize(char *line, struct command *cmd) {
    uint32_t argc = 0;
    char prev = 0;

    cmd->argv[argc] = line;

    while (*line) {
        if (argc > MAX_ARGC) return 0;

        if (*line == ' ') {
            *line = '\0';
            argc++;
        }

        else if ((*line != '\0') && (prev == '\0')) {
            cmd->argv[argc] = line;
        }

        prev = *line;
        line++;
    }

    cmd->argc = argc;

    return 1;
}

void dbg_print_args(struct command cmd) {
    int i = 0;
    while (i <= cmd.argc) {
        uart_puts(cmd.argv[i]);
        uart_puts("\r\n");
        i++;
    }
}

void led_init() {
    RCC->AHB1ENR = RCC_AHB1ENR_GPIOCEN;

    GPIOC->MODER &= ~GPIO_MODER_MODER13_Msk;
    GPIOC->MODER |= (1 << GPIO_MODER_MODER13_Pos);
}

int main(void)
{
    clock_init();
    uart_init();
    led_init();

    char line[MAX_SIZE];
    char *msg = "uart working. type your message:\r\n";

    /* Loop forever */
    uart_puts(msg);
	while (1) {
        if (uart_gets(line)) {
            struct command cmd;
            if (!tokenize(line, &cmd)) {
                uart_puts("Max args exceeded. only 6 args are permitted\r\n");
                continue;
            }
            uart_puts("received:\r\n");
            dbg_print_args(cmd);
        }
    }
    return 0;
}
