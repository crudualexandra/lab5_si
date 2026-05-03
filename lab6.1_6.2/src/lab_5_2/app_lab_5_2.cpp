#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "app_lab_5_2.h"
#include "dd_ldr/dd_ldr.h"
#include "dd_led_pwm/dd_led_pwm.h"
#include "dd_lcd/dd_lcd.h"
#include "dd_led/dd_led.h"
#include "ctrl_pid/ctrl_pid.h"
#include "srv_serial_stdio/srv_serial_stdio.h"
#include "srv_heartbeat/srv_heartbeat.h"

#define PIN_LDR           A0
#define PIN_LED_PWM       9
#define PIN_LED_HEARTBEAT 13

#define LCD_I2C_ADDR 0x27
#define LCD_COLS     16
#define LCD_ROWS     2

#define LED_ACT       0
#define LED_HEARTBEAT 0

#define MS_TO_TICKS(ms) \
    ((pdMS_TO_TICKS(ms) > 0) ? pdMS_TO_TICKS(ms) : (TickType_t)1)

#define PERIOD_SENSOR    200
#define PERIOD_CONTROL   200
#define PERIOD_DISPLAY   500
#define PERIOD_HEARTBEAT 500

#define CONTROL_DT_S (PERIOD_CONTROL / 1000.0f)
#define LIGHT_EMA_ALPHA 0.35f

#define DEFAULT_SET_POINT 50.0f
#define DEFAULT_KP 0.5f
#define DEFAULT_KI 0.1f
#define DEFAULT_KD 0.02f

#define PID_OUT_MIN 0.0f
#define PID_OUT_MAX 255.0f

#define LCD_PAGE_CYCLES 3

typedef struct
{
    int   raw;
    float percent;
    bool  valid;
} SensorData_t;

typedef struct
{
    float setPoint;
    float kp;
    float ki;
    float kd;
    float output;
} ControlData_t;

static SensorData_t  sensorData  = {0, 0.0f, false};
static ControlData_t controlData = {DEFAULT_SET_POINT, DEFAULT_KP, DEFAULT_KI, DEFAULT_KD, 0.0f};

static CtrlPid_t pid;

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

static inline void storePidOutput(float val)
{
    xSemaphoreTake(xCtrlMutex, portMAX_DELAY);
    controlData.output = val;
    xSemaphoreGive(xCtrlMutex);
}

static inline void writeSetPoint(float val)
{
    xSemaphoreTake(xCtrlMutex, portMAX_DELAY);
    controlData.setPoint = val;
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

    float filtered = 0.0f;
    bool  first    = true;

    for (;;)
    {
        int   raw    = hwLdrRead();
        float sample = hwLdrGetLight();

        if (first)
        {
            filtered = sample;
            first    = false;
        }
        else
        {
            filtered = LIGHT_EMA_ALPHA * sample
                     + (1.0f - LIGHT_EMA_ALPHA) * filtered;
        }

        xSemaphoreTake(xSensorMutex, portMAX_DELAY);
        sensorData.raw     = raw;
        sensorData.percent = filtered;
        sensorData.valid   = true;
        xSemaphoreGive(xSensorMutex);

        vTaskDelayUntil(&xLastWake, MS_TO_TICKS(PERIOD_SENSOR));
    }
}

// Task 2: PID Control

static void controlTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWake = xTaskGetTickCount();

    for (;;)
    {
        SensorData_t snap = readSensorSnap();

        xSemaphoreTake(xCtrlMutex, portMAX_DELAY);
        float sp = controlData.setPoint;
        regPidSetGains(&pid, controlData.kp, controlData.ki, controlData.kd);
        xSemaphoreGive(xCtrlMutex);

        float output = 0.0f;
        if (snap.valid)
            output = regPidCompute(&pid, sp, snap.percent);

        uint8_t duty = (uint8_t)lroundf(output);
        hwLedPwmWrite(LED_ACT, duty);

        storePidOutput(output);

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
                snprintf(row0, sizeof(row0), "L:%3d%% SP:%3d%%",
                         (int)lroundf(snapS.percent),
                         (int)lroundf(snapC.setPoint));
            else
                snprintf(row0, sizeof(row0), "L:--%%  SP:%3d%%",
                         (int)lroundf(snapC.setPoint));

            snprintf(row1, sizeof(row1), "OUT:%3d Kp:%.2f",
                     (int)lroundf(snapC.output), (double)snapC.kp);
        }
        else
        {
            snprintf(row0, sizeof(row0), "Ki:%.2f Kd:%.2f",
                     (double)snapC.ki, (double)snapC.kd);

            if (snapS.valid)
                snprintf(row1, sizeof(row1), "raw:%4d", snapS.raw);
            else
                snprintf(row1, sizeof(row1), "raw:----");
        }

        hwLcdClear();
        hwLcdSetCursor(0, 0);
        hwLcdPrint(row0);
        hwLcdSetCursor(0, 1);
        hwLcdPrint(row1);
        float outPlot   = snapC.output * (100.0f / 255.0f);
        float lightPlot = snapS.valid ? snapS.percent : 0.0f;

        printf(">SetPoint:%.2f|g:control\n", (double)snapC.setPoint);
        printf(">Light:%.2f|g:control\n",    (double)lightPlot);
        printf(">Output:%.2f|g:control\n",   (double)outPlot);

        vTaskDelayUntil(&xLastWake, MS_TO_TICKS(PERIOD_DISPLAY));
    }
}

