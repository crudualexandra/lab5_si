#include <Arduino.h>
#include "dd_led.h"

static int  ledPins[DD_LED_MAX_COUNT];
static bool ledStates[DD_LED_MAX_COUNT];

static inline bool isValidId(int ledId)
{
    return (ledId >= 0 && ledId < DD_LED_MAX_COUNT);
}

void hwLedInit(int ledId, int pin)
{
    if (!isValidId(ledId))
        return;

    ledPins[ledId]   = pin;
    ledStates[ledId] = false;

    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

void hwLedOn(int ledId)
{
    if (!isValidId(ledId))
        return;

    ledStates[ledId] = true;
    digitalWrite(ledPins[ledId], HIGH);
}

void hwLedOff(int ledId)
{
    if (!isValidId(ledId))
        return;

    ledStates[ledId] = false;
    digitalWrite(ledPins[ledId], LOW);
}

void hwLedToggle(int ledId)
{
    if (!isValidId(ledId))
        return;

    if (ledStates[ledId])
        hwLedOff(ledId);
    else
        hwLedOn(ledId);
}

bool hwLedIsOn(int ledId)
{
    if (!isValidId(ledId))
        return false;

    return ledStates[ledId];
}
