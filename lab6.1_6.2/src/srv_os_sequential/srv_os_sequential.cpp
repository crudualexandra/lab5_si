#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "srv_os_sequential.h"

static SrvOsTask tasks[SRV_OS_MAX_TASKS];
static int taskCount = 0;

static volatile uint16_t tickCount = 0;

ISR(TIMER1_COMPA_vect)
{
    tickCount++;
}

static void initTimer1(void)
{
    cli();

    TCCR1A = 0;
    TCCR1B = 0;

    TCCR1B = (1 << WGM12) | (1 << CS11);

    OCR1A = 1999;

    TIMSK1 = (1 << OCIE1A);

    sei();
}

void sysOsInit(void)
{
    taskCount = 0;

    cli();
    tickCount = 0;
    sei();

    initTimer1();
}

void sysOsRegisterTask(SrvOsTaskFunc func, int rec, int offset)
{
    if (taskCount >= SRV_OS_MAX_TASKS || func == NULL)
    {
        return;
    }

    if (rec <= 0 || offset < 0)
    {
        return;
    }

    SrvOsTask *t = &tasks[taskCount];
    t->task_func = func;
    t->rec = rec;
    t->offset = offset;
    t->rec_cnt = offset;

    taskCount++;
}

void sysOsSchedulerLoop(void)
{
    uint16_t pendingTicks;

    cli();
    pendingTicks = tickCount;
    tickCount = 0;
    sei();

    if (pendingTicks == 0)
    {
        return;
    }

    while (pendingTicks--)
    {
        for (int i = 0; i < taskCount; i++)
        {
            if (--tasks[i].rec_cnt <= 0)
            {
                tasks[i].rec_cnt = tasks[i].rec;
                tasks[i].task_func();

#if SRV_OS_ONE_TASK_PER_TICK
                break;
#endif
            }
        }
    }
}
