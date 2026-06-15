#include "bsp_iwdg.h"
#include "esp_task_wdt.h" // ESP32 官方任务看门狗库
#include "esp_log.h"

static const char *TAG = "BSP_IWDG";

/*******************************************************
 Function:          BSP_IWDG_Init
 Description:       初始化全局任务看门狗，并挂载当前任务 
 Input:             timeout_sec: 超时时间(秒)
*******************************************************/
void BSP_IWDG_Init(uint32_t timeout_sec)
{
    // ================= 新版看门狗结构体 =================
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = timeout_sec * 1000, // 注意：新版单位变成了毫秒！
        .idle_core_mask = 0,              // 0 代表不监视系统空闲任务，只监视我们自己的任务
        .trigger_panic = true             // 超时后触发系统 Panic (强制硬件复位重启)
    };
    
    // 1. 传入结构体指针进行初始化
    esp_task_wdt_init(&twdt_config);
    // ===================================================================
    
    // 2. 将【当前正在执行这个函数的任务】添加到看门狗的暗杀名单中
    // 传 NULL 代表指代当前任务
    esp_task_wdt_add(NULL);
    
    ESP_LOGI(TAG, "任务看门狗已就位，超时引爆时间: %d 秒，当前任务已被严密监视", timeout_sec);
}

/*******************************************************
 Function:          BSP_IWDG_Feed
 Description:       重置当前任务的看门狗计时器 (摸狗头)
*******************************************************/
void BSP_IWDG_Feed(void)
{
    // 告诉系统：当前任务还活着，没有死锁，把定时器清零
    esp_task_wdt_reset();
}