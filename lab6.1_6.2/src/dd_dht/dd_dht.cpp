#include <Arduino.h>
#include <DHT.h>
#include "dd_dht.h"

static DHT   dht;
static float lastTemp  = 0.0f;
static float lastHum   = 0.0f;
static bool  lastValid = false;

void hwDhtInit(uint8_t pin, DhtModel_t model)
{
    DHT::DHT_MODEL_t hw = (model == DD_DHT22) ? DHT::DHT22 : DHT::DHT11;
    dht.setup(pin, hw);
}

bool hwDhtRead(void)
{
    float temp = dht.getTemperature();
    float hum  = dht.getHumidity();

    bool ok = !isnan(temp) && !isnan(hum) && (dht.getStatus() == DHT::ERROR_NONE);

    if (ok)
    {
        lastTemp = temp;
        lastHum  = hum;
    }

    lastValid = ok;
    return ok;
}

float hwDhtGetTemp(void)
{
    return lastTemp;
}

float hwDhtGetHum(void)
{
    return lastHum;
}

bool hwDhtIsValid(void)
{
    return lastValid;
}
