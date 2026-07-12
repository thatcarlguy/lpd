// LPD Board Evaluation
//
//  Board - ATTiny 1604 (not optiboot)
//  Speed - 20 MHz (using 5V)
//  Programmer - SerialUPDI - SLOW: 57600 baud.
//
// 
//
// Test out the RS485 communication 
//  
// https://www.come-star.com/blog/how-far-can-rs485-transmit-baud-rate-vs-distance-explained/
//   9600 baud -> 4000 ft
//  19200 baud -> 3000 ft
//  38400 baud -> 2500 ft
//  57600 baud -> 1500 ft
// 115200 baud -> 1000 ft
//  
// https://www.e-tinkers.com/2024/09/attiny3227-programming-uart/
//
//   PA1 - SPI/MOSI    RS485 output, Analog output control
//   PA2 - SPI/MISO    RS485 output, Analog output control
//   PA3 - SPI/SCK     RS485 output, Analog output control
//
//
// Carl Hall, carl@LEAero.us
// 20260706

#include <avr/io.h>
#include <util/delay.h>   // provides _delay_us() and _delay_ms()
#include <stdio.h>

// Set baudrate for RS485
//#define BAUD_RATE 115200
#define BAUD_RATE 57600
//#define BAUD_RATE 9600

static void USART0_sendChar(char c) {
  while (!(USART0.STATUS & USART_DREIF_bm)); //Wait for Data Register Empty Interrupt Flag is set
  USART0.TXDATAL = c;
}

void setup(){
  // Configure Status LED
  PORTB.DIRSET = PIN3_bm;

  // Configure AO/DO Select
  PORTB.DIRSET = PIN0_bm;     // set PB0 to OUTPUT
  PORTB.OUTSET = PIN0_bm;     // logical high for AO, to disable talking to digitizer
  PORTB.DIRSET = PIN1_bm;     // set PB1 to OUTPUT
  PORTB.OUTSET = PIN1_bm;     // logical high for DO, to enable RS485 send

  // Configure RS485 
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, !CLKCTRL_PEN_bm);     // clear Prescalar to run at 20 MHz (needed?)
  while (!(CLKCTRL.MCLKSTATUS & CLKCTRL_OSC20MS_bm)) {};
  
  PORTMUX.CTRLB |= PORTMUX_USART0_bm;     // Select the alternate pin positions for USART0
  
  PORTA.DIRSET = PIN1_bm;   // Configure TXD as output
  PORTA.DIRCLR = PIN2_bm;   // Configure RXD as input (not currently used)
  PORTA.DIRSET = PIN3_bm;   // Configure XCK as output

  USART0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | 
                 USART_PMODE_DISABLED_gc | 
                 USART_SBMODE_1BIT_gc | 
                 USART_CHSIZE_8BIT_gc;          // Set Frame Format: 8 data bits, 1 stop bit, no parity
  
  USART0.BAUD = (uint16_t) (F_CPU * 64 / (16 * (float) BAUD_RATE) + 0.5);  // Configure USART Baud Rate
  
  //USART0.CTRLB = USART_TXEN_bm | USART_RXEN_bm;     // enable transmit and recieve
  USART0.CTRLB |= USART_TXEN_bm;                      // enable transmit only
  
}

void loop(){
  
  PORTB.OUTCLR = PIN3_bm;     // Red
  
  char str[] = "Hello World\r\n";
  for (size_t i=0; i< strlen(str); i++) {
    USART0_sendChar(str[i]);
  }
  _delay_ms(100);

  PORTB.OUTSET = PIN3_bm;     // Green
  _delay_ms(900);

}







