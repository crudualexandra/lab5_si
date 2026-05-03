#include <Arduino.h>
#include "dd_sound.h"

#define SAMPLE_COUNT 50

static uint8_t soundPin = A1;
static int lastPeakToPeak = 0;
static int lastDcAvg = 0;

void hwSoundInit(uint8_t pin)
{
    soundPin = pin;
    pinMode(pin, INPUT);
}

int hwSoundRead(void)
{
    int sampleMin = 1023;
    int sampleMax = 0;
    long sum = 0;

    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        int val = analogRead(soundPin);

        if (val > sampleMax) sampleMax = val;
        if (val < sampleMin) sampleMin = val;
        sum += val;
    }

    lastPeakToPeak = sampleMax - sampleMin;
    lastDcAvg = (int)(sum / SAMPLE_COUNT);
    return lastPeakToPeak;
}
#define SOUND_RAW_MAX 80

uint8_t hwSoundGetPercent(void)
{
    int clamped = constrain(lastPeakToPeak, 0, SOUND_RAW_MAX);
    return (uint8_t)map(clamped, 0, SOUND_RAW_MAX, 0, 100);
}

int hwSoundGetDcAvg(void)
{
    return lastDcAvg;
}

bool hwSoundIsConnected(void)
{
    return (lastDcAvg > 412 && lastDcAvg < 612);
}
