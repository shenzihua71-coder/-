#include <reg52.h>
#include "led.h"

/* LED 与继电器、LCD 共用 P2, 只能用位操作, 不能整口赋值 */
void led_open(unsigned char num)
{
    if (num < 2 || num > 5) /* LED1 是继电器, LED6~LED8 是 LCD 控制线 */
        return;

    P2 &= (unsigned char)~(1 << (num - 1)); /* 低电平点亮 */
}

void led_close(unsigned char num)
{
    if (num < 2 || num > 5)
        return;

    P2 |= (unsigned char)(1 << (num - 1));
}

void led_all_close(void)
{
    P2 |= 0x1E; /* P2.1~P2.4 置高, 不影响继电器和 LCD */
}
