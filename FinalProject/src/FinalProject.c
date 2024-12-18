#include "DAC.h"
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

#define ISER0 	(*(volatile unsigned int *) 0xE000E100)

#define PCLKT0          16 		// MHz
#define DACMAXIMUM		1023
#define DUTYCYCLE		0.125	//0 to 1

#define BPM             103
#define ADSR_Fs         10 		// Khz
#define ADSR_CurveCntrl	16		// Increases overall effect of curve of ADSR
#define ADSR_MAXVAL     1023

/////////////////////////////////////////////////////////////////////
////////////////////// Structs and Enums /////////////////////////////
typedef struct {
	int state;
	int highTime;
	int lowTime;
	int beatTime;
} OutData;


/* Times are in Milliseconds.
 * sHeight and sRatio is a percentage from 0 to 1
 * Times are unaffected by BPM, but sRatio is affected by BPM
 * sRatio is how long to sustain the note after subtracting the ADR times from the total beat time.
 */
typedef struct{
    float aTime; float aCurve;
    float dTime; float dCurve;
    float sRatio;float sHeight;
    float rTime; float rCurve;
} ADSR;

enum Note {
	Sixteenth	= (PCLKT0 * 1000000 * 60) / (BPM * 4),
	DotSixteenth= (int) (Sixteenth + 0.5 * Sixteenth),
	Eight 		= (PCLKT0 * 1000000 * 60) / (BPM * 2),
	DotEight	= (int) (Eight + 0.5 * Eight),
	Quarter 	= (PCLKT0 * 1000000 * 60) / BPM,
	DotQuarter 	= (int) (Quarter + 0.5 * Quarter),
	Half		= (PCLKT0 * 1000000 * 60 * 2) / BPM,
	DotHalf 	= (int) (Half + 0.5 * Half),
	Whole		= ((PCLKT0 * 1000000 * 60) / BPM) * 4
};

enum NoteChar {
	QuarterChar,
	EightChar,
	HalfChar,
	SharpChar,
	FlatChar,
};

//A4 is 440 Hz
enum KeyNames {
                                                                                Key_A0, Key_Bb0, Key_B0,
    Key_C1, Key_Db1, Key_D1, Key_Eb1, Key_E1, Key_F1, Key_Gb1, Key_G1, Key_Ab1, Key_A1, Key_Bb1, Key_B1,
    Key_C2, Key_Db2, Key_D2, Key_Eb2, Key_E2, Key_F2, Key_Gb2, Key_G2, Key_Ab2, Key_A2, Key_Bb2, Key_B2,
    Key_C3, Key_Db3, Key_D3, Key_Eb3, Key_E3, Key_F3, Key_Gb3, Key_G3, Key_Ab3, Key_A3, Key_Bb3, Key_B3,
    Key_C4, Key_Db4, Key_D4, Key_Eb4, Key_E4, Key_F4, Key_Gb4, Key_G4, Key_Ab4, Key_A4, Key_Bb4, Key_B4,
    Key_C5, Key_Db5, Key_D5, Key_Eb5, Key_E5, Key_F5, Key_Gb5, Key_G5, Key_Ab5, Key_A5, Key_Bb5, Key_B5,
    Key_C6, Key_Db6, Key_D6, Key_Eb6, Key_E6, Key_F6, Key_Gb6, Key_G6, Key_Ab6, Key_A6, Key_Bb6, Key_B6,
    Key_C7, Key_Db7, Key_D7, Key_Eb7, Key_E7, Key_F7, Key_Gb7, Key_G7, Key_Ab7, Key_A7, Key_Bb7, Key_B7,
    Key_C8
};
/////////////////// Function Declarations ///////////////////////////
void initialize(void);

void delay_us(int us);
void delay_ms(int ms);

void LCDwriteCommand(unsigned int cmd);
void LCDwriteData(char data);
void LCD_init();
void LCD_setCursor(unsigned int row, unsigned int col);
void LCD_displayString(char *str);
void LCD_defineCustomChar(unsigned int location, unsigned int *pattern);

