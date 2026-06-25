#include "bsp_iwdg.h"
#include "esp_task_wdt.h" // ESP32 官方任务看门狗库
#include "esp_log.h"

static const char *TAG = "BSP_IWDG";

/*******************************************************
 Function:          BSP_IWDG_Init
 Description:       初始化全局任务看门狗，并挂载当前任务 
 Input:             timeout_sec: 超时时间(秒)
*******************************************************/
void BSP_IWDG_Global_Init(uint32_t timeout_sec) {
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = timeout_sec * 1000, 
        .idle_core_mask = 0,              
        .trigger_panic = true             
    };
    esp_task_wdt_init(&twdt_config);
    ESP_LOGI(TAG, "全局看门狗: %ld 秒", timeout_sec);
}




// 2. 任务签到（把当前任务加入监视名单）
void BSP_IWDG_Add_Current_Task(void) {
    esp_task_wdt_add(NULL); 
     
}

/*******************************************************
 Function:          BSP_IWDG_Feed
 Description:       重置当前任务的看门狗计时器 (摸狗头)
*******************************************************/
void BSP_IWDG_Feed(void)
{
    
    esp_task_wdt_reset();
}