#ifndef __BSP_WIFI_H
#define __BSP_WIFI_H
#include "esp_err.h"   // 提供 esp_err_t
#include <stddef.h>    // 提供 size_t
// 开启  Wi-Fi 热点
void wifi_init_softap(void);
esp_err_t wifi_get_sn(char *sn_buf, size_t max_len);
esp_err_t wifi_set_sn(const char *sn_str);
esp_err_t wifi_reset_sn(void);
#endif