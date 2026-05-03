#ifndef DD_LCD_H
#define DD_LCD_H

#include <stdint.h>

void hwLcdInit(uint8_t address, uint8_t cols, uint8_t rows);
void hwLcdClear();
void hwLcdSetCursor(uint8_t col, uint8_t row);
void hwLcdPrintChar(char c);
void hwLcdPrint(const char* str);

#endif
