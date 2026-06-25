#ifndef __BSP_IWDG_H_
#define __BSP_IWDG_H_

#include <stdint.h>

/*
 * @brief  初始化任务看门狗，并强制将【当前调用它的任务】加入监视名单
 * @param  timeout_sec: 狗饿死（触发系统重启）的超时时间（单位：秒）
 */
void BSP_IWDG_Global_Init(uint32_t timeout_sec);  

/*
 * @brief  喂狗（必须在被监视的任务的 while(1) 循环中定期调用）
 */
void BSP_IWDG_Add_Current_Task(void);            // 将当前任务加入监视
void BSP_IWDG_Feed(void);                        // 喂狗
#endif /* __BSP_IWDG_H_ */


