#include "bsp_tcp_connect.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp_wifi.h"
#include "bsp_parse.h"

static const char *TAG = "BSP_TCP";
volatile int g_active_tcp_sock = -1; // -1 代表当前没有手机/上位机连接TCP/IP




 /*******************************************************
 Author: PAN       Version: V1.0       Date:2026/06/15
 Function:          BSP_TCP_Server_Init
 Description:       初始化TCP服务器，创建Socket并绑定指定端口开始监听
 Input:             port - 要监听的端口号
 Output:            无
 Return:            listen_sock - 监听套接字描述符，失败返回-1
 Others:            无
*******************************************************/
int BSP_TCP_Server_Init(uint16_t port)
{
    // 1. 创建 Socket
    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Socket 创建失败");
        return -1;
    }

    // 端口复用
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2. 绑定端口
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    
    if (bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        ESP_LOGE(TAG, "Socket 绑定失败");
        close(listen_sock);
        return -1;
    }

    // 3. 开始监听
    if (listen(listen_sock, 1) < 0) {
        ESP_LOGE(TAG, "监听失败！");
        close(listen_sock);
        return -1;
    }
    
    ESP_LOGI(TAG, "TCP Server 正在监听 %d 端口, 等待连接...", port);
    return listen_sock; // 反馈结果
}

 /*******************************************************
 Author: PAN       Version: V1.0       Date:2026/06/15
 Function:         BSP_TCP_Accept_Client
 Description:       阻塞等待客户端连接 
 Input:             listen_sock - 由BSP_TCP_Server_Init返回的监听套接字
 Output:            sock
 Return:            成功返回与客户端通信的套接字描述符(sock)，失败返回 -1
 Others:            无
*******************************************************/
int BSP_TCP_Accept_Client(int listen_sock)
{
     
    struct sockaddr_storage source_addr;
    socklen_t addr_len = sizeof(source_addr);
    
    // 4. 阻塞等待客户端连接
    int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
    if (sock < 0) {
        ESP_LOGE(TAG, "接受连接失败，重试...");
        return -1; //  退出函数 
    }
    
    ESP_LOGI(TAG, "客户端已连接！");
    g_active_tcp_sock = sock;// 给全局句柄赋值
    
    //=================   开启底层 TCP Keep-Alive 机制 =================
    int keepAlive = 1;      // 1. 开启 Keep-Alive
    int keepIdle = 5;       // 2. 如果 5 秒钟内双方没有任何数据通信，ESP32  
    int keepInterval = 2;   // 3. 每隔 2 秒钟，ESP32 底层发送探测包
    int keepCount = 3;      // 4. 3次没有回复消息ACK
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE,  &keepAlive, sizeof(keepAlive));  //开启心跳包
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(keepIdle));   //配置时间
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(keepInterval)); //配置试探间隔
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(keepCount));  //次数

    
     return sock; //返回句柄
}