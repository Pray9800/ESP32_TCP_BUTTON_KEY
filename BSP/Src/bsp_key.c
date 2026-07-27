

#include "bsp_key.h"
#include "bsp_tim.h"
void bsp_key_init(void)
{
    // 初始化配置结构体，准备配置 GPIO
    gpio_config_t io_conf = {
       
        .pin_bit_mask = (1ULL << KEY0_PIN) | 
                        (1ULL << KEY1_PIN) | 
                        (1ULL << KEY2_PIN) | 
                        (1ULL << KEY3_PIN),
        
        .mode = GPIO_MODE_INPUT,               // 输入模式
        .pull_up_en = GPIO_PULLUP_ENABLE,      // 开启内部上拉上拉电阻
        .pull_down_en = GPIO_PULLDOWN_DISABLE,  // 关闭内部下拉
        .intr_type = GPIO_INTR_DISABLE         // 暂时不使用中断，纯轮询读取
    };
    
    // 把这张配置表丢给系统，4 个引脚瞬间配置完成
    gpio_config(&io_conf);
}



 /*******************************************************
 Author: PAN       Version: V1.0       Date:2026/05/11
 Function:          Key_Scan_Once
 Description:       按键读取和映射
 Input:             
 Output:            按键代表的数值
 Return:            无
 Others:            无
*******************************************************/
static uint8_t Key_Scan_Once(void)
{
    // 依次检测 KEY1, KEY0, KEY2, KEY3 (低电平为 0 即代表按下)
    if (KEY1_READ() == 0) return KEY1_SIG;   //物理按键1
    if (KEY0_READ() == 0) return KEY0_SIG;   //物理按键2
    if (KEY2_READ() == 0) return KEY2_SIG;   //物理按键3
    if (KEY3_READ() == 0) return KEY3_SIG;   //物理按键4
    return 0;  //没有按键按下就是发送0
}

/*******************************************************
 Author: PAN       Version: V1.0       Date:2026/05/11
 Function:          Key_Process_Scan
 Description:       读取按键 读取两次  优先级K1 K0 K2 K3 
 Input:             无
 Output:            无
 Return:            按键代表的数值
 Others:            无
*******************************************************/
uint8_t Key_Process_Scan(void) 
{
     uint8_t keys_value_first, keys_value_second;
     keys_value_first = Key_Scan_Once();
     
     Sys_Delay(15);  
     
     keys_value_second = Key_Scan_Once();
     if (keys_value_second == keys_value_first && keys_value_second != 0) {
         return keys_value_second;
     } else {
         return 0;
     }
}


