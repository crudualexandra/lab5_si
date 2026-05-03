#ifndef CTRL_PID_H
#define CTRL_PID_H

#include <stdbool.h>

typedef struct
{
    float kp;
    float ki;
    float kd;

    float integral;
    float prevError;

    float outMin;
    float outMax;
    float dt;

    bool  firstRun;
} CtrlPid_t;

void  regPidInit(CtrlPid_t *p, float kp, float ki, float kd,
                 float outMin, float outMax, float dt);
void  regPidSetGains(CtrlPid_t *p, float kp, float ki, float kd);
void  regPidReset(CtrlPid_t *p);
float regPidCompute(CtrlPid_t *p, float setPoint, float measured);

#endif
