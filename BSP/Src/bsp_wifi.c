#include "bsp_wifi.h"
#include "esp_wifi.h"
#include "lwip/opt.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>
#include "esp_netif.h"
#include "lwip/ip_addr.h"
#include "app_task.h"
#include "esp_system.h"
#include "string.h"
#define MODE_AP   0  // 热点模式
#define MODE_STA  1  // 联网模式
#define WIFI_MODE_SELECT   MODE_AP





static const char *TAG = "BSP_WIFI";//用于应答





 /*******************************************************
 Author: PAN       Version: V1.0       Date:2026/07/28
 Function:          wifi_get_sn
 Description:       从NVS中读取WiFi模块序列号；若未找到或内容无效，则使用默认序列号
 Input:             sn_buf: 存储序列号的缓冲区
 Output:            sn_buf: 返回读取到的序列号或默认序列号
 Return:            ESP_OK: 成功读取或使用默认值
                    ESP_ERR_INVALID_ARG: 输入参数非法
 Others:            默认序列号由 DEFAULT_SN 定义，确保首次启动时有可用值
*******************************************************/
esp_err_t wifi_get_sn(char *sn_buf, size_t max_len)
{   
    // 检查输入参数的有效性
    if (sn_buf == NULL || max_len < 9) {
        return ESP_ERR_INVALID_ARG;
    }  

    nvs_handle_t nvs_handle;
    //用于第一次启动
    //读取NVS中的序列号，如果没有找到则使用默认值
    esp_err_t err = nvs_open(NVS_NAMESPACE_WIFI, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        // NVS 未找到或未写入过，使用默认序列号 默认为八个0的序列号 原因在于
        strncpy(sn_buf, DEFAULT_SN, max_len); //保存操作后的sn_buf
        return ESP_OK;
    }
     //用于重启
    size_t required_size = max_len;
    err = nvs_get_str(nvs_handle, NVS_KEY_SN, sn_buf, &required_size);
    nvs_close(nvs_handle); //释放nvs_handle资源 解除绑定
    //写入的有问题 或者不对 还是保持原来的8个0 
    if (err != ESP_OK || strlen(sn_buf) != 8) {
        strncpy(sn_buf, DEFAULT_SN, max_len);
    }
    return ESP_OK;
}


 /*******************************************************
 Author: PAN       Version: V1.0       Date:2026/07/28
 Function:          wifi_set_sn
 Description:       将指定的WiFi模块序列号写入NVS中，供下次启动时读取
 Input:             sn_str: 需要保存的8位序列号字符串
 Output:            无
 Return:            ESP_OK: 成功写入NVS
                    ESP_ERR_INVALID_ARG: 输入参数非法
                    其他ESP错误码: NVS打开或提交失败
 Others:            序列号长度必须为8位，且会通过 nvs_commit 提交保存
*******************************************************/
esp_err_t wifi_set_sn(const char *sn_str)
{
    if (sn_str == NULL || strlen(sn_str) != 8) {
        return ESP_ERR_INVALID_ARG;
    }  //只需要八位的

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE_WIFI, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS Open Failed: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_str(nvs_handle, NVS_KEY_SN, sn_str);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }
    nvs_close(nvs_handle);
    return err;
}
//初始化

esp_err_t wifi_reset_sn(void)
{
    return wifi_set_sn(DEFAULT_SN);
}






 /*******************************************************
 Author: PAN       Version: V1.0       Date:2026/06/15
 Function:          wifi_init_softap
 Description:       初始化WiFi AP热点模式，配置NVS、网络接口、静态IP及DHCP，启动热点
 Input:             无
 Output:            无
 Return:            无
 Others:            热点SSID: YOZX-C6, 密码: 12345678, 固定IP: 192.168.100.125
*******************************************************/
void wifi_init_softap(void)
{
    // 1. 初始化 NVS 闪存（Wi-Fi 底层必须用到它来存校准数据）
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);


    // ------------------ 【wifi改动：读取 NVS 序列号并拼接 SSID】 ------------------
    char sn_str[10] = {0};
    wifi_get_sn(sn_str, sizeof(sn_str)); // 读取 8 位 SN，读不到默认 "00000000"
    char full_ssid[32] = {0};
    strcpy(full_ssid,C6_SSID_PREFIX); //组装前缀
    strncat(full_ssid,sn_str,8); //现在8位
    //序列号改成不是8位的时候采用下面函数做拼接
   // snprintf(full_ssid, sizeof(full_ssid), "%s%s", C6_SSID_PREFIX, sn_str); // 拼接为 "YOZX-20260001"
    // -----------------------------------------------------------------------------




    // 2. 初始化网络接口
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    
    esp_netif_create_default_wifi_ap();  //开启AP模式  热点
    // esp_netif_create_default_wifi_sta(); //开启STA模式

    esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");



if (ap_netif != NULL) 
    {
        esp_netif_ip_info_t ip_info;
        
        // 传入设定IP 掩码 公网
        esp_netif_str_to_ip4(C6_SERVER_IP, &ip_info.ip); 
        esp_netif_str_to_ip4(C6_SERVER_NETMASK, &ip_info.netmask);
        esp_netif_str_to_ip4(C6_SERVER_GW, &ip_info.gw);

        // 关 DHCP -> 新 IP -> 开 DHCP 
        esp_netif_dhcps_stop(ap_netif);        
        esp_netif_set_ip_info(ap_netif, &ip_info);
        esp_netif_dhcps_start(ap_netif);       
        
        ESP_LOGI(TAG, "固定 IP: 192.168.100.125");
    } else {
        ESP_LOGE(TAG, "严重错误：未找到 WIFI_AP_DEF 对应的句柄 ");
    }

    // 3. 配置并启动 Wi-Fi (AP热点模式)
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            //省略 改用拼接模式
            // .ssid = C6_WIFI_SSID,                  // wifi名称 
            // .ssid_len = strlen(C6_WIFI_SSID),      // 长度
            .password = C6_WIFI_PASS,              // 密码
            .max_connection = 10,           // 最大连接数
            .authmode = WIFI_AUTH_WPA2_PSK // 加密方式

            //STAmode配置示例           
        // .threshold.rssi = -127,         // 允许连接信号极弱的路由器
        // .scan_method = WIFI_FAST_SCAN,  // 快速扫描模式，连网速度极快
        },
    };

    //  将动态拼接好的 full_ssid 复制给 wifi_config 结构体 
    //把十六禁止强制转换为字符串
    //wifi_config.ap.ssid 是专门用于保存wifi 名称的寄存器 
    strncpy((char *)wifi_config.ap.ssid, full_ssid, sizeof(wifi_config.ap.ssid));
    wifi_config.ap.ssid_len = strlen(full_ssid);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));  //AP/STA
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config)); //AP STA
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_set_ps(WIFI_PS_NONE);  // 不省电

    ESP_LOGI(TAG, "Wi-Fi AP 启动成功! 真实 SSID: %s, 密码: %s", full_ssid, C6_WIFI_PASS);
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