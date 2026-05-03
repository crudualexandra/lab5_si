#ifndef DD_SOUND_H
#define DD_SOUND_H

#include <stdint.h>

void    hwSoundInit(uint8_t pin);
int     hwSoundRead(void);
uint8_t hwSoundGetPercent(void);
int     hwSoundGetDcAvg(void);
bool    hwSoundIsConnected(void);

#endif
