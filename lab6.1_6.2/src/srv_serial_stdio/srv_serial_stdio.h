#ifndef SRV_SERIAL_STDIO_H
#define SRV_SERIAL_STDIO_H

#include <stdio.h>

void sysSerialInit();
int  sysSerialPutChar(char c, FILE *stream);
int  sysSerialGetChar(FILE *stream);

#endif
