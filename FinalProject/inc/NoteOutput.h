#ifndef NOTEOUTPUT_H_
#define NOTEOUTPUT_H_

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

/////////////////////////////////////////////////////////////////////

#define SampleRate  44100      // Hz

#define PCLKSEL0    (*(volatile unsigned int *) 0x400FC1A8)
#define PCONP       (*(volatile unsigned int *) 0x400Fc0C4)

#define DACR        (*(volatile unsigned int *) 0x4008C000)
#define DACCTRL     (*(volatile unsigned int *) 0x4008C004)
#define DACCNTVAL   (*(volatile unsigned int *) 0x4008C008)

/////////////////////////////////////////////////////////////////////

#endif
