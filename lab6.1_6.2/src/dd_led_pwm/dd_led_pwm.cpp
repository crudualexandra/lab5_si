#include <Arduino.h>
#include "dd_led_pwm.h"

static int     pwmPins[DD_LED_PWM_MAX_COUNT];
static uint8_t pwmDuty[DD_LED_PWM_MAX_COUNT];

static inline bool isValidId(int id)
{
    return (id >= 0 && id < DD_LED_PWM_MAX_COUNT);
}

void hwLedPwmInit(int id, int pin)
{
    if (!isValidId(id))
        return;

    pwmPins[id] = pin;
    pwmDuty[id] = 0;

    pinMode(pin, OUTPUT);
    analogWrite(pin, 0);
}

void hwLedPwmWrite(int id, uint8_t duty)
{
    if (!isValidId(id))
        return;

    pwmDuty[id] = duty;
    analogWrite(pwmPins[id], duty);
}

uint8_t hwLedPwmGetDuty(int id)
{
    if (!isValidId(id))
        return 0;

    return pwmDuty[id];
}
