#include <stdint.h>

extern volatile uint32_t btn_pressed_counter;
extern char buf[16];

void exti_init();
void EXTI0_IRQHandler();
