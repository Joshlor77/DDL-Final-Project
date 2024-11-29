#ifndef DAC_DEF_H_
#define DAC_DEF_H_

#define PCLKSEL0    (*(volatile unsigned int *) 0x400FC1A8)
#define PCONP       (*(volatile unsigned int *) 0x400Fc0C4)

#define DACR        (*(volatile unsigned int *) 0x4008C000)
#define DACCTRL     (*(volatile unsigned int *) 0x4008C004)
#define DACCNTVAL   (*(volatile unsigned int *) 0x4008C008)

#endif
