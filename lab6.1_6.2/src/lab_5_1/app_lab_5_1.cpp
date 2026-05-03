#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <stdio.h>
#include <string.h>

#include "app_lab_5_1.h"
#include "dd_dht/dd_dht.h"
#include "dd_actuator/dd_actuator.h"
#include "dd_lcd/dd_lcd.h"
#include "dd_led/dd_led.h"
#include "srv_serial_stdio/srv_serial_stdio.h"
#include "srv_heartbeat/srv_heartbeat.h"

#define PIN_DHT           4
#define PIN_RELAY         7
#define PIN_LED_HEARTBEAT 13

#define LCD_I2C_ADDR 0x27
#define LCD_COLS     16
#define LCD_ROWS     2

#define ACT_RELAY     0
#define LED_HEARTBEAT 0

#define MS_TO_TICKS(ms) \
    ((pdMS_TO_TICKS(ms) > 0) ? pdMS_TO_TICKS(ms) : (TickType_t)1)

#define PERIOD_SENSOR    2000
#define PERIOD_CONTROL   500
#define PERIOD_DISPLAY   1000
#define PERIOD_HEARTBEAT 500

#define DEFAULT_SET_POINT  29.0f
#define DEFAULT_HYSTERESIS 1.0f
#define LCD_PAGE_CYCLES 3

typedef struct
{
    float temperature;
    float humidity;
    bool  valid;
} SensorData_t;

typedef struct
{
    float setPoint;
    float hysteresis;
    bool  relayState;
} ControlData_t;

static SensorData_t  sensorData  = {0.0f, 0.0f, false};
static ControlData_t controlData = {DEFAULT_SET_POINT, DEFAULT_HYSTERESIS, false};

static SemaphoreHandle_t xSensorMutex = NULL;
static SemaphoreHandle_t xCtrlMutex   = NULL;

static inline SensorData_t readSensorSnap(void)
{
    SensorData_t s;
    xSemaphoreTake(xSensorMutex, portMAX_DELAY);
    s = sensorData;
    xSemaphoreGive(xSensorMutex);
    return s;
}

static inline ControlData_t readControlSnap(void)
{
    ControlData_t c;
    xSemaphoreTake(xCtrlMutex, portMAX_DELAY);
    c = controlData;
    xSemaphoreGive(xCtrlMutex);
    return c;
}

static inline void writeRelayState(bool state)
{
    xSemaphoreTake(xCtrlMutex, portMAX_DELAY);
    controlData.relayState = state;
    xSemaphoreGive(xCtrlMutex);
}

static inline void writeSetPoint(float val)
{
    xSemaphoreTake(xCtrlMutex, portMAX_DELAY);
    controlData.setPoint = val;
    xSemaphoreGive(xCtrlMutex);
}

static inline void writeHysteresis(float val)
{
    xSemaphoreTake(xCtrlMutex, portMAX_DELAY);
    controlData.hysteresis = val;
    xSemaphoreGive(xCtrlMutex);
}

static void convertToLower(char *s)
{
    for (uint8_t i = 0; s[i]; i++)
        if (s[i] >= 'A' && s[i] <= 'Z')
            s[i] += 32;
}

// Task 1: Sensor Acquisition

static void sensorReadTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWake = xTaskGetTickCount();

    for (;;)
    {
        bool ok = hwDhtRead();

        xSemaphoreTake(xSensorMutex, portMAX_DELAY);
        if (ok)
        {
            sensorData.temperature = hwDhtGetTemp();
            sensorData.humidity    = hwDhtGetHum();
            sensorData.valid       = true;
        }
        else
        {
            sensorData.valid = false;
        }
        xSemaphoreGive(xSensorMutex);

        vTaskDelayUntil(&xLastWake, MS_TO_TICKS(PERIOD_SENSOR));
    }
}

// Task 2: ON-OFF Control

static void controlTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWake = xTaskGetTickCount();

    for (;;)
    {
        SensorData_t  snap = readSensorSnap();
        ControlData_t ctrl = readControlSnap();

        bool newState = ctrl.relayState;

        if (snap.valid)
        {
            float lo = ctrl.setPoint - ctrl.hysteresis;
            float hi = ctrl.setPoint + ctrl.hysteresis;

            if (snap.temperature < lo)
                newState = true;
            else if (snap.temperature > hi)
                newState = false;
        }

        if (newState)
            hwActuatorOn(ACT_RELAY);
        else
            hwActuatorOff(ACT_RELAY);

        writeRelayState(newState);

        vTaskDelayUntil(&xLastWake, MS_TO_TICKS(PERIOD_CONTROL));
    }
}

// Task 3: Display

