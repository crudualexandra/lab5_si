#include "ctrl_pid.h"

#define MIN_DT_S 0.001f

void regPidInit(CtrlPid_t *p, float kp, float ki, float kd,
                float outMin, float outMax, float dt)
{
    if (!p)
        return;

    p->kp = kp;
    p->ki = ki;
    p->kd = kd;

    p->outMin = outMin;
    p->outMax = outMax;
    p->dt     = (dt > 0.0f) ? dt : MIN_DT_S;

    regPidReset(p);
}

void regPidSetGains(CtrlPid_t *p, float kp, float ki, float kd)
{
    if (!p)
        return;

    p->kp = kp;
    p->ki = ki;
    p->kd = kd;
}

void regPidReset(CtrlPid_t *p)
{
    if (!p)
        return;

    p->integral  = 0.0f;
    p->prevError = 0.0f;
    p->firstRun  = true;
}

float regPidCompute(CtrlPid_t *p, float setPoint, float measured)
{
    if (!p)
        return 0.0f;

    float error = setPoint - measured;

    float derivative = 0.0f;
    if (!p->firstRun)
        derivative = (error - p->prevError) / p->dt;

    float newIntegral = p->integral + error * p->dt;

    float output = p->kp * error
                 + p->ki * newIntegral
                 + p->kd * derivative;

    bool saturatedHigh = (output > p->outMax);
    bool saturatedLow  = (output < p->outMin);

    if (saturatedHigh)
        output = p->outMax;
    else if (saturatedLow)
        output = p->outMin;

    bool blockIntegration =
        (saturatedHigh && error > 0.0f) ||
        (saturatedLow  && error < 0.0f);

    if (!blockIntegration)
        p->integral = newIntegral;

    p->prevError = error;
    p->firstRun  = false;

    return output;
}
