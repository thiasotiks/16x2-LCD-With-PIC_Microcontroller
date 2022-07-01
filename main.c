/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * ~~~~~~~~~~~~ Interfacing (4-bit Mode) a 16x2 LCD with PIC16F676 [Rev. 1.0] ~~~~~~~~~~~~
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * 
 * 
 * This code is under MIT License
 * Copyright (c) 2022 Sayantan Sinha
*/

// PIC16F676 Configuration Bit Settings
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator: High Speed crystal on RA4/OSC2/CLKOUT and RA5/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = ON       // Power-up Timer Enable bit (PWRT enabled)
#pragma config MCLRE = OFF      // RA3/MCLR pin function select (RA3/MCLR pin function is digital I/O, MCLR internally tied to VDD)
#pragma config BOREN = OFF      // Brown-out Detect Enable bit (BOD disabled)
#pragma config CP = OFF         // Code Protection bit (Program Memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)

#include <xc.h>

#define _XTAL_FREQ 20000000

// Define the macros for MCU pins connected to LCD D4:7, EN, and RS
#define D4 RC0
#define D5 RC1
#define D6 RC2
#define D7 RC3
#define RS RC4
#define EN RC5

// Commands templates (See Hitachi HD44780U data sheet p. 28)
#define LCD_CLR_DISP 0x01              
#define LCD_RET_HOME 0x02
#define LCD_ENTRY_MODE_SET 0x04
#define LCD_DISP_CON 0x08
#define LCD_CUR_DISP_SHIFT 0x10
#define LCD_FUNC_SET 0x20
#define LCD_SET_CGRAM_ADDR 0x40
#define LCD_SET_DDRAM_ADDR 0x80

// Entry mode set command-bits
#define LCD_ENTRY_RIGHT 0x00          
#define LCD_ENTRY_LEFT 0x02
#define LCD_ENTRY_DISP_SHIFT 0x01
#define LCD_ENTRY_CUR_SHIFT 0x00

// Display on/off control command-bits
#define LCD_DISP_ON 0x04              
#define LCD_DISP_OFF 0x00
#define LCD_CUR_ON 0x02
#define LCD_CUR_OFF 0x00
#define LCD_BLINK_ON 0x01
#define LCD_BLINK_OFF 0x00

// Display cursor control command-bits (Hitachi HD44780 p. 27, 29)
#define LCD_DISP_SHIFT_L 0x08                       // Shifts the entire display to the left
#define LCD_DISP_SHIFT_R 0x0C                       // Shifts the entire display to the right
#define LCD_CUR_SHIFT_L 0x00                        // Shifts the cursor position to the left
#define LCD_CUR_SHIFT_R 0x04                        // Shifts the cursor position to the right

// Display function set command-bits
#define LCD_8BIT_MODE 0x10            
#define LCD_4BIT_MODE 0x00
#define LCD_2LINE_DISP 0x08
#define LCD_1LINE_DISP 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

unsigned char _dispFunc;
unsigned char _dispCon;
unsigned char _dispMode;

void lcdSend4bit(unsigned char data);                                 // Sends 4 bit data to LCD and provide a 37 us delay afterwaards           
void lcdCmd(unsigned char data);                                      // Sends a command word to LCD
void lcdWrite(unsigned char data);                                    // Sends a character to LCD
void lcdClear(void);                                                  // Clears the LCD and set the cursor in (0,0) pos
void lcdHome (void);                                                  // Sets the cursor position to (0,0)
void lcdSetCursor (unsigned char col, unsigned char row);             // Set cursor position (column#, row#)
void lcdDispOn(void);                                                 // Turns on the display
void lcdDispOff(void);                                                // Turns off the display. Display data remains in DDRAM, and can be displayed instantly by turning it on
void lcdCursor (void);                                                // Makes the cursor visible
void lcdNoCursor (void);                                              // Hides the cursor
void lcdBlink(void);                                                  // Blinks the cursor
void lcdNoBlink (void);                                               // Turns of blinking of the cursor
void lcdShiftDisplayLeft(void);                                       // Shifts the entire display left
void lcdShiftDisplayRight(void);                                      // Shifts the entire display Right
void lcdShiftCursorLeft(void);                                        // Shifts cursor position to the left without writing a character
void lcdShiftCursorRight(void);                                       // Shifts cursor position to the right without writing a character
void lcdLeftToRight (void);                                           // Characters appear from the left of the display (cursor shifts right)
void lcdRightToLeft (void);                                           // Characters appear from the right of the display (cursor shifts left)
void lcdAutoScroll (void);                                            // Shifts the entire display either to the right (I/D = 0) or to the left 
void lcdNoAutoScroll (void);                                          // Shifts the cursor, NOT the display
void lcdCreateChar(unsigned char location, unsigned char charmap[]);  // Create your own character and store in the CGRAM
void lcdInit(void);                                                   // Initializes the LCD Module
void lcdPrint(char *data);

