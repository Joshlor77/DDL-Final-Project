#include "FIO.h"

/* Sets the CCLK to 16Mhz
 *
 */

#define PLL0CON  	(*(volatile unsigned int *)0x400Fc080)
#define PLL0CFG 	(*(volatile unsigned int *)0x400FC084)
#define PLL0STAT	(*(volatile unsigned int *)0x400FC088)
#define PLOCK0		((PLL0STAT >> 26) & 1u)
#define PLL0FEED	(*(volatile unsigned int *)0x400FC08C)
#define CCLKCFG		(*(volatile unsigned int *)0x400FC104)
#define CLKOUTCFG	(*(volatile unsigned int *)0x400FC1C8)
#define CLKSRCSEL  	(*(volatile unsigned int *)0x400FC10C)
#define SCS 		(*(volatile unsigned int *)0x400FC1A0)
#define OSCSTAT		((SCS >> 6) & 1u)

static void mainOscStartUp(){
	//Enable
	SCS |= (1 << 5);

	//Wait for main oscillator to be ready
	while (OSCSTAT != 1){

	}
}

void feedSeq(){
	volatile int i;
	PLL0FEED = 0xAA;
	i++;
	PLL0FEED = 0x55;
}

//Value used is M-1 and K-1
void PLL0StartUpSeq(){
    mainOscStartUp();

	//Disconnect
	PLL0CON &= ~(1u << 1);
	feedSeq();

	//Disable
	PLL0CON &= ~(1u);
	feedSeq();

	CLKSRCSEL = (CLKSRCSEL | 1) & ~(1u << 1);

	//PLL Multiplier
	PLL0CFG &= ~(0x7FFF);
	feedSeq();
	unsigned int M = 16;
	PLL0CFG |= (M - 1);

	//Enable
	PLL0CON |= 1;
	feedSeq();

	//Wait for PLL0 to achieve lock
	while(1){
		if (PLOCK0){
			break;
		}
	}

	//CPU Clock Divider Setting
	unsigned int K = 24;
	CCLKCFG |= (K - 1);

	//Connect
	PLL0CON |= (1 << 1);
	feedSeq();
}

//Outputs the clock to P1.27, this is to test what the CPU clock is.
void outputClkPin(){
	CLKOUTCFG &= ~15u;
	PINMODE[3] |= (1 << 23);
	PINSEL[3] |= (1 << 22);
	CLKOUTCFG |= (1 << 8);
}
