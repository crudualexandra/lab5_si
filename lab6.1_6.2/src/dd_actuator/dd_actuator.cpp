#include <Arduino.h>
#include "dd_actuator.h"

static int  pins[DD_ACTUATOR_MAX_COUNT];
static bool states[DD_ACTUATOR_MAX_COUNT];
static bool activeLow[DD_ACTUATOR_MAX_COUNT];

static inline bool isValidId(int id)
{
    return (id >= 0 && id < DD_ACTUATOR_MAX_COUNT);
}

static void applyOutput(int id, bool logicalOn)
{
    bool pinLevel = activeLow[id] ? !logicalOn : logicalOn;
    digitalWrite(pins[id], pinLevel ? HIGH : LOW);
}

void hwActuatorInit(int id, int pin, bool isActiveLow)
{
    if (!isValidId(id))
        return;

    pins[id]      = pin;
    states[id]    = false;
    activeLow[id] = isActiveLow;

    digitalWrite(pin, isActiveLow ? HIGH : LOW);
    pinMode(pin, OUTPUT);
}

void hwActuatorOn(int id)
{
    if (!isValidId(id))
        return;

    states[id] = true;
    applyOutput(id, true);
}

void hwActuatorOff(int id)
{
    if (!isValidId(id))
        return;

    states[id] = false;
    applyOutput(id, false);
}

void hwActuatorToggle(int id)
{
    if (!isValidId(id))
        return;

    if (states[id])
        hwActuatorOff(id);
    else
        hwActuatorOn(id);
}

bool hwActuatorGetState(int id)
{
    if (!isValidId(id))
        return false;

    return states[id];
}