void main()
{
    ANSEL = 0;
    TRISC = 0;
    lcdInit();
    lcdPrint("Hello World");
    __delay_ms(2000);
    lcdClear();
    
    char *txt = "Please Like, Share, Subscribe!";  
    unsigned char customChar[8] = {0x00, 0x0A, 0x15, 0x11, 0x11, 0x0A, 0x04, 0x00}; 
    unsigned char i = 0;
    
    lcdCreateChar(0, customChar);

    lcdSetCursor(2, 0);
    lcdWrite(0);
    lcdPrint(" Welcome ");
    lcdWrite(0);
    
    __delay_ms(2000);
    
    lcdClear();
    lcdPrint(txt);
    lcdSetCursor(0, 1);
    lcdPrint("https://www.youtube.com/c/Thiasotiks");
    
    while(1) {
        if(txt[i] != '\0') {
            lcdShiftDisplayLeft();
            i++;
        }
        else {
            i = 0;
            lcdSetCursor(0, 0);
        }
        __delay_ms(300);
    }
}

void lcdSend4bit(unsigned char data)
{
    D4 = data & 0x01;
    D5 = (data >> 1) & 0x01;
    D6 = (data >> 2) & 0x01;
    D7 = (data >> 3) & 0x01;
    
    EN = 1;
    __delay_us(1);
    EN = 0;
    __delay_us(40);
}

void lcdCmd(unsigned char data)
{
    RS = 0;
    lcdSend4bit(data >> 4);
    lcdSend4bit(data);
}

void lcdWrite(unsigned char data)
{
    RS = 1;
    lcdSend4bit(data >> 4);
    lcdSend4bit(data);
}

void lcdClear(void)
{
  lcdCmd(LCD_CLR_DISP);                                   // Clear display, set cursor position to zero
  __delay_ms(2);
}

void lcdHome (void)
{
  lcdCmd (LCD_RET_HOME);                                  // Set cursor position to zero
  __delay_ms (2);
}

void lcdSetCursor (unsigned char col, unsigned char row)  // Position the cursor. Col & Row starts from 0
{
  if (row >= 2)                                           // Max no. of rows = 2 (for 2-line display)
    row = 1;                                              // #row starts with zero
  unsigned char _rowOffset[2] = {0x00, 0x40};             // Row offset values corresponding to row #1, row #2 
  lcdCmd (LCD_SET_DDRAM_ADDR | (col + _rowOffset[row]));
}

void lcdDispOn(void)                                 // Turns on the display
{
  _dispCon |= LCD_DISP_ON;                           // D = 1 (See Hitachi HD44780U data sheet p. 26)
  lcdCmd(LCD_DISP_CON | _dispCon);
}

void lcdDispOff(void)                                // Turns off the display. Display data remains in DDRAM, and can be displayed instantly by turning it on
{
  _dispCon &= ~LCD_DISP_ON;                          // D = 0 (See Hitachi HD44780U data sheet p. 26)
  lcdCmd(LCD_DISP_CON | _dispCon);
}

void lcdCursor (void)
{
  _dispCon |= LCD_CUR_ON;                            // C = 1 (See Hitachi HD44780U data sheet p. 26)
  lcdCmd (LCD_DISP_CON | _dispCon);
}

void lcdNoCursor (void)
{
  _dispCon &= ~LCD_CUR_ON;                           // C = 0 (See Hitachi HD44780U data sheet p. 26)
  lcdCmd (LCD_DISP_CON | _dispCon);
}

void lcdBlink(void)                                  // Blinks the cursor
{
  _dispCon |= LCD_BLINK_ON;                          // B = 1 (See Hitachi HD44780U data sheet p. 26)
  lcdCmd (_dispCon);
}

void lcdNoBlink (void)                               // Stops blinking of cursor
{
  _dispCon &= ~LCD_BLINK_ON;                         // B = 0 (See Hitachi HD44780U data sheet p. 26)
  lcdCmd (_dispCon);
}

void lcdShiftDisplayLeft(void)                       // Shifts the entire display left
{
  lcdCmd (LCD_CUR_DISP_SHIFT | LCD_DISP_SHIFT_L);    // S/C = 1, R/L = 0 (See Hitachi HD44780U data sheet p. 29)
}

