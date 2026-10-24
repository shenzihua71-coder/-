/*======================================================================
 * 智能门锁  main.c
 *----------------------------------------------------------------------
 * 硬件接线:
 *   LCD1602 : RS=P2.6  RW=P2.5  EN=P2.7   D0~D7=P0
 *   4x4键盘 : P3.7~P3.4 为列, P3.3~P3.0 为行
 *   继电器  : P2.0 (低电平吸合, 见 config.h)
 *   晶振    : 11.0592MHz
 *----------------------------------------------------------------------
 * 功能:
 *   1. 输入 6 位密码, 输满 6 位自动校验, 也可按 '#' 提交
 *   2. '*' 退格, 'A' 修改密码, 'B' 取消
 *   3. 连续输错 3 次锁定 30 秒
 *   4. 开锁后继电器吸合 5 秒自动上锁, LCD 显示倒计时
 *   5. 15 秒无按键自动清除已输入内容
 *====================================================================*/
#include <reg52.h>
#include "config.h"
#include "delay.h"
#include "lcd.h"
#include "keyboard.h"
#include "jdq.h"

/* 密码操作所处的阶段 */
#define ST_IDLE      0 /* 等待输入开门密码 */
#define ST_CHG_OLD   1 /* 修改密码: 输入旧密码 */
#define ST_CHG_NEW   2 /* 修改密码: 输入新密码 */
#define ST_CHG_AGAIN 3 /* 修改密码: 再次输入新密码 */

static char pwd[PWD_LEN + 1] = PWD_DEFAULT; /* 当前密码 */
static char buf[PWD_LEN + 1];               /* 本次输入 */
static char new_pwd[PWD_LEN + 1];           /* 待确认的新密码 */

static unsigned char state = ST_IDLE;
static unsigned char buf_len = 0;
static unsigned char err_cnt = 0;
static unsigned int last_key_tick = 0;

/*--------------------------------------------------------------------
 * 密码工具
 *------------------------------------------------------------------*/
static bit pwd_equal(char data *a, char data *b)
{
    unsigned char i;

    for (i = 0; i < PWD_LEN; i++)
    {
        if (a[i] != b[i])
            return 0;
    }
    return 1;
}

static void pwd_copy(char data *dst, char data *src)
{
    unsigned char i;

    for (i = 0; i < PWD_LEN; i++)
        dst[i] = src[i];
    dst[PWD_LEN] = '\0';
}

/*--------------------------------------------------------------------
 * 界面
 *------------------------------------------------------------------*/
/* 按当前阶段重画整个界面, 并清空输入 */
static void ui_stage(void)
{
    lcd_clear();

    switch (state)
    {
    case ST_CHG_OLD:
        lcd_puts(1, 1, "Old password:");
        break;
    case ST_CHG_NEW:
        lcd_puts(1, 1, "New password:");
        break;
    case ST_CHG_AGAIN:
        lcd_puts(1, 1, "Repeat new:");
        break;
    default:
        lcd_puts(1, 1, "SMART LOCK");
        break;
    }

    if (state == ST_IDLE)
        lcd_puts(2, 1, "Enter password");
    else
        lcd_puts(2, 1, "PWD: ");

    buf_len = 0;
    last_key_tick = tick_get();
}

/* 只刷新第 2 行 "PWD: ***" */
static void ui_input(void)
{
    unsigned char i;

    lcd_clear_line(2);
    lcd_puts(2, 1, "PWD: "); /* 占第 1~5 列 */
    for (i = 0; i < buf_len; i++)
        lcd_putchar(2, (unsigned char)(6 + i), '*');
}

static void to_idle(void)
{
    state = ST_IDLE;
    ui_stage();
}

/*--------------------------------------------------------------------
 * 业务动作
 *------------------------------------------------------------------*/
/* 在指定位置倒计时显示秒数, 每秒刷新一次 */
static void countdown(unsigned char line, unsigned char col, unsigned char sec)
{
    for (; sec > 0; sec--)
    {
        lcd_putnum(line, col, sec, 2);
        lcd_putchar(line, (unsigned char)(col + 2), 's');
        delay_ms(1000);
    }
}