void calculateKeys(void);
void calculateADSR(void);
void changeSustainIdx(int beat);
double calculateADSRCurve(double x, double c);
int displayNote(int noteID, int charactersDisplayed);
/////////////////////////////////////////////////////////////////////
////////////////////// Global Variables /////////////////////////////
const int adsrFsCnt = (PCLKT0 * 1000000) / (ADSR_Fs * 1000);

const int MaxKeys = 88;
unsigned int KeyHighCount [88];
unsigned int KeyLowCount [88];


OutData outData;
//Selecting long ADSR can make notes of lower beats not reach later stages.
ADSR adsr = (ADSR) {
	.aTime = 25,   	.aCurve = -0.1,
    .dTime = 50,   	.dCurve = -0.14,
    .sRatio= 1,		.sHeight = 0.3,
    .rTime = 100,   .rCurve = -0.25
};
// Array size should be at least (ADSR_Fs * totalMilliseconds) + 1
volatile short adsrData [4100];

int sustainIdx = 0;
unsigned int sustainTimes[9];

int adsrSize;
volatile int adsrIdx;

int goNextNote = 0;
const int notes [] = {
		Key_A3, Key_A3, Key_A3, Key_F3, Key_C4,
		Key_A3, Key_F3, Key_C4, Key_A3,
		Key_E4, Key_E4, Key_E4, Key_F4, Key_C4,

		Key_Ab3, Key_F3, Key_C4, Key_A3,
		Key_A4, Key_A3, Key_A3, Key_A4, Key_Ab4, Key_G4,
		Key_Gb4, Key_F4, Key_Gb4, Key_Bb3, Key_Eb3, Key_D3, Key_Db3,

		Key_C4, Key_B3, Key_C4, Key_F3, Key_Ab3, Key_F3, Key_Ab3,
		Key_C4, Key_A3, Key_C4, Key_E3,
		Key_A4, Key_A3, Key_A3, Key_A4, Key_Ab4, Key_G4,

		Key_Gb4, Key_F4, Key_Gb4, Key_Bb3, Key_Eb3, Key_D3, Key_Db3,
		Key_C4, Key_B3, Key_C4, Key_F3, Key_Ab3, Key_F3, Key_C3,
		Key_A3, Key_F3, Key_C4, Key_A3,
		-1
};
const int beats [] = {
		Quarter, Quarter, Quarter, Eight, Eight,
		Quarter, Eight, Eight, Half,
		Quarter, Quarter, Quarter, Eight, Eight,

		Quarter, Eight, Eight, Half,
		Quarter, Eight, Eight, Quarter, Eight, Eight,
		Eight, Eight, Eight, Eight, Quarter, Eight, Eight,

		Eight, Eight, Eight, Eight, Quarter, Eight, Eight,
		Quarter, Eight, Eight, Half,
		Quarter, Eight, Eight, Quarter, Eight, Eight,

		Eight, Eight, Eight, Eight, Quarter, Eight, Eight,
		Eight, Eight, Eight, Eight, Quarter, Eight, Eight,
		Quarter, Eight, Eight, Half,
		-1
};
/////////////////////////////////////////////////////////////////////
/////////////////////// Interrupt Functions /////////////////////////
void TIMER0_IRQHandler(void){
    if (T0.IR & 1){
    	if (outData.state){
    		if (adsrSize <= 1){
    			DACR = 1023 << 6;
    		} else if (adsrData[adsrIdx] == -1){
    			DACR = ((unsigned int) adsrData[adsrIdx + 1]) << 6;
    		} else {
    			DACR = ((unsigned int) adsrData[adsrIdx]) << 6;
    		}
    		outData.state = 0;
    		T0.MR[0] += outData.highTime;
    	} else {
        	DACR = 0;
        	outData.state = 1;
    		T0.MR[0] += outData.lowTime;
    	}
        T0.IR = 1;
    }
    if (T0.IR & 2){
    	goNextNote = 1;
        T0.MR[2] = T0.TC + adsrFsCnt;
    	T0.IR = 2;
    }
    if (T0.IR & 4){
        if (adsrData[adsrIdx] == -1 && sustainTimes[sustainIdx] != 0){
			T0.MR[2] = T0.TC + sustainTimes[sustainIdx];
        } else {
            T0.MR[2] = T0.TC + adsrFsCnt;
        }
        if (adsrIdx < adsrSize){
            adsrIdx++;
        }
        T0.IR = 4;
    }
}
/////////////////////////////////////////////////////////////////////
///////////////////////////// MAIN //////////////////////////////////

