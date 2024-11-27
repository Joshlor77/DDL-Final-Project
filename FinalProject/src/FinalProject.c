#include "NoteOutput.h"
#include "Clocking.h"
#include "Timer.h"
#include "FIO.h"
#include <math.h>

// Define control and data pins
#define RS (1<<0)  // RS pin connected to P0.0
#define RW (1<<1)  // RW pin connected to P0.1 (ground if not used)
#define EN (1<<2)  // EN pin connected to P0.2
#define D0 (1<<3)  // DB0 pin connected to P0.3
#define D1 (1<<4)  // DB1 pin connected to P0.4
#define D2 (1<<5)  // DB2 pin connected to P0.5
#define D3 (1<<6)  // DB3 pin connected to P0.6
#define D4 (1<<7)  // DB4 pin connected to P0.7
#define D5 (1<<8)  // DB5 pin connected to P0.8
#define D6 (1<<9)  // DB6 pin connected to P0.9
#define D7 (1<<10) // DB7 pin connected to P0.10

#define DACR        (*(volatile unsigned int *) 0x4008C000)
#define ISER0 		(*(volatile unsigned int *) 0xE000E100)

#define PI          	3.141592653589793238462643383279
#define BuffSize		4096
#define transferSize 	2047
#define LUTfreq			4

#define NumNotes		5
#define BBPM        112
#define SampleRate  48000 //Hz
#define PCLK        16000000 // Hz for DAC

////////////////////// Global Variables /////////////////////////////
typedef struct {
    unsigned int SrcAddr;
    unsigned int DestAddr;
    unsigned int LLI;
    unsigned int Control;
} LLI;

LLI LLIA2;
LLI LLIB2;
unsigned short buffA [BuffSize];
unsigned short buffB [BuffSize];
int freeBufferFlag = 0; // 0 for buffA, 1 for buffB. Indicates which buffer isn't being used by the DMA
int generatePaused = 0;
int bufferChanged = 0; //1 when buffer is swapped.

const int samplesPerBeat = (SampleRate * 60) / BBPM;
const int numNotes = 5;
int notes [NumNotes + 1] = {500, 1000, 2000, 4000, 8000, -1};
int beats [NumNotes + 1] = {31, 31, 31, 31, 31, -1};
/////////////////////////////////////////////////////////////////////
/////////////////// Function Declarations ///////////////////////////
void delay_us(int us);
void delay_ms(int ms);

void LCDwriteCommand(unsigned int cmd);
void LCDwriteData(char data);
void LCD_init();
void LCD_setCursor(unsigned int row, unsigned int col);
void LCD_displayString(char *str);
void LCD_defineCustomChar(unsigned int location, unsigned int *pattern);

void initNoteSystem(void);
void setDMABuffA(void);
void setDMABuffB(void);

void initialize(void);
/////////////////////////////////////////////////////////////////////
/////////////////////// Interrupt Functions /////////////////////////

int intCalled = 0;
void DMA_IRQHandler(void){
	generatePaused = 0;
	intCalled = !intCalled;
	bufferChanged = 1;
	DMACIntTCClear = 1;
	DMACIntErrClr = 1;
	freeBufferFlag = !freeBufferFlag;
}

/////////////////////////////////////////////////////////////////////
///////////////////////////// MAIN //////////////////////////////////