static void do_unlock(void)
{
    err_cnt = 0;
    lock_open();

    lcd_clear();
    lcd_puts(1, 1, "DOOR OPENED");
    lcd_puts(2, 1, "Relock in ");
    countdown(2, 11, UNLOCK_SEC); /* 第 11~12 列显示秒数 */

    lock_close();
    lcd_clear();
    lcd_puts(1, 1, "DOOR LOCKED");
    delay_ms(1200);

    to_idle();
}

static void do_wrong(void)
{
    lcd_clear();
    lcd_puts(1, 1, "ACCESS DENIED");
    lcd_puts(2, 1, "Tries left: ");
    lcd_putnum(2, 13, (unsigned int)(MAX_TRY - err_cnt), 1);
    delay_ms(1500);

    ui_stage(); /* 回到输入界面, 输入已被清空 */
}

static void do_lockout(void)
{
    err_cnt = 0;

    lcd_clear();
    lcd_puts(1, 1, "LOCKED OUT");
    lcd_puts(2, 1, "Wait ");
    countdown(2, 6, LOCKOUT_SEC);

    ui_stage();
}

static void do_change_start(void)
{
    state = ST_CHG_OLD;
    ui_stage();
}

/*--------------------------------------------------------------------
 * 提交一次完整输入(6 位)
 *------------------------------------------------------------------*/
static void submit(void)
{
    if (buf_len == 0)
        return;

    if (buf_len != PWD_LEN)
    {
        lcd_clear_line(2);
        lcd_puts(2, 1, "Need 6 digits");
        delay_ms(1200);
        ui_input(); /* 恢复已经输入的星号, 不用重新输 */
        return;
    }

    switch (state)
    {
    case ST_IDLE:
        if (pwd_equal(buf, pwd))
        {
            do_unlock();
        }
        else
        {
            err_cnt++;
            if (err_cnt >= MAX_TRY)
                do_lockout();
            else
                do_wrong();
        }
        break;

    case ST_CHG_OLD:
        if (pwd_equal(buf, pwd))
        {
            state = ST_CHG_NEW;
            ui_stage();
        }
        else
        {
            lcd_clear();
            lcd_puts(1, 1, "Old pwd wrong");
            delay_ms(1500);
            to_idle();
        }
        break;

    case ST_CHG_NEW:
        pwd_copy(new_pwd, buf);
        state = ST_CHG_AGAIN;
        ui_stage();
        break;

    default: /* ST_CHG_AGAIN */
        if (pwd_equal(buf, new_pwd))
        {
            pwd_copy(pwd, new_pwd);
            err_cnt = 0;
            lcd_clear();
            lcd_puts(1, 1, "Password OK");
            delay_ms(1500);
        }
        else
        {
            lcd_clear();
            lcd_puts(1, 1, "Mismatch");
            delay_ms(1500);
        }
        to_idle();
        break;
    }
}

/*--------------------------------------------------------------------
 * 主循环
 *------------------------------------------------------------------*/
void main(void)
{
    unsigned char key;

    timer0_init(); /* 先建立 1ms 节拍, 后面所有延时都依赖它 */
    lock_init();   /* 上电先确保锁是关的 */
    lcd_init();

    state = ST_IDLE;
    ui_stage();

    while (1)
    {
        key = key_get();

        if (key == KEY_NONE)
        {
            /* 长时间没有按键: 清掉已输入的内容, 防止别人接着输入 */
            if (buf_len > 0 &&
                (unsigned int)(tick_get() - last_key_tick) > IDLE_TIMEOUT_MS)
            {
                ui_stage();
            }
            continue;
        }

        last_key_tick = tick_get();

        if (key >= '0' && key <= '9')
        {
            if (buf_len < PWD_LEN)
            {
                buf[buf_len] = (char)key;
                buf_len++;
                ui_input();

                if (buf_len == PWD_LEN) /* 输满 6 位自动校验 */
                    submit();
            }
        }
        else if (key == '*') /* 退格 */
        {
            if (buf_len > 0)
            {
                buf_len--;
                ui_input();
            }
        }
        else if (key == '#') /* 手动提交 */
        {
            submit();
        }
        else if (key == 'A') /* 修改密码 */
        {
            if (state == ST_IDLE && buf_len == 0)
                do_change_start();
        }
        else if (key == 'B') /* 取消 */
        {
            if (state != ST_IDLE)
                to_idle();
        }
    }
}
