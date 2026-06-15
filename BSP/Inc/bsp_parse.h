#ifndef __BSP_PARSE_H__
#define __BSP_PARSE_H__

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// 接收指令结构体
typedef struct {
    uint8_t    cmd;    // 命令
    uint8_t    len;    // 长度/寄存器地址
    uint8_t    data;   // 数据主体
} uart1_data_t;

// 外部声明 指令和灯带
extern uart1_data_t UR_Send_Msg;
extern TaskHandle_t xWsLightTaskHandle;
//解析函数声明
void Protocol_Parse_Byte(uint8_t rx_temp);

#endif