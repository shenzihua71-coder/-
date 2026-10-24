#ifndef __LCD_H__
#define __LCD_H__

#include <reg52.h>

/* LCD1602 接线: RS=P2.6  RW=P2.5  EN=P2.7  数据线=P0 */
sbit lcd_rs = P2 ^ 6;
sbit lcd_rw = P2 ^ 5;
sbit lcd_en = P2 ^ 7;

#define LCD_DATA P0
#define LCD_WIDTH 16 /* 每行 16 个字符 */

void lcd_init(void);

void lcd_clear(void);                             /* 清屏 */
void lcd_clear_line(unsigned char line);          /* 清除某一行(1 或 2) */
void lcd_goto(unsigned char line, unsigned char col);
void lcd_putchar(unsigned char line, unsigned char col, char c);
void lcd_puts(unsigned char line, unsigned char col, char code *s); /* 字符串超出本行自动截断 */
void lcd_putnum(unsigned char line, unsigned char col, unsigned int num, unsigned char width);

#endif
