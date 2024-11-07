#include "Notes.h"

/* In order to play different notes, the match registers of the timer peripherals are used to generate square waves of different frequencies.
 * The LPC1769 has 6 pins available for generate square wave inputs.
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

#define Timer ((volatile struct TimerStruct *) 0x40004000)

/* Initializes MAT2.0, MAT2.1, MAT
 *
 */
void initNoteSystem(){

}