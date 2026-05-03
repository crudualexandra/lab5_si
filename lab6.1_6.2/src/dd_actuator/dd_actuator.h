#ifndef DD_ACTUATOR_H
#define DD_ACTUATOR_H

#include <stdbool.h>

#define DD_ACTUATOR_MAX_COUNT 4

void hwActuatorInit(int id, int pin, bool activeLow);
void hwActuatorOn(int id);
void hwActuatorOff(int id);
void hwActuatorToggle(int id);
bool hwActuatorGetState(int id);

#endif
