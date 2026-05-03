#ifndef DD_KEYPAD_H
#define DD_KEYPAD_H

#define DD_KEYPAD_ROWS 4
#define DD_KEYPAD_COLS 4

void hwKeypadInit();
char hwKeypadGetKey();
char hwKeypadWaitForKey();

#endif