int main() {
    for (int i = 0; i < 4096; i++){
    	buffA[i] = 512;
    	buffB[i] = 512;
    }
    initialize();
    int n = 0, note = 0, sample = 0, lutIndex = 0;
    int lutIndexInc = notes[note] / LUTfreq;
    int samplesToGenerate = beats[note] * samplesPerBeat;
    while (1) {
        if (!generatePaused){
            if (bufferChanged){ //When buffer is swapped
                bufferChanged = 0;
                n = 0;
            	if (freeBufferFlag){
            		setDMABuffB();
            	} else {
            		setDMABuffA();
            	}
            }
            if (n >= 4096){ //When current buffer is full
                generatePaused = 1;
                continue;
            }
            if (sample >= samplesToGenerate){ //When current note is done generating
                note++;
                if (notes[note] == -1){
                	note = 0;
                }
                sample = 0;
                samplesToGenerate = beats[note] * samplesPerBeat;
                lutIndexInc = notes[note] / LUTfreq;
            }
            if (freeBufferFlag){
                buffA[n] = lut[(int) lutIndex];
                lutIndex = (lutIndex + lutIndexInc) % LUTLength;
                buffA[n] = buffA[n] << 6;
            } else {
                buffB[n] = lut[(int) lutIndex];
                lutIndex = (lutIndex + lutIndexInc) % LUTLength;
                buffB[n] = buffB[n] << 6;
            }
            n++;
            sample++;
        }

    }

    return 0;
}

/////////////////////////////////////////////////////////////////////
/////////////////// Function Definitions ////////////////////////////

void initialize(void){
    PLL0StartUpSeq(); // Set CPU Clock to 16Mhz

    LCD_init();

    // Define a custom character 
    unsigned int quarter[8] = {0x07, 0x07, 0x04, 0x04, 0x04, 0x1C, 0x1C, 0x1C};
    LCD_defineCustomChar(0, quarter);

    unsigned int eighth[8]  = {0x06, 0x06, 0x05, 0x05, 0x04, 0x1C, 0x1C, 0x1C};
    LCD_defineCustomChar(1, eighth);

    unsigned int half[8] = {0x07, 0x07, 0x04, 0x04, 0x04, 0x1C, 0x14, 0x1C};
    LCD_defineCustomChar(2, half);

    unsigned int sharp[8] = {0x0A, 0x0A, 0x1F, 0x0A, 0x0A, 0x1F, 0x0A, 0x0A};
    LCD_defineCustomChar(3, sharp);

    unsigned int flat[8] = {0x10, 0x10, 0x10, 0x16, 0x19, 0x12, 0x14, 0x18};
    LCD_defineCustomChar(4, flat);


    LLIA2.SrcAddr = (unsigned int) &buffA[2048];
    LLIA2.DestAddr = (unsigned int) &DACR;
    LLIA2.LLI = (unsigned int) 0;
    LLIA2.Control = transferSize | (1 << 18) | (1 << 21) | (1 << 26) | (1u << 31);

    LLIB2.SrcAddr = (unsigned int) &buffB[2048];
    LLIB2.DestAddr = (unsigned int) &DACR;
    LLIB2.LLI = (unsigned int) 0;
    LLIB2.Control = transferSize | (1 << 18) | (1 << 21) | (1 << 26) | (1u << 31);

    initNoteSystem();
}

// Delay functions
void delay_us(int us) {
	int value = us * 16;
	int start = T0.TC; // note starting time
	T0.TCR |= (1<<0); // start timer
	while ((T0.TC-start)<value) {} // wait for time to pass
}
void delay_ms(int ms) {
	delay_us(ms * 1000);
}

// Write a command to the LCD
void LCDwriteCommand(unsigned int cmd) {
    FIO[0].CLR = RS;            // RS = 0 for command
    FIO[0].CLR = RW;            // RW = 0 for write
    FIO[0].PIN = (cmd << 3);    // Set DB0-DB7
    FIO[0].SET = EN;            // E = 1 to enable
    delay_us(10);                       // Short delay for pulse
    FIO[0].CLR = EN;            // E = 0 to latch data
    delay_us(100);                     // Wait for command to complete
}

// Write data to the LCD
void LCDwriteData(char data) {
    FIO[0].PIN = (data << 3);   // Set DB0-DB7
    FIO[0].SET = RS;            // RS = 1 for data
    FIO[0].CLR = RW;            // RW = 0 for write
    FIO[0].SET = EN;            // E = 1 to enable
    delay_us(100);                       // Short delay for pulse
    FIO[0].CLR = EN;            // E = 0 to latch data
    delay_us(100);                     // Wait for data processing
}

