# 声明 APP 层依赖了 FreeRTOS 操作系统 和 BSP 硬件底层
# 如果你在这层用了 ESP_LOGI，最好也加上 log 依赖

idf_component_register(SRCS "Src/app_task.c"
                            "Src/app_task_led.c"
                            "Src/app_task_tcp.c"
                           
                       INCLUDE_DIRS "Inc"
                       REQUIRES freertos BSP log)