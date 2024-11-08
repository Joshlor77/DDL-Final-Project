#include "PWM.h"

/* This is for being able to play notes.
 * PWM is used for generate Square Waves at different frequencies.
 * Only PWM features which may be used in the project are implemented
 * 
 * All PWM channels are single edge controlled.
 * PCLK is used as the clock input
 * 
 */

struct PWMStruct {
    unsigned int IR;
    unsigned int TCR;
    unsigned int TC;
    unsigned int PR;
    unsigned int PC;
    unsigned int MCR;
    unsigned int MR0;
    unsigned int MR1;
    unsigned int MR2;
    unsigned int MR3;
    unsigned int CCR;
    unsigned int CR[4];
    unsigned int MR4;
    unsigned int MR5;
    unsigned int MR6;
    unsigned int PCR;
    unsigned int LER;
    unsigned int CTCR;
};

#define PWM (*(volatile struct PWMStruct *) 0x40018000)

void PWMControls(int CounterEnable, int Reset, int PWMEnable){
    int data = 0;
    if (CounterEnable)
        data += 1;
    if (Reset)
        data += 2;
    if (PWMEnable){
        data += 8;
    }
    PWM.TCR |= data;
}

void writeMCR(int data){
    PWM.MCR |= (data & 0xFFFFF);
}

void PWMEnable(int data){
    PWM.PCR |= (data & 0x3F) << 8;
}

void PWMLatchEnable(int data){
    PWM.LER |= (data & 0x3F);
}

int writeMR(int MR, int data){
    switch (MR){
        case 0:
            PWM.MR0 = data;
            return 0;
        case 1:
            PWM.MR1 = data;
            return 0;
        case 2:
            PWM.MR2 = data;
            return 0;
        case 3:
            PWM.MR3 = data;
            return 0;
        case 4:
            PWM.MR4 = data;
            return 0;
        case 5:
            PWM.MR5 = data;
            return 0;
        case 6:
            PWM.MR6 = data;
            return 0;
        default:
            return 1;
    }
}