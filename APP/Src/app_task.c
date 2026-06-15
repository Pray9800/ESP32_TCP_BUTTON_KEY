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



uint8_t g_keys_value = 0, g_keys_value_last = 0;//按键之后返回值
uint8_t g_rgb_value = 0, g_rgb_sign = 0, g_rgb_sign_last = 0; //UR机械臂传来的sig 和对应颜色数值的value
// A5 5A 帧头 0A 指令 01长度 01数据 b6 6b真尾
//00 没有按键  01第一个按键  02第二个按键  03第三个按键  04第四个按键 
uint8_t wifi_key_msg[7] = {0xa5,0x5a,0x0a,0x01,0x00,0xb6,0x6b}; 
uint16_t rgb_cnt = 0;  // 刷新用于计数
uint8_t g_brightness = 100; //全局亮度
uint8_t g_brightness_flag = 0; //亮度变化指令
uint8_t g_red_blink_state = 1; //红灯闪烁状态 灯带闪烁 不是指示灯 初始化为1 保证先亮灯
uint16_t wsred_cnt = 0;          // 闪烁时间毫秒累加器

//任务通知类型
static const char *TAG = "TSAK_APP";//用于应答
static const char *TASK3 = "TSAK_TCP";//用于应答



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
static void vTask_Key_Sig(void *pvParameters)
{
    // 灯带初始化3
    bsp_key_init(); // 四个按键的GPIO配置
    ws2812_spi_init();// 初始化 SPI 和 DMA
     ESP_LOGI(TAG, "hello wisdom pan 2027"); 
    ws2812_set_num_spi(WS_ARRAY_SIZE, 255, 255, 255); Sys_Delay(500);
    ws2812_set_num_spi(WS_ARRAY_SIZE, 100, 100, 100); Sys_Delay(500);
    ws2812_set_num_spi(WS_ARRAY_SIZE, 0, 0, 255);     Sys_Delay(500);
    ws2812_set_num_spi(WS_ARRAY_SIZE, 50, 50, 50);    Sys_Delay(500);
    
   

    while (1)
    {
       

        // 周期性扫描 10ms 消抖
        g_keys_value = Key_Process_Scan(); 
        //数据有变
        if(g_keys_value != g_keys_value_last)
        {    
            wifi_key_msg[4] = g_keys_value;   //按键信号赋值 

            // 确定wifi是否连着
            if (g_active_tcp_sock != -1) 
            {
              send(g_active_tcp_sock, wifi_key_msg, sizeof(wifi_key_msg), 0);
            }
            Sys_Delay(10);
            g_keys_value_last = g_keys_value; //保存
            ESP_LOGI(TAG, "key value: %d", g_keys_value); // 打印按键值到串口监视器
        }
       
        // 10ms延时
        Sys_Delay(10); 
    }

}





// ==================== 任务三：TCP Server 通讯任务 ====================
static void vTask_TCP_Server(void *pvParameters)
{
    wifi_init_softap(); // 先去初始化网络
    sys_delay_ms(1000); // 等待系统稳定
 
    // 初始BSP里面的初始化
    int listen_sock = BSP_TCP_Server_Init(8080);
    if (listen_sock < 0) {
        ESP_LOGE(TASK3, "初始话失败 删除任务");
        vTaskDelete(NULL);   
    }
   
    /*循环调度 */
    while (1) 
    {
        //  接收信息和处理
        BSP_TCP_Wait_And_Handle(listen_sock);
        
        //   底层 accept 失败退出来了 
        sys_delay_ms(100); 
    }
}




// ==================== 任务四：灯带动态控制与刷新 ====================
void vTask_WsLight_Change(void *pvParameters)
{
    
    const TickType_t xFrequency = pdMS_TO_TICKS(10); // 10ms的意思
    
    ESP_LOGI(TAG, "WS2812灯带指令控制");

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
    
    // 创建指示灯闪烁任务（优先级设为较低的 4）
    xTaskCreate(vTask_Led_Blink, "vTask_Led_Blink", 1024, NULL, 4, NULL);

    // 创建按键核心逻辑任务按键优先级最高
    xTaskCreate(vTask_Key_Sig, "vTask_Key_Sig", 4096, NULL, 7, NULL);

    //创建WIFI连接和TCP/IP连接任务 传输按键信息
    xTaskCreate(vTask_TCP_Server, "vTask_TCP_Server", 4096, NULL, 6, NULL);

     //创建灯带控制代码  控制灯带
    xTaskCreate(vTask_WsLight_Change, "vTask_WsLight_Change", 4096, NULL, 5,&xWsLightTaskHandle);




}




#if rtos_mode
// 任务的“登记处”
void app_task_init(void)
{
    xTaskCreate(
        task_led,           // 1. 任务函数名
        "led_task",         // 2. 任务名字 (用于调试)
        2048,               // 3. 堆栈大小 (字节)
        NULL,               // 4. 传递给任务的参数
        5,                  // 5. 优先级 (数值越大越高)
        NULL                // 6. 任务句柄
    );
}
#endif