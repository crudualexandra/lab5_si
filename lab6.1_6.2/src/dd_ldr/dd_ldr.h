#ifndef DD_LDR_H
#define DD_LDR_H

#include <stdint.h>

void    hwLdrInit(uint8_t pin);
int     hwLdrRead(void);
float   hwLdrGetLight(void);
uint8_t hwLdrGetPercent(void);
int     hwLdrGetJitter(void);
bool    hwLdrIsConnected(void);

#endif
