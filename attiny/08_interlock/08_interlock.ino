// LPD Board Evaluation
//
//  Board - ATTiny 1604 (not optiboot)
//  Speed - 20 MHz (using 5V)
//  Programmer - SerialUPDI - SLOW: 57600 baud.
//
// https://learn.adafruit.com/adafruit-updi-friend/advanced-reprogramming-with-updi
// https://wolles-elektronikkiste.de/en/programming-the-tinyavr-series-0-1-2-part-1
//
// Read an analog voltage put on PA4 (was counter, will be lsrsence)
//
// Carl Hall, carl@LEAero.us
// 20260712

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

// Set baudrate for RS485
//#define BAUD_RATE 115200
#define BAUD_RATE 57600
//#define BAUD_RATE 9600

char print_string[256];
volatile bool event = false;


static void USART0_sendChar(char c) {
  while (!(USART0.STATUS & USART_DREIF_bm)); //Wait for Data Register Empty Interrupt Flag is set
  USART0.TXDATAL = c;
}

ISR(PORTB_PORT_vect) {
  PORTB.INTFLAGS = PIN2_bm;   // clear interrupt flag
  event = true;
}

void setup() {
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
  USART0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc | USART_SBMODE_1BIT_gc | USART_CHSIZE_8BIT_gc;          // Set Frame Format: 8 data bits, 1 stop bit, no parity
  USART0.BAUD = (uint16_t) (F_CPU * 64 / (16 * (float) BAUD_RATE) + 0.5);  // Configure USART Baud Rate
  USART0.CTRLB |= USART_TXEN_bm;                      // enable transmit only
  
  // configure interrupt
  PORTB.DIRCLR = PIN2_bm;
  PORTB.PIN2CTRL = PORT_ISC_BOTHEDGES_gc;       // 1. Configure the pin: Pull-up disabled, sense falling and rising edges
  
  
}

void loop() {
  
  if(event){
    if(PORTB.IN & PIN2_bm){
      sprintf(print_string, "Interlock Closed\r\n");
    }
    else{
      sprintf(print_string, "Interlock Opened\r\n");
    }
  }
  else{
    sprintf(print_string, "...\r\n");
  }
  
  for (size_t i=0; i< strlen(print_string); i++) {
    USART0_sendChar(print_string[i]);
  }
  _delay_ms(500);
  
}







