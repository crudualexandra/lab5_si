#include <Arduino.h>
#include "dd_ldr.h"
#define STABILITY_SAMPLES 16
#define SAMPLE_GAP_US     700
#define JITTER_THRESHOLD  20

#define ADC_MAX 1023

static uint8_t ldrPin     = A0;
static int     lastRaw    = 0;
static int     lastJitter = 0;

void hwLdrInit(uint8_t pin)
{
    ldrPin = pin;
    pinMode(pin, INPUT);
}

int hwLdrRead(void)
{
    int  minVal = ADC_MAX;
    int  maxVal = 0;
    long sum    = 0;

    for (int i = 0; i < STABILITY_SAMPLES; i++)
    {
        int val = analogRead(ldrPin);
        if (val < minVal) minVal = val;
        if (val > maxVal) maxVal = val;
        sum += val;

        if (i < STABILITY_SAMPLES - 1)
            delayMicroseconds(SAMPLE_GAP_US);
    }

    lastRaw    = (int)((sum + STABILITY_SAMPLES / 2) / STABILITY_SAMPLES);
    lastJitter = maxVal - minVal;
    return lastRaw;
}

float hwLdrGetLight(void)
{
    float pct = 100.0f - ((float)lastRaw * (100.0f / (float)ADC_MAX));
    if (pct < 0.0f)   pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;
    return pct;
}

uint8_t hwLdrGetPercent(void)
{
    return (uint8_t)(hwLdrGetLight() + 0.5f);
}

int hwLdrGetJitter(void)
{
    return lastJitter;
}

bool hwLdrIsConnected(void)
{
    return (lastJitter < JITTER_THRESHOLD);
}
