#ifndef DD_LED_PWM_H
#define DD_LED_PWM_H

#include <stdint.h>

#define DD_LED_PWM_MAX_COUNT 2

void    hwLedPwmInit(int id, int pin);
void    hwLedPwmWrite(int id, uint8_t duty);
uint8_t hwLedPwmGetDuty(int id);

#endif