void lcdShiftDisplayRight(void)                      // Shifts the entire display right
{
  lcdCmd (LCD_CUR_DISP_SHIFT | LCD_DISP_SHIFT_R);    // S/C = 1, R/L = 1 (See Hitachi HD44780U data sheet p. 29)
}

void lcdShiftCursorLeft(void)                        // Shifts cursor position to the left without writing a character
{
    lcdCmd(LCD_CUR_DISP_SHIFT | LCD_CUR_SHIFT_L);    // S/C = 0, R/L = 0 (See Hitachi HD44780U data sheet p. 29)
}

void lcdShiftCursorRight(void)                       // Shifts cursor position to the right without writing a character
{
    lcdCmd(LCD_CUR_DISP_SHIFT | LCD_CUR_SHIFT_R);    // S/C = 0, R/L = 1 (See Hitachi HD44780U data sheet p. 29)
}

void lcdLeftToRight (void)                           // Characters appear from the left of the display (cursor shifts right)
{
  _dispMode |= LCD_ENTRY_LEFT;                       // I/D = 1 (See Hitachi HD44780U data sheet p. 26)
  lcdCmd (LCD_ENTRY_MODE_SET | _dispMode);
}

void lcdRightToLeft (void)                           // Characters appear from the right of the display (cursor shifts left)
{
  _dispMode &= ~LCD_ENTRY_LEFT;                      // I/D = 0 (See Hitachi HD44780U data sheet p. 26)
  lcdCmd (LCD_ENTRY_MODE_SET | _dispMode);
}

void lcdAutoScroll (void)                            // Shifts the entire display either to the right (I/D = 0) or to the left (I/D = 1). Cursor does not move
{
  _dispMode |= LCD_ENTRY_DISP_SHIFT;                 // S = 1 (See Hitachi HD44780U data sheet p. 26)
  lcdCmd (LCD_ENTRY_MODE_SET | _dispMode);
}

void lcdNoAutoScroll (void)                          // Shifts the cursor, NOT the display
{
  _dispMode &= ~LCD_ENTRY_DISP_SHIFT;                // S = 0 (See Hitachi HD44780U data sheet p. 26)
  lcdCmd (LCD_ENTRY_MODE_SET | _dispMode);
}

void lcdCreateChar(unsigned char location, unsigned char *charmap)  // Create your own character and store in the CGRAM (See Hitachi HD44780U data sheet p. 19)
{
  location &= 0x07;                                    // Mask bit-3 to bit-7
  lcdCmd (LCD_SET_CGRAM_ADDR | (location << 3));       // Write data to CGRAM. CGRAM address is 6-bit long. (DB5:DB3 = Char Location, DB2:DB0 = Dot_Matrix_Row#)
  for (int i = 0; i < 8; i++)
    lcdWrite (charmap[i]);                             // Write Dot pattern to the CGRAM
  lcdCmd(LCD_SET_DDRAM_ADDR);                          // Write data to DDRAM (Display Data RAM)
}


void lcdInit(void)                 // Initializes the LCD Module ( See Hitachi HD44780 data sheet, fig. 24, p. 46)
{
    __delay_ms(50);                // Wait at least 40 ms after power rises above 2.7V
    
    RS = 0;
    EN = 0;
    
    lcdSend4bit(0x03);
    __delay_ms(5);
    lcdSend4bit(0x03);
    __delay_us(150);
    lcdSend4bit(0x03);
    lcdSend4bit(0x02);
    
    _dispFunc = LCD_4BIT_MODE | LCD_2LINE_DISP | LCD_5x8DOTS;          // Display Function set: DL = 0 (4-bit), N = 1 (2-line disp), F = X (5x8 dot font)
    lcdCmd(LCD_FUNC_SET | _dispFunc);
    
    _dispCon = LCD_DISP_ON | LCD_CUR_OFF | LCD_BLINK_OFF;              // Display on/off ctrl.: D = 0 (disp off), C = 0 (cursor off), B = 0 (blink off)
    lcdCmd(LCD_DISP_CON | _dispCon);
    
    lcdClear();
    
    _dispMode = LCD_ENTRY_LEFT | LCD_ENTRY_CUR_SHIFT;                  // Entry mode set: I/D = 1 (Cursor shifts right), No display Shift (Cursor moves)
    lcdCmd(LCD_ENTRY_MODE_SET | _dispMode);
}

void lcdPrint(char *data)
{
    for(int i = 0; data[i] != '\0'; i++) {
        lcdWrite(data[i]);
    }
}