// Initialize the LCD
void LCD_init() {
    FIO[0].DIR |= RS | RW | EN | D0 | D1 | D2 | D3 | D4 | D5 | D6 | D7;

    delay_ms(4);                       // Wait for LCD to power up
    LCDwriteCommand(0x38);             // 8-bit mode, 2-line display
    LCDwriteCommand(0x06);             // Auto-increment cursor
    LCDwriteCommand(0x0F);             // Display on, cursor off
    LCDwriteCommand(0x01);             // Clear display
    delay_ms(4);                       // Wait for clear command
}

// Set cursor to a specific position
void LCD_setCursor(unsigned int row, unsigned int col) {
    unsigned int address = col;
    switch (row) {
        case 0: address += 0x00; break;
        case 1: address += 0x40; break;
        case 2: address += 0x14; break;
        case 3: address += 0x54; break;
    }
    LCDwriteCommand(0x80 | address);   // Set cursor position
}

// Display a string on the LCD
void LCD_displayString(char *str) {
    while (*str) {
        LCDwriteData(*str++);          // Send each character
    }
}

// Define a custom character
void LCD_defineCustomChar(unsigned int location, unsigned int *pattern) {
    location &= 0x07;                  // Only 8 locations (0-7)
    LCDwriteCommand(0x40 | (location << 3));  // Set CGRAM address
    for (int i = 0; i < 8; i++) {
        LCDwriteData(pattern[i]);      // Write each row of the pattern
    }
    LCDwriteCommand(0x80);             // Return to DDRAM
}

void initNoteSystem(void){
    //Set P0.26 function to AOUT and mode to no pull resistors
    PINSEL[1] &= ~(3 << 20);
    PINMODE[1] &= (1 << 21);
    PINSEL[1] |= (1 << 21);

    //Set PCLK divider to 1 for DAC
    PCLKSEL0 |= (1 << 22);

    //Enable DMA Peripheral, enable DMA controller, disable Channel 0
    PCONP |= (1 << 29);
    DACCTRL = ~0xFu;
    DMACConfig |= 1;
    DMACC0.Config &= ~(1u);

    //Clear Interrupts for DMACC0
    DMACIntErrClr = 1;
    DMACIntTCClear = 1;
    setDMABuffA();

    //Set DMA Counter for DAC
    DACCNTVAL = PCLK / SampleRate;

    //Enable Interrupts
    ISER0 |= (1 << 26);

    //Enable DMA access and DMA counter
    DACCTRL |= (1 << 2) | (1 << 3);

}

//Make sure to disable the DMACC0 before programming it to avoid any issues
void setDMABuffA(void){
    DMACC0.Config &= ~1u;
    DMACC0.SrcAddr = (unsigned int) &buffA[0];
    DMACC0.DestAddr = (unsigned int) &DACR;
    DMACC0.Control = transferSize | (1 << 18) | (1 << 21) | (1 << 26);
	DMACC0.LLI = (unsigned int) &LLIA2;
    DMACC0.Config =
        (1) |       //Enable Ch0
        (7 << 6) |  //DAC Peripheral
        (1 << 11)|  //Memory to Peripheral
		(3 << 14);
    //Enable DMA Channel 0
    DMACC0.Config |= 1;
}

//Make sure to disable the DMACC0 before programming it to avoid any issues
void setDMABuffB(void){
    DMACC0.Config &= ~1u;
    DMACC0.SrcAddr = (unsigned int) &buffB[0];
    DMACC0.DestAddr = (unsigned int) &DACR;
    DMACC0.Control = transferSize | (1 << 18) | (1 << 21) | (1 << 26);
	DMACC0.LLI = (unsigned int) &LLIB2;
    DMACC0.Config =
        (1) |       //Enable Ch0
        (7 << 6) |  //DAC Peripheral
        (1 << 11)|  //Memory to Peripheral
		(3 << 14);
    //Enable DMA Channel 0
    DMACC0.Config |= 1;
}
