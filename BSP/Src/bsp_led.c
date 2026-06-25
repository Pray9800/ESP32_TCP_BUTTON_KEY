#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "bsp_led.h"
#include "bsp_tim.h"
#include <stdio.h>

static  uint8_t led_state = 0; // 0: 灭，1: 亮
void led_init(void)
{
    gpio_reset_pin(BLINK_GPIO); //复位到默认状态
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);//
}

   
void led_blink(void)
{
    if(led_state==1)
    {
        gpio_set_level(BLINK_GPIO, 0);
        led_state=!led_state;
    }
    else
    {
        gpio_set_level(BLINK_GPIO, 1);
        led_state=!led_state;
    }
  
}





 

