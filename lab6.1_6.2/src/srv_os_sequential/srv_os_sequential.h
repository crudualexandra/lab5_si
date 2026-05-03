#ifndef SRV_OS_SEQUENTIAL_H
#define SRV_OS_SEQUENTIAL_H

#include <stdint.h>

#define SRV_OS_MAX_TASKS 8

#ifndef SRV_OS_ONE_TASK_PER_TICK
#define SRV_OS_ONE_TASK_PER_TICK 1
#endif

typedef void (*SrvOsTaskFunc)(void);

typedef struct
{
    SrvOsTaskFunc task_func;
    int rec;
    int offset;
    int rec_cnt;
} SrvOsTask;

void sysOsInit(void);
void sysOsRegisterTask(SrvOsTaskFunc func, int rec, int offset);
void sysOsSchedulerLoop(void);

#endif
