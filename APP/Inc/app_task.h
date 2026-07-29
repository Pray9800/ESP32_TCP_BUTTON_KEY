



#ifndef __APP_TASK_H_
#define __APP_TASK_H_


#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define rtos_mode 0
#define blink_fre_100ms   50//led 闪烁频率
// #define BRIGHTNESS 100   //亮度


// ==================== 网络命令 CMD 宏定义 ====================
#define CMD_SET_COLOR    0x0A  // 灯光颜色设定 / 按键状态上报  
#define CMD_SET_BRIGHT   0x0B  // 灯光亮度设定  
#define CMD_ACK_COLOR    0x0C  // 灯光控制应答  
#define CMD_BUT_CHECK    0x0D  // 按键状态查询  
#define CMD_SYS_RESET    0x0F  // 系统软复位 

//  序列号配置相关指令
#define CMD_SET_SN       0x11  // 写入序列号（触发重启）
#define CMD_RESET_SN     0x12  // 恢复出厂序列号（触发重启）
#define CMD_GET_SN       0x13  // 读取当前序列号
//用于更改wifi的名称
#define NVS_NAMESPACE_WIFI  "wifi_cfg"      // NVS 命名空间
#define NVS_KEY_SN          "sn"            // NVS 序列号 Key



// ==================== UR机械臂  WIFI连接版本网络初始化配置 ====================
#define C6_WIFI_SSID     "YOZX-C6"        // Wi-Fi 名称
#define C6_SSID_PREFIX   "YOZX-"        // SSID 固定前缀
#define DEFAULT_SN       "00000000"     // 默认 8 位序列号
#define C6_WIFI_PASS      "12345678"       // Wi-Fi 密码

#define C6_SERVER_IP          "192.168.100.125"// ESP32 本机静态IP
#define C6_SERVER_NETMASK     "255.255.255.0"  // 子网掩码
#define C6_SERVER_GW          "192.168.100.1"  // 网关

#define C6_TCP_PORT           8238             // 监听端口


extern TaskHandle_t xWsLightTaskHandle;
extern volatile uint8_t g_key_query_flag;//按键查询标志位
//初始化并启动 APP 层的全部 RTOS 任务
 
void app_task_init(void);
void vTask_TCP_Server(void *pvParameters);  //TCP连接单独一个文件

#endif /* __APP_TASK_H_ */




/**
 * 硬件与网络资源映射
 * ============================================================
 * WIFI   →  SoftAP 热点模式 (SSID: YOZX-C6)
 *         支持通过配置命令修改热点名称（SSID），并可与序列号配置联动
 * TCP    →  Server 监听模式 (Port: 8238, TCP_NODELAY 开启)
 * SPI2   →  WS2812 灯带驱动 (DMA 异步全双工, MOSI: IO38)
 * GPIO   →  本地独立按键矩阵 (4路按键)
 *
 *
 * TCP 网络通信协议 (TCP Client ↔ ESP32)
 * ============================================================
 * [帧格式] A5 5A + CMD + LEN + DATA + B6 6B
 *
 * [上行指令 (ESP32 主动上报)]
 * 0x0A : 按键状态上报 (00:释放, 01~04:对应按键触发)
 * 0x0C : 灯光控制应答 (回传上位机下发的参数，确认执行)
 *
 * [下行指令 (上位机 下发指令)]
 * 0x0A : 灯光颜色设定 (1:白光 2:蓝光 3:绿光 4:红常亮 5:红闪)
 * 0x0B : 灯光亮度设定 (0~255 全局亮度倍率)
 * 0x0F : 系统软复位   (数据域 0x01 触发 esp_restart)
 *
 *
 * 指令与数据流向
 * │
 * vTask_TCP_Server 
 * (阻塞等待 recv)
 * │
 * Protocol_Parse_Byte 
 * (单字节安全状态机)
 * │
 * ┌──────────────────────┼──────────────────────┐
 * ▼                      ▼                      ▼
 * 0x0A / 0x0B               0x0F                   其他异常包
 * 通知灯光控制任务          触发硬件级重启             丢弃/防越界
 * (xTaskNotifyGive)       (esp_restart)
 *
 *
 * 本地按键 GPIO 输入 (防粘包机制)
 * ============================================================
 * KEY1 ~ KEY4 → 状态变化即刻触发底层 TCP send()
 * → 发送成功后强制挂起任务 20ms，从物理层阻断 TCP 粘包
 *
 *
 * FreeRTOS 任务优先级 (高 → 低)
 * ============================================================
 * vTask_Key_Sig        : Priority 7 (Highest) — 按键极速扫描、去抖与状态上报
 * vTask_TCP_Server     : Priority 6           — WiFi 监听、TCP 接收与协议逐字节解析
 * vTask_WsLight_Change : Priority 5           — WS2812 状态机 (含 500ms 异步闪烁与 10ms 硬件节拍)
 * vTask_Led_Blink      : Priority 4 (Lowest)  — 系统运行指示灯心跳 (100ms 周期)
 *
 *
 * RTOS 核心资源一览
 * ============================================================
 * [互斥锁 Mutex]
 * xTcpSendMutex      — TCP 底层发送通道锁。强制按键上报与灯带应答排队，
 * 防止多任务并发争抢 send() 导致底层网络报文交叠乱码。
 *
 * [任务通知 Task Notify]
 * xWsLightTaskHandle — 接收解析器的合法灯光指令瞬间唤醒。
 * (超时阈值设为 10ms，兼作红灯状态机的时基节拍器)
 * ============================================================
 */