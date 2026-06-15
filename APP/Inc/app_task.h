



#ifndef __APP_TASK_H_
#define __APP_TASK_H_


#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define rtos_mode 0
#define blink_fre_100ms   50//led 闪烁频率
// #define BRIGHTNESS 100   //亮度

extern TaskHandle_t xWsLightTaskHandle;

/**
 * @brief 初始化并启动 APP 层的全部 RTOS 任务
 */
void app_task_init(void);


#endif /* __APP_TASK_H_ */
