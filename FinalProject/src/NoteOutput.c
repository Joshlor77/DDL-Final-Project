#include "NoteOutput.h"
#include "FIO.h"

#define DMACIntStat         (*(volatile unsigned int *) 0x50004000)
#define DMACIntTCStat       (*(volatile unsigned int *) 0x50004004)
#define DMACIntTCClear      (*(volatile unsigned int *) 0x50004008)
#define DMACIntErrStat      (*(volatile unsigned int *) 0x5000400C)
#define DMACIntErrClr       (*(volatile unsigned int *) 0x50004010)
#define DMACRawIntTCStat    (*(volatile unsigned int *) 0x50004014)
#define DMACRawIntErrStat   (*(volatile unsigned int *) 0x50004018)
#define DMACEnbldChns       (*(volatile unsigned int *) 0x5000401C)
#define DMACSoftBReq        (*(volatile unsigned int *) 0x50004020)
#define DMACSoftSReq        (*(volatile unsigned int *) 0x50004024)
#define DMACSoftLBReq       (*(volatile unsigned int *) 0x50004028)
#define DMACSoftLSReq       (*(volatile unsigned int *) 0x5000402C)
#define DMACConfig          (*(volatile unsigned int *) 0x50004030)
#define DMACSync            (*(volatile unsigned int *) 0x50004034)
#define DMAREQSEL           (*(volatile unsigned int *) 0x400FC1C4)

struct DMAChStruct {
    unsigned int SrcAddr;
    unsigned int DestAddr;
    unsigned int LLI;
    unsigned int Control;
    unsigned int Config;
};

#define DMACC0 (*(volatile struct DMAChStruct *) 0x50004100)

typedef struct {
    unsigned int SrcAddr;
    unsigned int DestAddr;
    unsigned int LLI;
    unsigned int Control;
} LLI;

/////////////////////////////////////////////////////////////////////

#define PCLK        16000000    // Hz
#define SampleRate  44100       // Hz

#define PCLKSEL0    (*(volatile unsigned int *) 0x400FC1A8)
#define PCONP       (*(volatile unsigned int *) 0x400Fc0C4)

#define DACR        (*(volatile unsigned int *) 0x4008C000)
#define DACCTRL     (*(volatile unsigned int *) 0x4008C004)
#define DACCNTVAL   (*(volatile unsigned int *) 0x4008C008)

/////////////////////////////////////////////////////////////////////

short soundData [] = {1024};
int transferSize = 1;

void initNoteSystem(){
    for (int i = 0; i < sizeof(soundData)/sizeof(short); i++){
        soundData[i] = (soundData[i] << 6);
    }

    //Set P0.26 function to AOUT and mode to no pull resistors
    PINSEL[1] &= ~(3 << 20);
    PINMODE[1] &= (1 << 21);
    PINSEL[1] |= (1 << 21);

    //Set PCLK divider to 1 for DAC
    PCLKSEL0 |= (1 << 23);

    //Set DMA Counter for DAC
    DACCNTVAL = PCLK / SampleRate;

    //Enable DMA access and DMA counter
    DACCTRL |= 0xA;

    //Enable DMA Peripheral and enable DMA controller
    PCONP |= (1 << 29);
    DMACConfig |= 1;

    //Clear Interrupts for DMACC0
    DMACIntErrClr = 1;
    DMACIntTCClear = 1;

    //Set Source Address and Destination Address for DMA
    DMACC0.SrcAddr = &soundData;
    DMACC0.DestAddr = &DACR;
    DMACC0.LLI = 0;

    //Set relevant settings for DMACC0
    DMACC0.Control = 
        transferSize |  //Transfer Size 
        (1 << 12) | //SBSize
        (1 << 15) | //DBSize
        (1 << 18) | //SWidth
        (1 << 21);  //DWidth
    DMACC0.Config = 
        (1) |       //Enable Ch0
        (7 << 6) |  //DAC Peripheral
        (2 << 11);  //Memory to Peripheral

    //Enable DMA Channel 0
    DMACC0.Config |= 1;

}