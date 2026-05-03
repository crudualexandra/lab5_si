#include "srv_lcd_keypad_stdio.h"
#include <stdio.h>
#include <Arduino.h>

#include "dd_lcd/dd_lcd.h"
#include "dd_keypad/dd_keypad.h"

#define KEY_CONFIRM '#'
static FILE* srvLcdKeypadStream;
static FILE srvLcdKeypadStreamStorage;

void sysLcdKeypadInit()
{
    fdev_setup_stream(&srvLcdKeypadStreamStorage,
                      sysLcdKeypadPutChar,
                      sysLcdKeypadGetChar,
                      _FDEV_SETUP_RW);

    srvLcdKeypadStream = &srvLcdKeypadStreamStorage;

    stdout = srvLcdKeypadStream;
    stdin = srvLcdKeypadStream;
}

int sysLcdKeypadPutChar(char c, FILE* stream)
{
    hwLcdPrintChar(c);
    return 0;
}

int sysLcdKeypadGetChar(FILE* stream)
{
    char key = hwKeypadWaitForKey();

    if (key == KEY_CONFIRM)
    {
        return '\n';
    }

    return (int)key;
}
