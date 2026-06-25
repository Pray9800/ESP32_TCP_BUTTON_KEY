#include "bsp_parse.h"
#include "esp_log.h"
#include "esp_system.h"
#include "bsp_tim.h"
uart1_data_t UR_Send_Msg;
static uint8_t UART1_Rxbuff[64]; // 内部帧缓存

static const char *TAG = "BSP_PARSE";


/*******************************************************
 Function:       Protocol_Parse_Byte
 Description:    纯粹的单字节输入状态机 (自适应任何数据长度)
                 A5 5A   cmd   len   data...   B6 6B
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
        


       //重启功能
    if (rx_cnt == 7) 
        {
            if (UART1_Rxbuff[2] == 0x0F && UART1_Rxbuff[3] == 0x01 && 
                UART1_Rxbuff[4] == 0x00 && UART1_Rxbuff[5] == 0xB6 && 
                UART1_Rxbuff[6] == 0x6B) 
            {
                ESP_LOGW(TAG, "收到软复位指令，主控即将重启！");
                Sys_Delay(50); // 给串口留 50ms 打印日志的时间
                esp_restart();                 // 触发硬件级重启
            }
        }




        //  当收到第 4 个字节(即 rx_cnt==4)时，
        // UART1_Rxbuff[3] 刚好就是上位机定义的 
        //  2(包头) + 1(cmd) + 1(len) + 实际数据长度 + 2(包尾)
        //  总长度 = 6 + UART1_Rxbuff[3];
        
        if (rx_cnt >= 6) // 只有收到了包含包尾的长度，才开始校验
        {
            uint8_t expect_total_len = 6 + UART1_Rxbuff[3]; // 动态计算总长度
            
            if (rx_cnt == expect_total_len) 
            {
                //  包尾 校验
                if (UART1_Rxbuff[rx_cnt - 2] == 0xB6 && UART1_Rxbuff[rx_cnt - 1] == 0x6B)
                {
                    //  提取核心参数
                    UR_Send_Msg.cmd  = UART1_Rxbuff[2];
                    UR_Send_Msg.len  = UART1_Rxbuff[3];
                    UR_Send_Msg.data = UART1_Rxbuff[4]; 
                    //开启任务通知 
                   if (xWsLightTaskHandle != NULL) {
                        xTaskNotifyGive(xWsLightTaskHandle);
                    }
                    
                    ESP_LOGI(TAG , "解析结果 CMD:%02X, LEN:%02X, DATA:%02X", 
                             UR_Send_Msg.cmd, UR_Send_Msg.len, UR_Send_Msg.data);
                }
                rx_cnt = 0; //  清零 
            }
            else if (rx_cnt > 10) 
            {
                rx_cnt = 0; // 安全防线，防止异常长包导致数组越界
            }
        }
    }
}