float BeatCount;
int CurrRow = 0;
int main() {
    initialize();
    int note = 0;
    BeatCount = 0;
    int charactersDisplayed = 1;
    while (1) {
    	if (goNextNote){
    		note++;
    		goNextNote = 0;
    		if (notes[note] == -1){
    			note = 0;
    			//delay_ms(2000);
    		}
    		changeSustainIdx(beats[note]);
            adsrIdx = 0;
    		outData.highTime = KeyHighCount[notes[note]];
    		outData.lowTime = KeyLowCount[notes[note]];
        	T0.MR[1] += beats[note];

        	if (beats[note] == Quarter){
        		BeatCount += 1;
        	}
        	if (beats[note] == Half){
        		BeatCount += 2;
        	}
        	if (beats[note] == Eight) {
        		BeatCount += 0.5;
        	}

        	if (BeatCount >= 12){
        		if (CurrRow == 0){
            		LCD_setCursor(1 , 0);
        		}
        		if (CurrRow == 1){
        			LCD_setCursor(0 , 20);
        		}
        		if (CurrRow == 2){
        			LCD_setCursor(1 , 20);
        		}
        		if (CurrRow == 3){
        			LCD_setCursor(0 , 0);
        		}
        		CurrRow = (CurrRow + 1) % 4;
        		BeatCount = 0;
        		charactersDisplayed = 0;
        	}
        	if (charactersDisplayed < 20){
        		charactersDisplayed += displayNote(note, charactersDisplayed);
        	}
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

    //Set P0.26 function to AOUT and mode to no pull resistors
    PINSEL[1] &= ~(3 << 20);
    PINMODE[1] &= (1 << 21);
    PINSEL[1] |= (1 << 21);

    calculateKeys();
    calculateADSR();
    adsrIdx = 0;

    LCD_setCursor(0, 0);


    changeSustainIdx(beats[0]);
    outData.beatTime = beats[0];
    outData.highTime = KeyHighCount[notes[0]];
    outData.lowTime = KeyLowCount[notes[0]];
	outData.state = 1;
	displayNote(0, 0);

    //Enable Timer0 Match Register Channel 0
    T0.MR[0] = T0.TC + outData.highTime;
    T0.MCR |= 1;
    T0.MR[1] = T0.TC + outData.beatTime;
    T0.MCR |= 1 << 3;
    T0.MR[2] = T0.TC + adsrFsCnt;
    T0.MCR |= 1 << 6;

    PCLKSEL0 |= (1 << 2);	//Timer0 PCLKT0 = CCLK
    T0.IR = 0xF;          	//Clear MR Interupt flags
    T0.TCR |= 1;          	//Enable Timer0 counter
    ISER0 = (1 << 1);     	//Enable Timer0 interrupts
}

// Delay functions
void delay_us(int us) {
	int value = us * PCLKT0;
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

/* Calculates the Count values needed for each key to output that frequency.
 * For 50% duty cycle, the count for a frequency, f, is PCLK / (2f)
 */
void calculateKeys(void){
	unsigned int totalCount;
    for (int i = 1; i <= MaxKeys; i++){
    	totalCount = (unsigned int) ((PCLKT0 * 1000000.0) / (440.0 * powf(2.0, (i - 49.0) / 12.0)));
        KeyHighCount[i - 1] = (unsigned int) totalCount * DUTYCYCLE;
        KeyLowCount[i - 1] = (unsigned int) totalCount - KeyHighCount[i - 1];
    }
}

void calculateADSR(void){
    int atkSize = ADSR_Fs * (adsr.aTime);
    int decSize = ADSR_Fs * (adsr.dTime);
    int relSize = ADSR_Fs * (adsr.rTime);

    adsrSize = atkSize + decSize + relSize + 1;

    sustainTimes[0] = (Sixteenth - adsrSize * ((PCLKT0 * 1000) / ADSR_Fs)) * adsr.sRatio;
    sustainTimes[1] = (DotSixteenth - adsrSize * ((PCLKT0 * 1000) / ADSR_Fs)) * adsr.sRatio;
    sustainTimes[2] = (Eight - adsrSize * ((PCLKT0 * 1000) / ADSR_Fs)) * adsr.sRatio;
    sustainTimes[3] = (DotEight - adsrSize * ((PCLKT0 * 1000) / ADSR_Fs)) * adsr.sRatio;
    sustainTimes[4] = (Quarter - adsrSize * ((PCLKT0 * 1000) / ADSR_Fs)) * adsr.sRatio;
    sustainTimes[5] = (DotQuarter - adsrSize * ((PCLKT0 * 1000) / ADSR_Fs)) * adsr.sRatio;
    sustainTimes[6] = (Half - adsrSize * ((PCLKT0 * 1000) / ADSR_Fs)) * adsr.sRatio;
    sustainTimes[7] = (DotHalf - adsrSize * ((PCLKT0 * 1000) / ADSR_Fs)) * adsr.sRatio;
    sustainTimes[8] = (Whole - adsrSize * ((PCLKT0 * 1000) / ADSR_Fs)) * adsr.sRatio;

    for (int i = 0; i < adsrSize; i++){
        adsrData[i] = 0;
    }
    int idxShift = 0;
    int idx = 0;
    for (int i = 0; i < atkSize; i++){
        adsrData[idx] = (short) ADSR_MAXVAL * calculateADSRCurve(idx / (float) atkSize, adsr.aCurve);
        idx += 1;
    }

    idxShift = atkSize;
    for (int i = 0; i < decSize; i++){
        adsrData[idx] = (short) ADSR_MAXVAL * (1 - (calculateADSRCurve(((idx - idxShift) / (float) decSize), adsr.dCurve) * (1 - adsr.sHeight)));
        idx += 1;
    }

    adsrData[idx++] = -1; //Indicates Sustain Start
    idxShift = atkSize + decSize;
    for (int i = 0; i < relSize; i++){
        adsrData[idx] = (short) ADSR_MAXVAL * ((adsr.sHeight) * (1 - calculateADSRCurve((idx - idxShift) / (float) relSize, adsr.rCurve)));
        idx += 1;
    }

}

double calculateADSRCurve(double x, double c){
    return ((pow(2.0, ADSR_CurveCntrl * c * x) - 1) / (pow(2.0, ADSR_CurveCntrl * c) - 1));
}

void changeSustainIdx(int beat){
	switch (beat){
        case Sixteenth:
            sustainIdx = 0; break;
        case DotSixteenth:
            sustainIdx = 1; break;
        case Eight:
            sustainIdx = 2; break;
        case DotEight:
            sustainIdx = 3; break;
        case Quarter:
            sustainIdx = 4; break;
        case DotQuarter:
            sustainIdx = 5; break;
        case Half:
            sustainIdx = 6; break;
        case DotHalf:
            sustainIdx = 7; break;
        case Whole:
            sustainIdx = 8; break;
        default:
            sustainIdx = 0;
    }
}

int displayNote(int note, int charactersDisplayed){
	int charactersSent = 0;
	if (beats[note] == Quarter){
		LCDwriteData(QuarterChar);
		charactersSent++;
	}
	if (beats[note] == Half){
		LCDwriteData(HalfChar);
		charactersSent++;
	}
	if (beats[note] == Eight){
		LCDwriteData(EightChar);
		charactersSent++;
	}
	int noteInOct = notes[note] % 12;
	if ((noteInOct == 1 || noteInOct == 4 || noteInOct == 6 || noteInOct == 9 || noteInOct == 11) && (charactersSent + charactersDisplayed) != 20){
		LCDwriteData(FlatChar);
		charactersSent++;
	}
	return charactersSent;
}
