#include <reg52.h>
#include <intrins.h>
#include "config.h"
#include "keyboard.h"
#include "delay.h"

/*
 * 4x4 矩阵键盘接在 P3:
 *        P3.7  P3.6  P3.5  P3.4      <- 列 C1~C4, 依次输出低电平扫描
 *   P3.0  1     2     3     A        <- 行 R1~R4, 作为输入, 按下读到 0
 *   P3.1  4     5     6     B
 *   P3.2  7     8     9     C
 *   P3.3  *     0     #     D
 *
 * 如果手上的键盘丝印排列不同, 只改下面这张表即可
 */
static unsigned char code key_map[16] = {
    '1', '2', '3', 'A',
    '4', '5', '6', 'B',
    '7', '8', '9', 'C',
    '*', '0', '#', 'D'
};

/* 返回 0~15 的键位(行*4+列), 没有按键返回 KEY_NONE */
static unsigned char key_scan_raw(void)
{
    unsigned char col = 0xEF; /* C1 输出低电平, 其余为高 */
    unsigned char rows;
    unsigned char i, j;

    for (i = 0; i < 4; i++)
    {
        P3 = col; /* 行口写 1 作为输入, 同时拉低一列 */
        _nop_();
        _nop_();
        _nop_();
        _nop_(); /* 等电平稳定 */

        rows = (unsigned char)(P3 & 0x0F);
        if (rows != 0x0F) /* 有行被拉低, 说明有键按下 */
        {
            for (j = 0; j < 4; j++)
            {
                if ((rows & (1 << j)) == 0)
                    return (unsigned char)(j * 4 + i);
            }
        }

        col = (unsigned char)((col << 1) | 0x01); /* 换下一列: EF->DF->BF->7F */
    }

    return KEY_NONE;
}

unsigned char key_get(void)
{
    static unsigned char last = KEY_NONE;   /* 上一次的原始扫描值 */
    static unsigned char stable = KEY_NONE; /* 消抖后的稳定值 */
    static unsigned int change_tick = 0;    /* 原始值发生变化的时刻 */

    unsigned char raw;
    unsigned int now;

    raw = key_scan_raw();
    now = tick_get();

    if (raw != last) /* 电平还在变化, 重新计时 */
    {
        last = raw;
        change_tick = now;
        return KEY_NONE;
    }

    if ((unsigned int)(now - change_tick) < KEY_DEBOUNCE_MS)
        return KEY_NONE;

    if (raw == stable) /* 稳定但已经不是新按键 */
        return KEY_NONE;

    stable = raw;
    if (raw == KEY_NONE) /* 松手, 不产生键值 */
        return KEY_NONE;

    return key_map[raw];
}
