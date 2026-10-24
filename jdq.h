#ifndef __JDQ_H__
#define __JDQ_H__

#include <reg52.h>

/* 继电器控制脚. 上电默认输出高电平, 配合 config.h 的 LOCK_ACTIVE_LOW=1
 * 可以保证单片机复位瞬间门锁处于关闭状态 */
sbit LOCK = P2 ^ 0;

void lock_init(void);  /* 上电初始化, 确保继电器断开 */
void lock_open(void);  /* 开锁: 继电器吸合 */
void lock_close(void); /* 上锁: 继电器断开 */

#endif
