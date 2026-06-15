#include "bsp_wifi.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>
#include "esp_netif.h"
#include "lwip/ip_addr.h"
#define MODE_AP   0  // 热点模式
#define MODE_STA  1  // 联网模式
#define WIFI_MODE_SELECT   MODE_AP



static const char *TAG = "BSP_WIFI";//用于应答
void wifi_init_softap(void)
{
    // 1. 初始化 NVS 闪存（Wi-Fi 底层必须用到它来存校准数据）
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. 初始化网络接口
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    
    esp_netif_create_default_wifi_ap();  //开启AP模式  热点
    // esp_netif_create_default_wifi_sta(); //开启STA模式

    esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");



if (ap_netif != NULL) 
    {
        esp_netif_ip_info_t ip_info;
        
        // 2. 使用高效率的 IP4_ADDR 宏进行硬件级数值拼接
        IP4_ADDR(&ip_info.ip, 192, 168, 100, 125);
        IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
        IP4_ADDR(&ip_info.gw, 192, 168, 100, 1); 

        // 关 DHCP -> 新 IP -> 开 DHCP 
        esp_netif_dhcps_stop(ap_netif);        
        esp_netif_set_ip_info(ap_netif, &ip_info);
        esp_netif_dhcps_start(ap_netif);       
        
        ESP_LOGI(TAG, "固定 IP: 192.168.100.125");
    } else {
        ESP_LOGE(TAG, "严重错误：未找到 WIFI_AP_DEF 对应的句柄！");
    }







    // 3. 配置并启动 Wi-Fi (AP热点模式)
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "YOZX-C6",   // Wi-Fi 名字
            .ssid_len = strlen("YOZX-C6"),
            .password = "12345678",        // Wi-Fi 密码
            .max_connection = 10,           // 最大连接数
            .authmode = WIFI_AUTH_WPA2_PSK // 加密方式

            //STAmode配置示例
            
        // .threshold.rssi = -127,         // 允许连接信号极弱的路由器
        // .scan_method = WIFI_FAST_SCAN,  // 快速扫描模式，连网速度极快
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));  //AP/STA
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config)); //AP STA
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi AP 启动成功! SSID: YOZX-C6, 密码: 12345678");
    ESP_LOGI(TAG, "TCP Server 默认 IP 地址为: 192.168.100.125");
}











//双模
// #include "bsp_wifi.h"
// #include "esp_wifi.h"
// #include "nvs_flash.h"
// #include "esp_log.h"
// #include <string.h>
// #include "esp_netif.h"
// #include "lwip/ip_addr.h"

// static const char *TAG = "BSP_WIFI";

// /* ==================== 模式一键切换控制开关 ==================== */
// #define MODE_AP   0  // 热点模式
// #define MODE_STA  1  // 蹭网/连办公室路由器模式

 
// #define WIFI_MODE_SELECT   MODE_AP 
// /* ============================================================= */


// void wifi_init_softap(void)
// {
//     // 1. 初始化 NVS 闪存
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);

//     // 2. 初始化网络接口
//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());

// /* -------------------- 自动切换网络接口 -------------------- */
// #if (WIFI_MODE_SELECT == MODE_AP)
//     esp_netif_create_default_wifi_ap();  
//     esp_netif_t *netif_handle = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
// #elif (WIFI_MODE_SELECT == MODE_STA)
//     esp_netif_create_default_wifi_sta(); 
//     esp_netif_t *netif_handle = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
// #endif

//     // 静态固定 IP 分配闭环
//     if (netif_handle != NULL) 
//     {
//         esp_netif_ip_info_t ip_info;
        
//         IP4_ADDR(&ip_info.ip, 192, 168, 100, 125);
//         IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);

// #if (WIFI_MODE_SELECT == MODE_AP)
//         IP4_ADDR(&ip_info.gw, 192, 168, 100, 125); 
        
//         esp_netif_dhcps_stop(netif_handle);        
//         esp_netif_set_ip_info(netif_handle, &ip_info);
//         esp_netif_dhcps_start(netif_handle);       
//         ESP_LOGI(TAG, "AP模式静态IP配置成功: 192.168.100.125");
        
// #elif (WIFI_MODE_SELECT == MODE_STA)
//         // STA 蹭网模式下，网关必须精准指向外部路由器的真实 IP（通常是 .1）
//         IP4_ADDR(&ip_info.gw, 192, 168, 100, 1); 
        
//         esp_netif_dhcpc_stop(netif_handle);        
//         esp_netif_set_ip_info(netif_handle, &ip_info);
//         // 注意：STA 模式下不需要再调用 dhcpc_start，因为已经是固定 IP 了
//         ESP_LOGI(TAG, "STA模式静态IP配置成功: 192.168.100.125");
// #endif
//     } 
//     else 
//     {
//         ESP_LOGE(TAG, "严重错误：未找到对应的网络网卡句柄！");
//     }

//     // 3. 配置并启动 Wi-Fi 射频天线
//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));

// /* -------------------- 自动切换天线配置表 -------------------- */
// #if (WIFI_MODE_SELECT == MODE_AP)
//     wifi_config_t wifi_config = {
//         .ap = {
//             .ssid = "esp32-s3",            // 满足上位机要求的 Wi-Fi 名字
//             .ssid_len = strlen("esp32-s3"),
//             .password = "12345678",        
//             .max_connection = 4,           
//             .authmode = WIFI_AUTH_WPA2_PSK 
//         },
//     };
//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP)); 
//     ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config)); 
//     ESP_ERROR_CHECK(esp_wifi_start());
//     ESP_LOGI(TAG, "Wi-Fi AP 热点模式启动成功! SSID: esp32-s3");

// #elif (WIFI_MODE_SELECT == MODE_STA)
//     wifi_config_t wifi_config = {
//         .sta = {
//             .ssid = "Office_WiFi",        // 办公室 Wi-Fi 名字
//             .password = "Office_123456",   // 办公室 Wi-Fi 密码
//             .threshold.rssi = -127,        
//             .scan_method = WIFI_FAST_SCAN, 
//         },
//     };
//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); 
//     ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config)); 
//     ESP_ERROR_CHECK(esp_wifi_start());
//     esp_wifi_connect(); // 呼叫天线开始连接外部路由器
//     ESP_LOGI(TAG, "Wi-Fi STA 蹭网模式启动成功，正在连接路由器...");
// #endif
// }