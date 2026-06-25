#include "app_task.h"
#include "bsp_led.h"
#include "esp_log.h"
#include "bsp_key.h"
#include "bsp_tim.h"
#include "bsp_ws2812_spi.h"
#include "bsp_tcp_connect.h"
#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp_wifi.h"           
#include "lwip/sockets.h"        
#include "bsp_parse.h"
#include "bsp_iwdg.h"
#include <errno.h>
#include "freertos/semphr.h"             //  引入 FreeRTOS 信号量头文件

SemaphoreHandle_t xTcpSendMutex = NULL;  //   声明一个全局的 TCP 发送互斥锁

uint8_t g_keys_value = 0, g_keys_value_last = 0;//按键之后返回值
uint8_t g_rgb_value = 0, g_rgb_sign = 0, g_rgb_sign_last = 0; //UR机械臂传来的sig 和对应颜色数值的value
// A5 5A 帧头 0A 指令 01长度 01数据 b6 6b真尾
//00 没有按键  01第一个按键  02第二个按键  03第三个按键  04第四个按键 
uint8_t wifi_key_msg[7] = {0xa5,0x5a,0x0a,0x01,0x00,0xb6,0x6b}; 
//灯反馈模式 C0开头防止混淆
uint8_t wifi_Light_msg[7] = {0xa5,0x5a,0x0c,0x01,0x00,0xb6,0x6b}; 
uint16_t rgb_cnt = 0;  // 刷新用于计数
uint8_t g_brightness = 100; //全局亮度
uint8_t g_brightness_flag = 0; //亮度变化指令
uint8_t g_red_blink_state = 1; //红灯闪烁状态 灯带闪烁 不是指示灯 初始化为1 保证先亮灯
uint16_t wsred_cnt = 0;        // 闪烁时间毫秒累加器

//任务通知类型
static const char *TAG = "TSAK_APP";//用于应答
static const char *TASK2 = "TSAK_KEY";//用于应答
static const char *TASK4 = "TSAK_WS_Light";//用于应答





//RTOS类型 
//  灯带控制任务的句柄
TaskHandle_t xWsLightTaskHandle = NULL;

// ==================== 任务一：  指示灯闪烁任务 ====================
static void vTask_Led_Blink(void *pvParameters)
{
    // 硬件初始化放在任务最开始，只执行一次
    led_init(); 
    
    while (1)
    {
        led_blink();  
        Sys_Delay(100);
    }
}



// ==================== 任务二：按键扫描逻辑任务 ====================
 /*******************************************************
 Author: PAN        Version: V1.0       Date:2026/06/15
 Function:          vTask_Key_Sig
 Description:       按键扫描并检测按键值变化，通过TCP发送按键数据到客户端
 Input:             pvParameters - FreeRTOS任务参数（未使用）
 Output:            无
 Return:            无
 Others:            此任务为FreeRTOS任务，优先级7（最高），10ms周期扫描按键并上报
*******************************************************/
static void vTask_Key_Sig(void *pvParameters)
{
    // 灯带初始化3
     bsp_key_init(); // 四个按键的GPIO配置
     BSP_IWDG_Add_Current_Task();
      

    
    while (1)
    {     
        // 周期性扫描 10ms 消抖
        g_keys_value = Key_Process_Scan(); 
        
        //数据有变
        if(g_keys_value != g_keys_value_last)
        {    
            wifi_key_msg[4] = g_keys_value;   //按键信号赋值 

            // 确定wifi是否连着，并且确保互斥锁已经创建
            if (g_active_tcp_sock != -1 && xTcpSendMutex != NULL) 
            {
                // 1. 尝试获取发送互斥锁（最多等100ms），防止跟灯带任务抢通道
                if (xSemaphoreTake(xTcpSendMutex, pdMS_TO_TICKS(100)) == pdTRUE) 
                {
                    int ret = send(g_active_tcp_sock, wifi_key_msg, sizeof(wifi_key_msg), 0);
                    
                    // 2. 发完立刻释放锁，让出通道
                    xSemaphoreGive(xTcpSendMutex); 

                    if (ret >= 0) {
                        g_keys_value_last = g_keys_value;
                        
                     
                
                        Sys_Delay(20); 
                        
                    } else {
                        ESP_LOGW(TASK2, "send key failed, errno=%d", errno);
                    }
                }
            }
            else
            {
              // 如果断网了，照常更新状态，防止重连后误发旧状态
              g_keys_value_last = g_keys_value;
            }
            ESP_LOGI(TASK2, "key value: %d", g_keys_value); // 打印按键值到串口监视器
        }  
        
        // 10ms延时
        BSP_IWDG_Feed();
        Sys_Delay(10);
    }

}

// ==================== 任务三：app_task_tcp.c ====================
 

