#include <reg52.h>
#include "config.h"
#include "delay.h"

/* 1ms 定时初值: 65536 - (晶振/12/1000) 个机器周期 */
#define T0_RELOAD (65536UL - (FOSC / 12UL / 1000UL))

static volatile unsigned int sys_tick = 0; /* 开机以来的毫秒数 */

void timer0_init(void)
{
    TMOD &= 0xF0; /* 定时器0 工作在模式1(16 位定时) */
    TMOD |= 0x01;

    TH0 = (unsigned char)(T0_RELOAD >> 8);
    TL0 = (unsigned char)(T0_RELOAD);

    ET0 = 1; /* 允许定时器0 中断 */
    EA = 1;  /* 开总中断 */
    TR0 = 1; /* 启动定时器 */
}

void timer0_isr(void) interrupt 1
{
    TH0 = (unsigned char)(T0_RELOAD >> 8); /* 模式1 需要手动重装 */
    TL0 = (unsigned char)(T0_RELOAD);
    sys_tick++;
}

unsigned int tick_get(void)
{
    unsigned int t;

    ET0 = 0; /* 16 位变量读取需要保护, 防止被中断打断后读到半新半旧的值 */
    t = sys_tick;
    ET0 = 1;

    return t;
}

void delay_ms(unsigned int ms)
{
    unsigned int start;

    if (ms == 0)
        return;

    start = tick_get();
    while ((unsigned int)(tick_get() - start) < ms)
        ;
}
