#ifndef __DELAY_H__
#define __DELAY_H__

/* 启动定时器0, 产生 1ms 系统节拍. 必须最先调用, 其他模块的延时都依赖它 */
void timer0_init(void);

/* 读取当前毫秒计数(约 65 秒回绕一圈, 用减法比较可正确处理回绕) */
unsigned int tick_get(void);

/* 毫秒级延时 */
void delay_ms(unsigned int ms);

#endif
