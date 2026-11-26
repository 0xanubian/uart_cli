#include "stm32f4xx.h"
#include "systick.h"

volatile uint32_t time_ms = 0u;

// starts and configures the SysTick timer
void systick_init() {
    uint32_t reload_val = (84000000u / 1000u) - 1;

    SysTick->CTRL = 0;      // disable systick while initialising
    
    // initialise the systick with correct load and ctrl values
    SysTick->LOAD = reload_val & 0x00ffffffu;
    SysTick->VAL = 0;
    SysTick->CTRL |= 1u | (1u << 1) | (1u << 2);

    // enables the interrupt and sets its priority
    NVIC_SetPriority(SysTick_IRQn, 1);
    NVIC_EnableIRQ(SysTick_IRQn);
}

void SysTick_Handler() {
    time_ms++;
}
