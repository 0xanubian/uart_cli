#include "stm32f4xx.h"
#include "uart.h"

static char rx_ring_buf[MAX_SIZE];      // receive ring buffer
static volatile uint32_t rx_head = 0;   // write pointer
static volatile uint32_t rx_tail = 0;   // read pointer

static char tx_ring_buf[MAX_SIZE];      // transmit ring buffer
static volatile uint32_t tx_head = 0;
static volatile uint32_t tx_tail = 0;

// initiales the uart protocol. enables the clock, sets the alternate function 
// and baud rate and interrupt and sets priority to the interrupt
void uart_init() {
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

// interrupt handler for uart
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

// transmit a character. it waits if the ring buffer is full. not non-blocking
void uart_putc(char ch) {
    uint32_t nxt_head = (tx_head + 1u) & MAX_SIZE_MSK;
    
    while (nxt_head == tx_tail) {} // ring is full so wait 

    tx_ring_buf[tx_head] = ch;
    tx_head = nxt_head;

    USART1->CR1 |= USART_CR1_TXEIE;
}

// tranmit a whole string using uart_putc()
void uart_puts(const char *str) {
    while (*str) {
        uart_putc(*str);
        str++;
    }
}

// receive a line from uart. its non_blocking. it uses static variables so that
// it can receive the whole line in multiple calls to itself. it return 1 only
// when the user enters a new-line (CR-LF) otherwise it returns 0. if the ring 
// buffer is full then it will drop the characters so user cannot enter a line 
// longer than MAX_SIZE
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

        // handlers backspace or delete
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


