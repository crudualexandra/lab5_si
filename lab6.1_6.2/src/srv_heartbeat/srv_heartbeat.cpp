#include <Arduino.h>
#include "srv_heartbeat.h"
#include "dd_led/dd_led.h"

static int heartbeatLedId = 0;

void sysHeartbeatInit(int ledId, int pin)
{
    heartbeatLedId = ledId;
    hwLedInit(ledId, pin);
}

void sysHeartbeatRun(void)
{
    hwLedToggle(heartbeatLedId);
}
