#ifndef __BSP_TCP_CONNECT_H__
#define __BSP_TCP_CONNECT_H__

#include <stdint.h>



extern volatile int g_active_tcp_sock;//申明状态
/*
 * @brief  初始化 TCP Server (IP、端口复用、绑定、监听)
 * @param  port: 要监听的端口号 (如 8080)
 * @return 成功返回监听套接字(listen_sock)，失败返回 -1
 */
int BSP_TCP_Server_Init(uint16_t port);

/*
 * @brief  阻塞等待客户端连接，并处理内层的数据收发
 * @param  listen_sock: 
 */
void BSP_TCP_Wait_And_Handle(int listen_sock);

#endif