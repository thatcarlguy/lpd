// LPD Board Evaluation
//
//  Board - ATTiny 1604 (not optiboot)
//  Speed - 20 MHz (using 5V)
//  Programmer - SerialUPDI - SLOW: 57600 baud.
//
// 
//
// Test out the SPI communication 
//  
//
//
// Carl Hall, carl@LEAero.us
// 20260706

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdint.h>

// Current version
#define HARDWARE_VERSION "Hardware Ver: 0.0.0, 20260712\r\n"
#define FIRMWARE_VERSION "Firmware Ver: 0.0.0, 20260712\r\n"

// Set baudrate for RS485
//#define BAUD_RATE 115200
#define BAUD_RATE 57600
//#define BAUD_RATE 9600

// Configure output update rate for output (32768 Hz base period)
#define OUTPUT_RATE RTC_PERIOD_CYC32768_gc  // 1 Hz
//#define OUTPUT_RATE RTC_PERIOD_CYC16384_gc  // 2 Hz
//#define OUTPUT_RATE RTC_PERIOD_CYC8192_gc   // 4 Hz
//#define OUTPUT_RATE RTC_PERIOD_CYC4096_gc   // 8 Hz
//#define OUTPUT_RATE RTC_PERIOD_CYC2048_gc   // 16 Hz

uint16_t debug_value = 65535;
char print_string[256];

static void USART_sendChar(char c) {
  while (!(USART0.STATUS & USART_DREIF_bm)); //Wait for Data Register Empty Interrupt Flag is set
  USART0.TXDATAL = c;
}

static void USART_sendString(const char *str) {
    while (*str != '\0') {
        USART_sendChar(*str);
        str++;
    }
    while (!(USART0.STATUS & USART_DREIF_bm)); //Wait for Data Register Empty Interrupt Flag is set
}



ISR(RTC_PIT_vect) {
  RTC.PITINTFLAGS = RTC_PI_bm;    // Clear the Periodic Interrupt flag
  // store and reset the counter, maybe the below function has a parameter of counts?
  update_outputs();               // Update the outputs
}

void setup(){
  // Configure Status LED
  PORTB.DIRSET = PIN3_bm;

  // Configure AO/DO Select
  PORTB.DIRSET = PIN0_bm;     // set PB0 to OUTPUT
  PORTB.OUTSET = PIN0_bm;     // disable analog output
  PORTB.DIRSET = PIN1_bm;     // set PB1 to OUTPUT
  PORTB.OUTCLR = PIN1_bm;     // disable RS485

  // Configure USART for SPI and RS485
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, !CLKCTRL_PEN_bm);     // clear Prescalar to run at 20 MHz (needed?)
  while (!(CLKCTRL.MCLKSTATUS & CLKCTRL_OSC20MS_bm)) {};
  PORTA.DIRSET = PIN1_bm;   // Configure TXD/MOSI as output
  PORTA.DIRCLR = PIN2_bm;   // Configure RXD/MISO as input (not currently used)
  PORTA.DIRSET = PIN3_bm;   // Configure CLK/SCK as output
  PORTMUX.CTRLB |= PORTMUX_USART0_bm;     // Select the alternate pin positions for USART0
  USART0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc | USART_SBMODE_1BIT_gc | USART_CHSIZE_8BIT_gc;   // Set Frame Format for RS485: 8 data bits, 1 stop bit, no parity
  
  USART0.BAUD = (uint16_t) (F_CPU * 64 / (16 * (float) BAUD_RATE) + 0.5);  // Configure USART Baud Rate
  USART0.CTRLB = USART_TXEN_bm; // start transmission
  
  // Print out to RS485 some debug info
  PORTB.OUTSET = PIN1_bm;     // enable RS485
  _delay_ms(100);
  USART_sendString("Laser Particle Detector\r\n");
  USART_sendString(HARDWARE_VERSION);
  USART_sendString(FIRMWARE_VERSION);
  _delay_ms(1);                             // without this the \r\n is cut off for some reason
  PORTB.OUTCLR = PIN1_bm;     // disable RS485

  // Setup polling routine to update outputs
  RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;                // set internal clock @ 32768 Hz
  RTC.PITINTCTRL = RTC_PI_bm;                       // enable PIT interrupt
  RTC.PITCTRLA = OUTPUT_RATE | RTC_PITEN_bm;        // Start periodic interrupt timer
}

void update_outputs(){
  
  PORTB.OUTCLR = PIN3_bm;     // Red
  
  ///////////////////////////////////////////////////////////////////////////////////
  // Analog/Digital Inputs
  
  // read state of interlock
  
  // read value of laser power
  
  // read value of board temperature









  ///////////////////////////////////////////////////////////////////////////////////
  // Analog Outputs

  // Setup for SPI communication to analog digitizer
  USART0.CTRLC = USART_CMODE_MSPI_gc; 
  USART0.BAUD = 128;                            // baudrate is 1/32 of perif clock (this is hardset along with manual delays, so don't change independently)
  PORTB.OUTCLR = PIN0_bm;                       // configure analog output digitizer to listen

  // Update Analog Output Voltage
  USART_sendChar(0b00010000);                  // Update DAC output immediately
  USART_sendChar((debug_value >> 8) & 0xFF);   // upper 8 bits
  USART_sendChar(debug_value & 0xFF);          // lower 8 bits
  while (!(USART0.STATUS & USART_DREIF_bm));
  
  // Update Analog Output Current
  USART_sendChar(0b00100100);                  // Update DAC output immediately
  USART_sendChar((debug_value >> 8) & 0xFF);   // upper 8 bits
  USART_sendChar(debug_value & 0xFF);          // lower 8 bits
  while (!(USART0.STATUS & USART_DREIF_bm));
  
  _delay_us(10);              // not sure why this is needed
  PORTB.OUTSET = PIN0_bm;     // configure digitizer to stop listening

  ///////////////////////////////////////////////////////////////////////////////////
  // Digital Output
  
  // Setup for RS485 communication
  USART0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc | USART_SBMODE_1BIT_gc | USART_CHSIZE_8BIT_gc;          // Set Frame Format for RS485: 8 data bits, 1 stop bit, no parity
  USART0.BAUD = (uint16_t) (F_CPU * 64 / (16 * (float) BAUD_RATE) + 0.5);  // Configure USART Baud Rate
  PORTB.OUTSET = PIN1_bm;                 // Enable RS485 chip
  
  sprintf(print_string, "Readings: ### Hits, ### mW, ### C, RUNNING/NOT RUNNING, %u\r\n", debug_value);
  USART_sendString(print_string);
  _delay_ms(1);                             // without this the \r\n is cut off for some reason
  PORTB.OUTCLR = PIN1_bm;                 // Disable RS485
  
  ///////////////////////////////////////////////////////////////////////////////////

  PORTB.OUTSET = PIN3_bm;     // Green

  // update value of debug level variable
  if (debug_value>16000){ debug_value = 0;    }
  else{                   debug_value = 65535;}
}


void loop(){}
