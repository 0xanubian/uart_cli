#include <stdlib.h>

#include "stm32f4xx.h"
#include "exti.h"
#include "uart.h"

volatile uint32_t btn_pressed_counter = 0u;
char buf[16] = {0};

void exti_init() {
    /* enable clock for GPIOB and SYSCFG */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    /* set GPIOB pin 0 to input mode */
    GPIOB->MODER &= ~GPIO_MODER_MODE0_Msk;
    
    /* set the port to pull up so by default it reads high voltage */
    GPIOB->PUPDR &= ~GPIO_PUPDR_PUPD0_Msk;
    GPIOB->PUPDR |= GPIO_PUPDR_PUPD0_0;

    /* map port B to EXTI line 0 */
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PB;

    /* enable falling trigger and disable rising trigger */
    EXTI->FTSR |= EXTI_FTSR_TR0;
    EXTI->RTSR &= ~EXTI_RTSR_TR0_Msk;

    /* clear the pending */
    EXTI->PR = EXTI_PR_PR0;

    /* unmask the EXTI line 0 */
    EXTI->IMR |= EXTI_IMR_IM0;

    /* set the priority for the interrupt */
    NVIC_SetPriority(EXTI0_IRQn, 2);
    NVIC_EnableIRQ(EXTI0_IRQn);
}

void EXTI0_IRQHandler() {
    if (EXTI->PR & EXTI_PR_PR0) {
        EXTI->PR = EXTI_PR_PR0;
        btn_pressed_counter++;

        utoa(btn_pressed_counter, buf, 10);
        uart_puts("EXTI button pressed... button pressed counter: ");
        uart_puts(buf);
        uart_puts("\r\n");
    }
}
