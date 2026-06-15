



#ifndef __APP_TASK_H_
#define __APP_TASK_H_


#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define rtos_mode 0
#define blink_fre_100ms   50//led 闪烁频率
// #define BRIGHTNESS 100   //亮度



// ==================== 外骨骼网络出厂全局配置 ====================
#define EXSO_WIFI_SSID          "YOZX-C6"        // Wi-Fi 热点名称
#define EXSO_WIFI_PASS          "12345678"       // Wi-Fi 密码

#define EXSO_SERVER_IP          "192.168.100.125"// ESP32 本机静态IP
#define EXSO_SERVER_NETMASK     "255.255.255.0"  // 子网掩码
#define EXSO_SERVER_GW          "192.168.100.1"  // 网关

#define EXSO_TCP_PORT           8080             // 监听端口


extern TaskHandle_t xWsLightTaskHandle;

/**
 * @brief 初始化并启动 APP 层的全部 RTOS 任务
 */
void app_task_init(void);


#endif /* __APP_TASK_H_ */
