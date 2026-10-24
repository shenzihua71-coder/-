#include <reg52.h>
#include <intrins.h>
#include "lcd.h"
#include "delay.h"

/* 10 的幂表, 用于不依赖除法库的数字显示 */
static unsigned int code pow10[5] = {1, 10, 100, 1000, 10000};

/* 等待 LCD 空闲: 读忙标志 BF(D7).
 * 如果 RW 没有接好/读不到, 靠后面的 2ms 延时兜底, 程序不会卡死 */
static void lcd_wait_busy(void)
{
    unsigned char n = 255;

    LCD_DATA = 0xFF; /* P0 作输入前必须先写 1 */
    lcd_rs = 0;
    lcd_rw = 1;
    lcd_en = 1;
    _nop_();
    while ((LCD_DATA & 0x80) && --n)
        ;
    lcd_en = 0;

    if (n == 0)
        delay_ms(2);
}

static void lcd_write_cmd(unsigned char cmd)
{
    lcd_wait_busy();

    lcd_rs = 0;
    lcd_rw = 0;
    LCD_DATA = cmd; /* 先给数据, 再拉高 EN */
    lcd_en = 1;
    _nop_();
    lcd_en = 0;
}

static void lcd_write_data(unsigned char dat)
{
    lcd_wait_busy();

    lcd_rs = 1;
    lcd_rw = 0;
    LCD_DATA = dat;
    lcd_en = 1;
    _nop_();
    lcd_en = 0;
}

void lcd_init(void)
{
    lcd_rs = 0;
    lcd_rw = 0;
    lcd_en = 0;
    LCD_DATA = 0xFF;

    delay_ms(15); /* 上电后 LCD 需要 15ms 以上才能接受指令 */

    lcd_write_cmd(0x38); /* 8 位数据总线, 2 行显示, 5x7 点阵 */
    delay_ms(5);
    lcd_write_cmd(0x38); /* 连续三次, 确保进入 8 位模式 */
    delay_ms(1);
    lcd_write_cmd(0x38);

    lcd_write_cmd(0x0C); /* 开显示, 关光标, 不闪烁 */
    lcd_write_cmd(0x01); /* 清屏 */
    delay_ms(2);
    lcd_write_cmd(0x06); /* 写入后地址自动加 1, 整屏不移动 */
}

void lcd_clear(void)
{
    lcd_write_cmd(0x01);
    delay_ms(2); /* 清屏指令需要 1.64ms */
}

void lcd_goto(unsigned char line, unsigned char col)
{
    unsigned char addr;

    if (col < 1)
        col = 1;
    if (col > LCD_WIDTH)
        col = LCD_WIDTH;

    addr = (unsigned char)(col - 1);
    if (line == 2)
        addr = (unsigned char)(addr + 0x40);

    lcd_write_cmd((unsigned char)(0x80 | addr));
}

void lcd_putchar(unsigned char line, unsigned char col, char c)
{
    if (col < 1 || col > LCD_WIDTH)
        return;

    lcd_goto(line, col);
    lcd_write_data((unsigned char)c);
}

void lcd_puts(unsigned char line, unsigned char col, char code *s)
{
    if (col < 1 || col > LCD_WIDTH)
        return;

    lcd_goto(line, col);
    while (*s != '\0' && col <= LCD_WIDTH) /* 超出本行自动截断, 不会绕到下一行 */
    {
        lcd_write_data((unsigned char)(*s));
        s++;
        col++;
    }
}

void lcd_clear_line(unsigned char line)
{
    unsigned char i;

    lcd_goto(line, 1);
    for (i = 0; i < LCD_WIDTH; i++)
        lcd_write_data(' ');
}

void lcd_putnum(unsigned char line, unsigned char col, unsigned int num, unsigned char width)
{
    unsigned char i;
    unsigned int div;
    char d;

    if (width == 0 || width > 5)
        return;

    lcd_goto(line, col);
    for (i = width; i > 0; i--)
    {
        div = pow10[i - 1];
        d = '0';
        while (num >= div) /* 用减法代替除法, 省掉 16 位除法库 */
        {
            num -= div;
            d++;
        }
        lcd_write_data((unsigned char)d);
    }
}
