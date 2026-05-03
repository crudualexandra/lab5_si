#ifndef DD_LED_H
#define DD_LED_H

#define DD_LED_MAX_COUNT 4

void hwLedInit(int ledId, int pin);
void hwLedOn(int ledId);
void hwLedOff(int ledId);
void hwLedToggle(int ledId);
bool hwLedIsOn(int ledId);

#endif
