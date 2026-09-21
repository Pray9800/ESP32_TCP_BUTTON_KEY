#include "app_task.h"
#include "bsp_parse.h"
#include "esp_log.h"
#include "esp_system.h"
#include "bsp_tim.h"
#include "bsp_wifi.h"
#include "lwip/sockets.h"
#include "string.h"
uart1_data_t UR_Send_Msg;
static uint8_t UART1_Rxbuff[64]; // 内部帧缓存

static const char *TAG = "BSP_PARSE";

extern int g_active_tcp_sock;
/*******************************************************
 Author: PAN       Version: V1.0       Date:2026/07/29
 Function:          Protocol_Parse_Byte
 Description:       解析串口单字节输入数据，识别帧头、帧长、数据负载和帧尾，
                    并根据命令类型分发灯光控制、序列号写入/读取及系统复位等处理
 Input:             rx_temp: 单字节串口输入数据
 Output:            无
 Return:            无
 Others:            帧格式为 A5 5A cmd len data... B6 6B，支持命令分发与状态机复位
*******************************************************/
void Protocol_Parse_Byte(uint8_t rx_temp)
{
    static uint8_t rx_cnt = 0;
    //防止越界
    if (rx_cnt >= sizeof(UART1_Rxbuff)) {
        rx_cnt = 0; 
    }
    if (rx_cnt == 0) // 1. 找包头 A5
    {
        if (rx_temp == 0xA5) {
            UART1_Rxbuff[0] = rx_temp;
            rx_cnt = 1;
        }
    }
    else if (rx_cnt == 1) // 2. 找包头 5A
    {
        if (rx_temp == 0x5A) {
            UART1_Rxbuff[1] = rx_temp;
            rx_cnt = 2;
        } else {
            rx_cnt = 0; // 乱码，复位
        }
    }
    else // 包头确认，开始灌入数据
    {
        UART1_Rxbuff[rx_cnt] = rx_temp;
        rx_cnt++;
        
       //重启功能  优先触发
    if (rx_cnt == 7) 
        {
            if (UART1_Rxbuff[2] == CMD_SYS_RESET   && UART1_Rxbuff[3] == 0x01 && 
                UART1_Rxbuff[4] == 0x00 && UART1_Rxbuff[5] == 0xB6 && 
                UART1_Rxbuff[6] == 0x6B) 
            {
                ESP_LOGW(TAG, "收到软复位指令");
                Sys_Delay(50); // 给串口留 50ms 打印日志的时间
                esp_restart();                 // 触发硬件级重启
            }

            if (UART1_Rxbuff[2] == CMD_BUT_CHECK &&UART1_Rxbuff[3] == 0x01 &&
                UART1_Rxbuff[4] == 0x00 &&UART1_Rxbuff[5] == 0xB6 &&
                UART1_Rxbuff[6] == 0x6B)
            {
                ESP_LOGW(TAG, "收到按键查询指令");
                
                  g_key_query_flag = 1;
                  rx_cnt = 0;
            }
        }
        //  当收到第 4 个字节(即 rx_cnt==4)时，
        // UART1_Rxbuff[3] 刚好就是上位机定义的 
        //  2(包头) + 1(cmd) + 1(len) + 实际数据长度 + 2(包尾)
        //  总长度 = 6 + UART1_Rxbuff[3];  

        if (rx_cnt >= 6) // 只有收到了包含包尾的最小长度，才开始校验
        {
            uint8_t expect_total_len = 6 + UART1_Rxbuff[3]; // 动态计算总长度
            
            if (rx_cnt == expect_total_len) 
            {
                // 包尾 校验
                if (UART1_Rxbuff[rx_cnt - 2] == 0xB6 && UART1_Rxbuff[rx_cnt - 1] == 0x6B)
                {
                    // ---------------- 【绝对主线：提取核心参数】 ----------------
                    UR_Send_Msg.cmd  = UART1_Rxbuff[2];
                    UR_Send_Msg.len  = UART1_Rxbuff[3];
                    UR_Send_Msg.data = UART1_Rxbuff[4]; 


                    //  灯光控制指令 —— 直接唤醒灯光任务

                    if (UR_Send_Msg.cmd == CMD_SET_COLOR  || UR_Send_Msg.cmd ==CMD_SET_BRIGHT  )
                    {
                        // 开启任务通知，唤醒灯光任务
                        if (xWsLightTaskHandle != NULL) {
                            xTaskNotifyGive(xWsLightTaskHandle);
                        }
                        
                        ESP_LOGI(TAG , "解析结果 CMD:%02X, LEN:%02X, DATA:%02X", 
                                 UR_Send_Msg.cmd, UR_Send_Msg.len, UR_Send_Msg.data);
                    }

                    //  出厂初始化拦截 —— 修改/读取 WiFi 序列号

                    else 
                    {
                        // 1. 写入序列号 0x11 
                        if (UR_Send_Msg.cmd == CMD_SET_SN) 
                        {
                            uint8_t i;
                            for (i = 4; i < 8; i++) {  // SN占用索引 4,5,6,7
                                //由于是十六进制转接转成十进制  就是保证十位数和各位是都最大是9
                                if ((UART1_Rxbuff[i] >> 4) > 9 || (UART1_Rxbuff[i] & 0x0F) > 9) break; 
                            }
                            //4 5 6 7都通过校验 此时i加到8 说明校验通过
                            if (i == 8 && UR_Send_Msg.len == 0x04) // 校验通过且 LEN 必须为 4
                            {
                                char new_sn[9] = {0}; //保存8位序列号
                                //组合
                                snprintf(new_sn, sizeof(new_sn), "%02X%02X%02X%02X", 
                                         UART1_Rxbuff[4], UART1_Rxbuff[5], UART1_Rxbuff[6], UART1_Rxbuff[7]);

                                if (wifi_set_sn(new_sn) == ESP_OK) { // 写入 NVS
                                    ESP_LOGI(TAG, "SN 更新成功: %s, 即将重启...", new_sn);
                                    uint8_t resp[] = {0xA5, 0x5A, CMD_SET_SN, 0x01, 0x00, 0xB6, 0x6B};
                                    if (g_active_tcp_sock >= 0) send(g_active_tcp_sock, resp, sizeof(resp), 0);
                                    Sys_Delay(200); 
                                    esp_restart();
                                }
                            }
                            else 
                            {
                                ESP_LOGE(TAG, "SN 写入失败 格式不对");
                                uint8_t resp[] = {0xA5, 0x5A, CMD_SET_SN, 0x01, 0x01, 0xB6, 0x6B};
                                if (g_active_tcp_sock >= 0) send(g_active_tcp_sock, resp, sizeof(resp), 0);
                            }
                        }
                        
                        // 2. 恢复出厂 0x12 
                        else if (UR_Send_Msg.cmd == CMD_RESET_SN) 
                        {
                            ESP_LOGI(TAG, "重置 SN 为默认...");
                            wifi_reset_sn();
                            uint8_t resp[] = {0xA5, 0x5A, CMD_RESET_SN, 0x01, 0x00, 0xB6, 0x6B};
                            if (g_active_tcp_sock >= 0) send(g_active_tcp_sock, resp, sizeof(resp), 0);
                            Sys_Delay(200);
                            esp_restart();
                        }
                        
                        // 3. 读取序列号 0x13[cite: 2]
                        else if (UR_Send_Msg.cmd == CMD_GET_SN) 
                        {
                            char current_sn[10] = {0};
                            wifi_get_sn(current_sn, sizeof(current_sn)); 

                            uint8_t resp[11] = {0xA5, 0x5A, CMD_GET_SN, 0x04};
                            for (int j = 0; j < 4; j++) {
                                resp[4 + j] = ((current_sn[j * 2] - '0') << 4) | (current_sn[j * 2 + 1] - '0');
                            }
                            resp[8] = 0xB6;
                            resp[9] = 0x6B;

                            if (g_active_tcp_sock >= 0) send(g_active_tcp_sock, resp, 10, 0);
                            ESP_LOGI(TAG, "上报当前序列号: %s", current_sn);
                        }                   
                    }
                }
                rx_cnt = 0; // 清零，等待下一帧
            }
            else if (rx_cnt > 32) 
            {
                rx_cnt = 0; // 安全防线，放宽到 32 字节以防长包被截断
            }
        }
    }
}


/*******************************************************
 Author: PAN       Version: V1.0       Date:2026/09/20
 Function:           ASCII_Parse_Byte
 Description:       版本查询
 Input:             rx_temp: 单字节串口输入数据
 Output:            无
 Return:            无
 Others:            帧格式为 A5 5A cmd len data... B6 6B，支持命令分发与状态机复位
*******************************************************/
void ASCII_Parse_Byte(uint8_t rx_temp)
{
    // VERSION 查询指令 (7字节滑动窗口, 不依赖换行符)
    static char ver_buf[8] = {0};

    memmove(ver_buf, ver_buf + 1, 6);
    ver_buf[6] = (char)rx_temp;

    if (strcmp(ver_buf, "VERSION") == 0)
    {
        char resp[64];
        int n = snprintf(resp, sizeof(resp), "%s\r\n", FW_VERSION_STR);
        if (g_active_tcp_sock >= 0) send(g_active_tcp_sock, resp, n, 0);
        ESP_LOGI(TAG, "上报固件版本: %s", FW_VERSION_STR);
    }
}