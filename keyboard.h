#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#define KEY_NONE 0xFF /* 没有新按键 */

/* 扫描一次键盘.
 * 有新按键被按下时返回键值字符('0'~'9' '*' '#' 'A'~'D'),
 * 松开或没有按键时返回 KEY_NONE, 同一个键按住只返回一次 */
unsigned char key_get(void);

#endif
