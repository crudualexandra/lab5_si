#include "srv_serial_stdio.h"
#include <stdio.h>
#include <Arduino.h>

#ifdef SERIAL_FREERTOS_YIELD
#include <Arduino_FreeRTOS.h>
#endif

#define SERIAL_BAUD 9600

static FILE  srvSerialStreamStorage;
static FILE *srvSerialStream = NULL;

void sysSerialInit()
{
    Serial.begin(SERIAL_BAUD);

    fdev_setup_stream(&srvSerialStreamStorage,
                      sysSerialPutChar,
                      sysSerialGetChar,
                      _FDEV_SETUP_RW);

    srvSerialStream = &srvSerialStreamStorage;

    stdin  = srvSerialStream;
    stdout = srvSerialStream;
}

int sysSerialPutChar(char c, FILE *stream)
{
    if (c == '\n')
        sysSerialPutChar('\r', stream);

    Serial.write(c);
    return 0;
}

int sysSerialGetChar(FILE *stream)
{
    while (!Serial.available())
    {
#ifdef SERIAL_FREERTOS_YIELD
        vTaskDelay(1);
#endif
    }

    int c = Serial.read();

    if (c == '\r')
        c = '\n';

    return c;
}
