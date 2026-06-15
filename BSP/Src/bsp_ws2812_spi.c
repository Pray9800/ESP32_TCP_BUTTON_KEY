#include "bsp_ws2812_spi.h"
#include "driver/spi_master.h"
#include "string.h"

// 声明 SPI 设备句柄
static spi_device_handle_t spi_ws2812_handle;

// DMA 发送缓冲区（ESP32 的全局数组默认在内部 SRAM，完全支持 DMA）
uint8_t ws_spi_data[WS_SPI_BUF_SIZE] = {0};

/*******************************************************
 Function:      ws2812_spi_ini
 
 t
 Description:   初始化 ESP32 SPI 主机、映射引脚并配置 DMA
*******************************************************/
void ws2812_spi_init(void)
{
    // 1. 配置 SPI 总线
    spi_bus_config_t buscfg = {
        .mosi_io_num = WS2812_PIN, // 核心：将 MOSI 映射到 IO38
        .miso_io_num = -1,         // 不使用 MISO
        .sclk_io_num = -1,         // 不使用时钟线 (WS2812是单总线异步通信)
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = WS_SPI_BUF_SIZE // 最大传输大小
    };

    // 2. 配置 SPI 设备参数 (6MHz)
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 6 * 1000 * 1000, // 完美匹配你注释里的 6.0MHz
        .mode = 0,                         // SPI 模式 0
        .spics_io_num = -1,                // 不使用片选线
        .queue_size = 1,                   // 事务队列大小
    };

    // 3. 初始化 SPI 总线 (使用 SPI2，启用自动分配 DMA 通道)
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    
    // 4. 添加设备到 SPI 总线
    spi_bus_add_device(SPI2_HOST, &devcfg, &spi_ws2812_handle);

    // 全局清零，其中末尾的字节发送 0x00 相当于长时间低电平，触发 WS2812 锁存
    memset(ws_spi_data, 0x00, WS_SPI_BUF_SIZE);
}

/*******************************************************
 Function:      WS_Set_Color_Spi
 Description:   将单个 WS2812 灯珠的 RGB 颜色转换为 SPI 发送编码
*******************************************************/
static void WS_Set_Color_Spi(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if(index >= WS_ARRAY_SIZE) return; 
    
    // WS2812B 数据顺序是 G - R - B
    uint32_t color = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
    uint16_t start_idx = index * 24;
    
    for (int i = 0; i < 24; i++)
    {
        if (color & (1 << (23 - i))) {
            ws_spi_data[start_idx + i] = WS_CODE_1;
        } else {
            ws_spi_data[start_idx + i] = WS_CODE_0;
        }
    }
}

/*******************************************************
 Function:      ws2812_refresh_spi
 Description:   通过 DMA 启动 SPI 发送缓冲区数据
*******************************************************/
void ws2812_refresh_spi(void)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    
    t.length = WS_SPI_BUF_SIZE * 8; // 长度单位是 bit
    t.tx_buffer = ws_spi_data;      // 数据指针

    // spi_device_transmit 会自动阻塞当前任务，直到 DMA 传输彻底完成
    // 所以不需要像 STM32 那样写 while(HAL_SPI_GetState() != READY)
    spi_device_transmit(spi_ws2812_handle, &t);
}

/*******************************************************
 Function:      ws2812_set_num_spi
 Description:   将前 num 颗 WS2812 灯珠设置为指定颜色并立即刷新输出
*******************************************************/
void ws2812_set_num_spi(uint16_t num, uint8_t r, uint8_t g, uint8_t b)
{
    if (num > WS_ARRAY_SIZE) num = WS_ARRAY_SIZE;
    
    // 清空前面的颜色缓存 (后面的 0x00 锁存复位信号保留)
    memset(ws_spi_data, WS_CODE_0, WS_ARRAY_SIZE * 24);
    
    for (uint16_t i = 0; i < num; i++)
    {
        WS_Set_Color_Spi(i, r, g, b);
    }
    
    ws2812_refresh_spi();
}

/*******************************************************
 Function:      ws2812_rgb_all_spi
 Description:   将灯珠缓存为指定颜色，但不立即发送
*******************************************************/
void ws2812_rgb_all_spi(uint8_t ws_count, uint8_t r, uint8_t g, uint8_t b)  
{
    if (ws_count > WS_ARRAY_SIZE) ws_count = WS_ARRAY_SIZE;
    
    for(uint16_t i = 0; i < ws_count; i++)
    {
        WS_Set_Color_Spi(i, r, g, b);
    }
}