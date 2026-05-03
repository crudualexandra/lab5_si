#ifndef SRV_LCD_KEYPAD_STDIO_H
#define SRV_LCD_KEYPAD_STDIO_H

#include <stdio.h>

void sysLcdKeypadInit();
int  sysLcdKeypadPutChar(char c, FILE* stream);
int  sysLcdKeypadGetChar(FILE* stream);

#endif