// Task 4: Command Reader

static void processHelp(void)
{
    printf("Commands:\n");
    printf("  set <0-100> - set target light level (%%)\n");
    printf("  kp <value>  - set proportional gain\n");
    printf("  ki <value>  - set integral gain\n");
    printf("  kd <value>  - set derivative gain\n");
    printf("  reset       - clear PID integral/derivative\n");
    printf("  status      - print current state\n");
    printf("  help        - show this list\n");
}

static void processStatus(void)
{
    SensorData_t  snapS = readSensorSnap();
    ControlData_t snapC = readControlSnap();

    printf("--- Status ---\n");
    if (snapS.valid)
        printf("Light     : %.2f %% (raw %d)\n",
               (double)snapS.percent, snapS.raw);
    else
        printf("Light     : invalid read\n");
    printf("Set Point : %.2f %%\n", (double)snapC.setPoint);
    printf("Kp        : %.3f\n", (double)snapC.kp);
    printf("Ki        : %.3f\n", (double)snapC.ki);
    printf("Kd        : %.3f\n", (double)snapC.kd);
    printf("Integral  : %.3f\n", (double)pid.integral);
    printf("Output    : %.2f (PWM 0-255)\n", (double)snapC.output);
}

static void processReset(void)
{
    xSemaphoreTake(xCtrlMutex, portMAX_DELAY);
    regPidReset(&pid);
    xSemaphoreGive(xCtrlMutex);
    printf("[CTRL] PID state cleared\n");
}

static void processSet(void)
{
    float val = 0.0f;
    if (scanf("%f", &val) == 1 && val >= 0.0f && val <= 100.0f)
    {
        writeSetPoint(val);
        printf("[CTRL] Set Point = %.2f %%\n", (double)val);
    }
    else
    {
        printf("[ERR] Usage: set <level>  (0 - 100)\n");
    }
}

static void processGain(const char *cmd)
{
    float val = 0.0f;
    if (scanf("%f", &val) == 1 && val >= 0.0f && val < 1000.0f)
    {
        xSemaphoreTake(xCtrlMutex, portMAX_DELAY);
        if      (cmd[1] == 'p') controlData.kp = val;
        else if (cmd[1] == 'i') controlData.ki = val;
        else                    controlData.kd = val;
        regPidReset(&pid);
        xSemaphoreGive(xCtrlMutex);
        printf("[CTRL] %c%c = %.3f (PID reset)\n",
               cmd[0], cmd[1], (double)val);
    }
    else
    {
        printf("[ERR] Usage: %s <value>  (0 - 1000)\n", cmd);
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
        else if (strcmp(cmd, "reset") == 0)
            processReset();
        else if (strcmp(cmd, "set") == 0)
            processSet();
        else if (strcmp(cmd, "kp") == 0 || strcmp(cmd, "ki") == 0 || strcmp(cmd, "kd") == 0)
            processGain(cmd);
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

void labCtrlLightInit()
{
    sysSerialInit();

    hwLdrInit(PIN_LDR);
    hwLedPwmInit(LED_ACT, PIN_LED_PWM);
    sysHeartbeatInit(LED_HEARTBEAT, PIN_LED_HEARTBEAT);
    hwLcdInit(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);

    regPidInit(&pid,
               DEFAULT_KP, DEFAULT_KI, DEFAULT_KD,
               PID_OUT_MIN, PID_OUT_MAX,
               CONTROL_DT_S);

    xSensorMutex = xSemaphoreCreateMutex();
    xCtrlMutex   = xSemaphoreCreateMutex();

    if (!xSensorMutex || !xCtrlMutex)
        stopWithError("mutex creation failed");

    BaseType_t ok = pdPASS;
    ok &= xTaskCreate(sensorReadTask, "Sensor",  192, NULL, 2, NULL);
    ok &= xTaskCreate(controlTask,    "Control", 256, NULL, 3, NULL);
    ok &= xTaskCreate(displayTask,    "Display", 384, NULL, 1, NULL);
    ok &= xTaskCreate(cmdReadTask,    "CmdRead", 256, NULL, 1, NULL);
    ok &= xTaskCreate(heartbeatTask,  "HB",      192, NULL, 1, NULL);

    if (ok != pdPASS)
        stopWithError("task creation failed");

    printf("Lab 5.2 - PID Light Control\n");
    printf("Sensor : LDR on A%d (raw 0-1023 -> 0-100%%)\n", PIN_LDR - A0);
    printf("LED    : PWM pin %d (0-255)\n", PIN_LED_PWM);
    printf("SP=%.1f%%  Kp=%.2f Ki=%.2f Kd=%.2f  dt=%.2fs\n",
           (double)DEFAULT_SET_POINT,
           (double)DEFAULT_KP, (double)DEFAULT_KI, (double)DEFAULT_KD,
           (double)CONTROL_DT_S);
    printf("Type 'help' for commands.\n");

    vTaskStartScheduler();
}

void labCtrlLightRun()
{
}
