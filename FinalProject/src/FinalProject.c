#include "NoteOutput.h"
#include "Clocking.h"
#include "Timer.h"
#include "FIO.h"

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
    delay_us(1);                       // Short delay for pulse
    FIO[0].CLR = EN;            // E = 0 to latch data
    delay_us(100);                     // Wait for command to complete
}

// Write data to the LCD
void LCDwriteData(unsigned int data) {
    FIO[0].SET = RS;            // RS = 1 for data
    FIO[0].CLR = RW;            // RW = 0 for write
    FIO[0].PIN = (data << 3);   // Set DB0-DB7
    FIO[0].SET = EN;            // E = 1 to enable
    delay_us(1);                       // Short delay for pulse
    FIO[0].CLR = EN;            // E = 0 to latch data
    delay_us(100);                     // Wait for data processing
}

// Initialize the LCD
void LCD_init() {
    FIO[0].DIR |= RS | RW | EN | D0 | D1 | D2 | D3 | D4 | D5 | D6 | D7;

    delay_ms(4);                       // Wait for LCD to power up
    LCDwriteCommand(0x38);             // 8-bit mode, 2-line display
    LCDwriteCommand(0x06);             // Auto-increment cursor
    LCDwriteCommand(0x0C);             // Display on, cursor off
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

// Main function
int main() {
    PLL0StartUpSeq();				   // Set CPU Clock to 16Mhz
    initNoteSystem();				   // Initialize the note output system.
	LCD_init();                        // Initialize the LCD


    // Define a custom character 
    //unsigned int empty[8] = {0x07, 0x07, 0x04, 0x04, 0x04, 0x1C, 0x1C, 0x1C};
    //LCD_defineCustomChar(0, empty);

    LCD_setCursor(0, 0);               // Set cursor to top left
    LCD_displayString("String Value");

    LCD_setCursor(1, 0);               // Set cursor to second line
    LCD_displayString("Custom Char: ");
    LCDwriteData(0x41);                // Display custom empty character

    while (1) {
        // Main loop (idle)
    }

    return 0;
}
