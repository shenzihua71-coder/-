#ifndef _LED_H_
#define _LED_H_

#include <reg52.h>

/* 板上 8 个 LED 接 P2.0~P2.7, 低电平点亮.
 * 注意: P2.0 已经用作继电器 LOCK, P2.5~P2.7 已经用作 LCD 控制线,
 *       所以真正能安全使用的只有 LED2~LED5 (P2.1~P2.4).
 * 本门锁程序用 LCD 显示状态, 没有使用 LED, 这里保留驱动方便扩展. */
void led_open(unsigned char num);  /* 点亮 LED2~LED5 */
void led_close(unsigned char num); /* 熄灭 LED2~LED5 */
void led_all_close(void);          /* 熄灭 LED2~LED5 */

#endif
