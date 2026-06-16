#ifndef __BSP_TCP_CONNECT_H__
#define __BSP_TCP_CONNECT_H__

#include <stdint.h>



extern volatile int g_active_tcp_sock;//申明状态
 
int BSP_TCP_Server_Init(uint16_t port);



 
int BSP_TCP_Accept_Client(int listen_sock);

#endif