#ifndef TIMER_H_
#define TIMER_H_

struct TimerStruct {
    unsigned int IR;
    unsigned int TCR;
    unsigned int TC;
    unsigned int PR;
    unsigned int PC;
    unsigned int MCR;
    unsigned int MR[4];
    unsigned int CCR;
    unsigned int CR[2];
    unsigned int EMR;
    unsigned int CTCR;
};
#define T0 (*(volatile struct TimerStruct *) 0x40004000)
#define T1 (*(volatile struct TimerStruct *) 0x40008000)
#define T2 (*(volatile struct TimerStruct *) 0x40090000)
#define T3 (*(volatile struct TimerStruct *) 0x40094000)

#endif
