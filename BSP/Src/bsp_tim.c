


#include "bsp_tim.h"
#include "freertos/FreeRTOS.h" // 必须包含 FreeRTOS 核心头文件
#include "freertos/task.h"     // 必须包含任务头文件


 
/*******************************************************
 Author: PAN        Version: V1.0       Date:2026/05/11
 Function:          Key_Scan_Once
 Description:       毫秒延时（ms）
 Input:             ms           
 Output:            按键代表的数值
 Return:            无
 Others:            无
*******************************************************/
void Sys_Delay(uint16_t time_ms)
{
    // pdMS_TO_TICKS 会把毫秒转换为 FreeRTOS 的 Tick 节拍数
    vTaskDelay(pdMS_TO_TICKS(time_ms));
}
 