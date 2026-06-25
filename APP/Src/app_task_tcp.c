#include "app_task.h"
#include "bsp_wifi.h"
#include "bsp_tim.h"
#include "bsp_tcp_connect.h"
#include "bsp_parse.h"
#include "lwip/sockets.h"
#include "esp_log.h"




static const char *TASK3 = "TSAK_TCP";//用于应答


// ==================== 任务三：TCP Server 通讯任务 ====================


 /*******************************************************
 Author: PAN        Version: V1.0       Date:2026/06/15
 Function:          vTask_TCP_Server
 Description:       初始化WiFi并创建TCP服务器，接收客户端传来的网络信息并送入解析函数
 Input:             pvParameters - FreeRTOS任务参数（未使用）
 Output:            无
 Return:            无
 Others:            此任务为FreeRTOS任务，循环运行，处理客户端连接和数据接收
*******************************************************/
void vTask_TCP_Server(void *pvParameters)
{
    wifi_init_softap(); // 先去初始化网络
    sys_delay_ms(1000); // 等待系统稳定
    // 初始BSP里面的初始化
    int listen_sock = BSP_TCP_Server_Init(C6_TCP_PORT);
    if (listen_sock < 0) {
        ESP_LOGE(TASK3, "初始话失败 删除任务");
        vTaskDelete(NULL);   
    }
   
    /*循环调度 */
    char rx_buffer[128];
    while (1) 
    {
        //  接收信息和处理
        //  等手机客户端连进来
        int sock = BSP_TCP_Accept_Client(listen_sock);
        if (sock < 0) {
            sys_delay_ms(100); 
            continue; 
        }


        while(1)
        {
            // 阻塞接收原始网络字节数据
            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            if (len > 0) 
            {
                rx_buffer[len] = 0; 
               
                
                // 解析数据包 带入解析函数
                for (int i = 0; i < len; i++) 
                {
                    Protocol_Parse_Byte((uint8_t)rx_buffer[i]);                 
                }
            } 
            else if (len == 0) 
            {
                ESP_LOGW(TASK3, "客户端正常断开");
                break; 
            } 
            else 
            {
                ESP_LOGE(TASK3, "异常闪退");
                break; 
            }
        }
        if (sock != -1) {
            g_active_tcp_sock = -1;
            close(sock);
            ESP_LOGI(TASK3, " 挂起状态等待新连接");
        }
        
    }
}
