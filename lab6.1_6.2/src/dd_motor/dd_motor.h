#ifndef DD_MOTOR_H
#define DD_MOTOR_H

#include <stdbool.h>
#include <stdint.h>

#define DD_MOTOR_MAX_COUNT 2

void    hwMotorInit(int id, int pinEN, int pinIN1, int pinIN2);
void    hwMotorSetSpeed(int id, uint8_t speedPct, bool forward);
void    hwMotorStop(int id);
uint8_t hwMotorGetSpeed(int id);
bool    hwMotorGetDirection(int id);

#endif