// ==================== 任务四：灯带动态控制与刷新 ====================
 /*******************************************************
 Author: PAN        Version: V1.0       Date:2026/06/15
 Function:          vTask_WsLight_Change
 Description:       控制WS2812灯带颜色、亮度及闪烁状态，接收TCP指令并实时刷新灯带输出
 Input:             pvParameters - FreeRTOS任务参数（未使用）
 Output:            无
 Return:            无
 Others:            此任务为FreeRTOS任务，优先级5，10ms周期调度，支持红灯500ms闪烁及任务通知机制
*******************************************************/
void vTask_WsLight_Change(void *pvParameters)
{
    
    ws2812_spi_init();// 初始化 SPI 和 DMA
    ws2812_set_num_spi(WS_ARRAY_SIZE, 255, 255, 255); Sys_Delay(300);
    ws2812_set_num_spi(WS_ARRAY_SIZE, 100, 100, 100); Sys_Delay(300);
    ws2812_set_num_spi(WS_ARRAY_SIZE, 0, 0, 255);     Sys_Delay(300);
    ws2812_set_num_spi(WS_ARRAY_SIZE, 50, 50, 50);    Sys_Delay(300);
    const TickType_t xFrequency = pdMS_TO_TICKS(10); // 10ms的意思
    
    ESP_LOGI(TASK4, "WS2812灯带指令控制");

    while (1) 
    {
     
        uint32_t notified = ulTaskNotifyTake(pdTRUE, xFrequency);  //等10ms 没有任务就去执行红灯闪烁

        // ==================== 如果是网络丢来了新指令 ====================
        if (notified > 0)  
        {
            
            if (UR_Send_Msg.cmd == 0x0B) // 亮度指令
            {
                g_brightness = UR_Send_Msg.data; 
                g_brightness_flag = 1;
            }
            else if (UR_Send_Msg.cmd == 0x0A) // 颜色变化指令
            {        
                g_rgb_sign = UR_Send_Msg.data; 

                if      (g_rgb_sign == 5) g_rgb_value = 4; // 红闪
                else if (g_rgb_sign == 4) g_rgb_value = 4; // 红常亮
                else if (g_rgb_sign == 3) g_rgb_value = 2; // 绿
                else if (g_rgb_sign == 2) g_rgb_value = 1; // 蓝
                else if (g_rgb_sign == 1) g_rgb_value = 7; // 白
            }

                //反馈更新
                if (g_active_tcp_sock != -1) 
            {  
                wifi_Light_msg[4]= UR_Send_Msg.data;
                send(g_active_tcp_sock, wifi_Light_msg, sizeof(wifi_Light_msg), 0);
            }
        }

        // ====================   10ms  ====================
        
        //  红灯 500ms 异步闪烁状态机
        if (g_rgb_sign == 5) 
        {
            wsred_cnt += 10; //  任务每 10ms 一次，所以这里直接累加 10ms
            if (wsred_cnt >= 500) 
            {
                g_red_blink_state = !g_red_blink_state; // 翻转亮灭状态
                wsred_cnt = 0;                          // 复位闪烁计时
                g_rgb_sign_last = 0xFF;                 // 保证红色灯带闪烁时候  
            }
        }
        else 
        {
            wsred_cnt = 0;
            g_red_blink_state = 1; 
        }
        //  刷新判定状态机
        if ((g_rgb_sign != g_rgb_sign_last) || g_brightness_flag) 
        {
            rgb_cnt++; // 刷新次数
            if (rgb_cnt >= 3) 
            {   
                rgb_cnt = 0;
                g_rgb_sign_last = g_rgb_sign; // 达到 3 次，锁定不再重复刷新
                g_brightness_flag = 0;        // 清除亮度变化标志

            } 
            else 
            {  
                // 计算当前亮灭闪烁状态下的实际输出亮度
                uint8_t current_brightness = g_brightness;
                if (g_rgb_sign == 5 && g_red_blink_state == 0) 
                {
                    current_brightness = 0; // 灭灯状态，亮度强行归零
                }
                
               
                // 执行灯带灯光
                ws2812_set_num_spi(WS_ARRAY_SIZE, 
                                  ((g_rgb_value >> 2) & 0x01) * current_brightness, 
                                  ((g_rgb_value >> 1) & 0x01) * current_brightness, 
                                  ((g_rgb_value & 0x01))      * current_brightness);
            }              
        }
    }
}



// ==================== APP 任务层 ====================
void app_task_init(void)
{
    BSP_IWDG_Global_Init(3);


    
    xTcpSendMutex = xSemaphoreCreateMutex(); 
    // 创建指示灯闪烁任务（优先级设为较低的 4）
    xTaskCreate(vTask_Led_Blink, "vTask_Led_Blink", 1024, NULL, 4, NULL);

    // 创建按键核心逻辑任务按键优先级最高
    xTaskCreate(vTask_Key_Sig, "vTask_Key_Sig", 4096, NULL, 7, NULL);

    //创建WIFI连接和TCP/IP连接任务 传输按键信息
    xTaskCreate(vTask_TCP_Server, "vTask_TCP_Server", 4096, NULL, 6, NULL);

     //创建灯带控制代码  控制灯带
    xTaskCreate(vTask_WsLight_Change, "vTask_WsLight_Change", 4096, NULL, 5,&xWsLightTaskHandle);

}




// #if rtos_mode
// // 任务的“登记处”
// void app_task_init(void)
// {
//     xTaskCreate(
//         task_led,           // 1. 任务函数名
//         "led_task",         // 2. 任务名字 (用于调试)
//         2048,               // 3. 堆栈大小 (字节)
//         NULL,               // 4. 传递给任务的参数
//         5,                  // 5. 优先级 (数值越大越高)
//         NULL                // 6. 任务句柄
//     );
// }
// #endif