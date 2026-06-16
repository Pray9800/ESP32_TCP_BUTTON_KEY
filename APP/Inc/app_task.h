



#ifndef __APP_TASK_H_
#define __APP_TASK_H_


#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define rtos_mode 0
#define blink_fre_100ms   50//led 闪烁频率
// #define BRIGHTNESS 100   //亮度



// ==================== 网络出厂全局配置 ====================
#define C6_WIFI_SSID          "YOZX-C6"        // Wi-Fi 热点名称
#define C6_WIFI_PASS          "12345678"       // Wi-Fi 密码

#define C6_SERVER_IP          "192.168.100.125"// ESP32 本机静态IP
#define C6_SERVER_NETMASK     "255.255.255.0"  // 子网掩码
#define C6_SERVER_GW          "192.168.100.1"  // 网关

#define C6_TCP_PORT           8080             // 监听端口




extern TaskHandle_t xWsLightTaskHandle;
 
//初始化并启动 APP 层的全部 RTOS 任务
 
void app_task_init(void);
void vTask_TCP_Server(void *pvParameters);  //TCP连接单独一个文件

#endif /* __APP_TASK_H_ */
