#include "stm32f4xx.h"
#include "init.h"

// starts the clock to max frequency i.e. 84mhz
void clock_init(void)
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

// starts the onboard led
void led_init() {
    // enable its clock signal of GPIOC
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    // inable the pin13 of GPIOC
    GPIOC->MODER &= ~GPIO_MODER_MODER13_Msk;
    GPIOC->MODER |= (1 << GPIO_MODER_MODER13_Pos);
}

