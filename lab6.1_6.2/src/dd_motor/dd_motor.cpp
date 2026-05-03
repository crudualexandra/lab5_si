#include "dd_motor.h"
#include <Arduino.h>

typedef struct
{
    int     pinEN;
    int     pinIN1;
    int     pinIN2;
    uint8_t speedPct;
    bool    forward;
    bool    initialised;
} MotorSlot_t;

static MotorSlot_t slots[DD_MOTOR_MAX_COUNT];

void hwMotorInit(int id, int pinEN, int pinIN1, int pinIN2)
{
    if (id < 0 || id >= DD_MOTOR_MAX_COUNT) return;

    slots[id].pinEN  = pinEN;
    slots[id].pinIN1 = pinIN1;
    slots[id].pinIN2 = pinIN2;
    slots[id].speedPct    = 0;
    slots[id].forward     = true;
    slots[id].initialised = true;

    pinMode(pinEN,  OUTPUT);
    pinMode(pinIN1, OUTPUT);
    pinMode(pinIN2, OUTPUT);

    analogWrite(pinEN, 0);
    digitalWrite(pinIN1, LOW);
    digitalWrite(pinIN2, LOW);
}

void hwMotorSetSpeed(int id, uint8_t speedPct, bool forward)
{
    if (id < 0 || id >= DD_MOTOR_MAX_COUNT) return;
    if (!slots[id].initialised) return;

    if (speedPct > 100) speedPct = 100;

    slots[id].speedPct = speedPct;
    slots[id].forward  = forward;

    if (forward)
    {
        digitalWrite(slots[id].pinIN1, HIGH);
        digitalWrite(slots[id].pinIN2, LOW);
    }
    else
    {
        digitalWrite(slots[id].pinIN1, LOW);
        digitalWrite(slots[id].pinIN2, HIGH);
    }

    analogWrite(slots[id].pinEN, (uint8_t)((uint16_t)speedPct * 255u / 100u));
}

void hwMotorStop(int id)
{
    if (id < 0 || id >= DD_MOTOR_MAX_COUNT) return;
    if (!slots[id].initialised) return;

    analogWrite(slots[id].pinEN, 0);
    digitalWrite(slots[id].pinIN1, LOW);
    digitalWrite(slots[id].pinIN2, LOW);
    slots[id].speedPct = 0;
}

uint8_t hwMotorGetSpeed(int id)
{
    if (id < 0 || id >= DD_MOTOR_MAX_COUNT) return 0;
    return slots[id].speedPct;
}

bool hwMotorGetDirection(int id)
{
    if (id < 0 || id >= DD_MOTOR_MAX_COUNT) return true;
    return slots[id].forward;
}
