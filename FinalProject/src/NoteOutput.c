#include "NoteOutput.h"

/* Usage Guideline
 * call SetChInterval to change the note frequency
 * call enableCh to play the note
 * call disableCh to stop playing the note
 * Implement the togglePin functions to use whichever IOpin as the squarewave output.
 * 
 * For now the only outputs are square waves generated by timed interrupts.
 * 
 */

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
#define ISER0 (*(volatile unsigned int *) 0xE000E100)


void initNoteSystem(void){
    T0.IR = 0xF;          //Clear MR Interupt flags
    T0.TCR |= 1;          //Enable Timer0 counter
    ISER0 = (1 << 1);     //Enable Timer0 interrupts
}

/* Contains the time intervals for each Match Register
 * This is used to update the Match Register during the interrupt handler for the next interrupt.
 * The values for these intervals are calculated to be (Period/2) / PCLK for 50% duty cycle.
 */
unsigned int ChInterval[4];

// Enables interrupt generation from the specified match register
void enableCh(unsigned int MR){
    T0.MCR |= (1 << MR);
    T0.MR[0] = T0.TC + ChInterval[MR];
}

// Changes the interval for the Match Register
void setChInterval(int MR, unsigned int interval){
    ChInterval[MR] = interval;
}

/* Disables interrupt generation from the specified Match Register.
 * Also clears the relevant interrupt flag.
 */
void disableCh(int MR){
    T0.MCR &= ~(1u << MR);
    T0.IR = (1 << MR);
}

//These are for implementing the pin toggle for the square wave. These are to be implemented elsewhere.

__attribute__ ((weak)) void togglePin0(void){

};
__attribute__ ((weak)) void togglePin1(void){

};
__attribute__ ((weak)) void togglePin2(void){

};
__attribute__ ((weak)) void togglePin3(void){

};

void TIMER0_IRQHandler(void){
    if (T0.IR & 1){
        T0.IR = 1;
        T0.MR[0] += ChInterval[0];
        togglePin0();
    }
    if (T0.IR & 2){
        T0.IR = 2;
        T0.MR[1] += ChInterval[1];
        togglePin1();
    }
    if (T0.IR & 4){
        T0.IR = 4;
        T0.MR[2] += ChInterval[2];
        togglePin2();
    }
    if (T0.IR & 8){
        T0.IR = 8;
        T0.MR[3] += ChInterval[3];
        togglePin3();
    }
}