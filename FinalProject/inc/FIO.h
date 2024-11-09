#ifndef FIO_H_
#define FIO_H_

struct FIOstruct {
	unsigned int DIR;
	unsigned int filler [3];
	unsigned int MASK;
	unsigned int PIN;
	unsigned int SET;
	unsigned int CLR;
};
#define FIO 		((volatile struct FIOstruct *) 0x2009c000)
#define PINMODE		((volatile unsigned int *)	0x4002C040)

#endif
