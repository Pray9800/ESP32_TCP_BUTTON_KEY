#ifndef BSP_LED_H   
#define BSP_LED_H


#include "driver/gpio.h"

#define BLINK_GPIO GPIO_NUM_4

void led_init(void);
void led_blink(void);

#endif // BSP_LED_H