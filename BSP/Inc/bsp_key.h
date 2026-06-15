#ifndef BSP_KEY_H
#define BSP_KEY_H

#include "driver/gpio.h"

//  对应原理图映射的 ESP32-S3 物理引脚
#define KEY0_PIN            GPIO_NUM_21
#define KEY1_PIN            GPIO_NUM_47
#define KEY2_PIN            GPIO_NUM_48
#define KEY3_PIN            GPIO_NUM_45

//  （0 代表按下，1 代表松开）
#define KEY0_READ()          gpio_get_level(KEY0_PIN)
#define KEY1_READ()          gpio_get_level(KEY1_PIN)
#define KEY2_READ()          gpio_get_level(KEY2_PIN)
#define KEY3_READ()          gpio_get_level(KEY3_PIN)

// 按键信号


#define KEY0_SIG             2  
#define KEY1_SIG             1  
#define KEY2_SIG             3  
#define KEY3_SIG             4  

// 初始化函数声明
void bsp_key_init(void);
uint8_t Key_Process_Scan(void);
#endif // BSP_KEY_H