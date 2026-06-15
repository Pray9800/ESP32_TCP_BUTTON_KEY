# 登记当前目录下的所有 .c 文件
# 暴露当前目录的 .h 文件给别人用
# 声明自己依赖系统自带的 driver (比如 gpio 控制)


idf_component_register(SRCS "Src/bsp_led.c"
                            "Src/bsp_key.c"
                            "Src/bsp_tim.c"
                            "Src/bsp_ws2812_spi.c"
                            "Src/bsp_wifi.c"
                            "Src/bsp_tcp_connect.c"
                            "Src/bsp_parse.c"
                             "Src/bsp_iwdg.c"
                       INCLUDE_DIRS "Inc"
                       REQUIRES driver esp_driver_gpio esp_driver_uart
                        esp_driver_spi freertos esp_wifi esp_netif lwip nvs_flash log )
                       