static void displayTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWake = xTaskGetTickCount();

    uint8_t cycleCount  = 0;
    uint8_t currentPage = 0;

    for (;;)
    {
        SensorData_t  snapS = readSensorSnap();
        ControlData_t snapC = readControlSnap();

        float lo = snapC.setPoint - snapC.hysteresis;
        float hi = snapC.setPoint + snapC.hysteresis;

        cycleCount++;
        if (cycleCount >= LCD_PAGE_CYCLES)
        {
            cycleCount  = 0;
            currentPage = (currentPage + 1) % 2;
        }

        char row0[17];
        char row1[17];

        if (currentPage == 0)
        {
            if (snapS.valid)
                snprintf(row0, sizeof(row0), "T:%.1fC SP:%.1fC",
                         (double)snapS.temperature, (double)snapC.setPoint);
            else
                snprintf(row0, sizeof(row0), "T:--.- SP:%.1fC",
                         (double)snapC.setPoint);

            snprintf(row1, sizeof(row1), "Relay:%-3s H:%.1f",
                     snapC.relayState ? "ON" : "OFF",
                     (double)snapC.hysteresis);
        }
        else
        {
            snprintf(row0, sizeof(row0), "Lo:%.1f Hi:%.1f",
                     (double)lo, (double)hi);

            if (snapS.valid)
                snprintf(row1, sizeof(row1), "Hum:%.1f%%",
                         (double)snapS.humidity);
            else
                snprintf(row1, sizeof(row1), "Hum:--.-%%");
        }

        hwLcdClear();
        hwLcdSetCursor(0, 0);
        hwLcdPrint(row0);
        hwLcdSetCursor(0, 1);
        hwLcdPrint(row1);
        float relayPlot = snapC.relayState ? snapC.setPoint : 0.0f;
        float temp      = snapS.valid ? snapS.temperature : 0.0f;

        printf(">Temperature:%.2f\n", (double)temp);
        printf(">SetPoint:%.2f\n",    (double)snapC.setPoint);
        printf(">Lo:%.2f\n",          (double)lo);
        printf(">Hi:%.2f\n",          (double)hi);
        printf(">Relay:%.2f\n",       (double)relayPlot);

        vTaskDelayUntil(&xLastWake, MS_TO_TICKS(PERIOD_DISPLAY));
    }
}

// Task 4: Command Reader

static void processHelp(void)
{
    printf("Commands:\n");
    printf("  set <temp>  - set target temperature (C)\n");
    printf("  hyst <val>  - set hysteresis band (C)\n");
    printf("  status      - print current state\n");
    printf("  help        - show this list\n");
}

static void processStatus(void)
{
    SensorData_t  snapS = readSensorSnap();
    ControlData_t snapC = readControlSnap();

    printf("--- Status ---\n");
    if (snapS.valid)
        printf("Temperature : %.2f C\n", (double)snapS.temperature);
    else
        printf("Temperature : invalid read\n");
    printf("Humidity    : %.2f %%\n", (double)snapS.humidity);
    printf("Set Point   : %.2f C\n", (double)snapC.setPoint);
    printf("Hysteresis  : %.2f C\n", (double)snapC.hysteresis);
    printf("Thresholds  : Lo=%.2f  Hi=%.2f\n",
           (double)(snapC.setPoint - snapC.hysteresis),
           (double)(snapC.setPoint + snapC.hysteresis));
    printf("Relay       : %s\n", snapC.relayState ? "ON" : "OFF");
}

static void processSet(void)
{
    float val = 0.0f;
    if (scanf("%f", &val) == 1 && val > 0.0f && val < 80.0f)
    {
        writeSetPoint(val);
        printf("[CTRL] Set Point = %.2f C\n", (double)val);
    }
    else
    {
        printf("[ERR] Usage: set <temp>  (0 - 80)\n");
    }
}

static void processHyst(void)
{
    float val = 0.0f;
    if (scanf("%f", &val) == 1 && val > 0.0f && val < 20.0f)
    {
        writeHysteresis(val);
        printf("[CTRL] Hysteresis = %.2f C\n", (double)val);
    }
    else
    {
        printf("[ERR] Usage: hyst <band>  (0 - 20)\n");
    }
}

static void cmdReadTask(void *pvParameters)
{
    (void)pvParameters;

    char cmd[8];

    for (;;)
    {
        if (scanf("%7s", cmd) != 1)
            continue;

        convertToLower(cmd);

        if (strcmp(cmd, "help") == 0)
            processHelp();
        else if (strcmp(cmd, "status") == 0)
            processStatus();
        else if (strcmp(cmd, "set") == 0)
            processSet();
        else if (strcmp(cmd, "hyst") == 0)
            processHyst();
        else
            printf("[ERR] Unknown command '%s'. Type 'help'.\n", cmd);
    }
}

// Task 5: Heartbeat

static void heartbeatTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWake = xTaskGetTickCount();

    for (;;)
    {
        sysHeartbeatRun();
        vTaskDelayUntil(&xLastWake, MS_TO_TICKS(PERIOD_HEARTBEAT));
    }
}

static void stopWithError(const char *msg)
{
    printf("FATAL: %s\n", msg);
    for (;;) { }
}

void labCtrlTempInit()
{
    sysSerialInit();

    hwDhtInit(PIN_DHT, DD_DHT22);
    hwActuatorInit(ACT_RELAY, PIN_RELAY, true);
    sysHeartbeatInit(LED_HEARTBEAT, PIN_LED_HEARTBEAT);
    hwLcdInit(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);

    xSensorMutex = xSemaphoreCreateMutex();
    xCtrlMutex   = xSemaphoreCreateMutex();

    if (!xSensorMutex || !xCtrlMutex)
        stopWithError("mutex creation failed");

    BaseType_t ok = pdPASS;
    ok &= xTaskCreate(sensorReadTask, "Sensor",  256, NULL, 2, NULL);
    ok &= xTaskCreate(controlTask,    "Control", 256, NULL, 3, NULL);
    ok &= xTaskCreate(displayTask,    "Display", 384, NULL, 1, NULL);
    ok &= xTaskCreate(cmdReadTask,    "CmdRead", 256, NULL, 1, NULL);
    ok &= xTaskCreate(heartbeatTask,  "HB",      192, NULL, 1, NULL);

    if (ok != pdPASS)
        stopWithError("task creation failed");

    printf("Lab 5.1 - ON-OFF Temperature Control with Hysteresis\n");
    printf("Sensor  : DHT22 on pin %d\n", PIN_DHT);
    printf("Relay   : pin %d (active-LOW)\n", PIN_RELAY);
    printf("Set Pt  : %.1f C   Hysteresis: %.1f C\n",
           (double)DEFAULT_SET_POINT, (double)DEFAULT_HYSTERESIS);
    printf("Type 'help' for commands.\n");

    vTaskStartScheduler();
}

void labCtrlTempRun()
{
}
