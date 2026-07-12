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
#include <util/delay.h>   // provides _delay_us() and _delay_ms()
#include <stdio.h>


void setup(){
  // Configure Status LED
  PORTB.DIRSET = PIN3_bm;

  // Configure AO/DO Select
  PORTB.DIRSET = PIN0_bm;     // set PB0 to OUTPUT
  PORTB.OUTSET = PIN0_bm;     // disable analog output
  PORTB.DIRSET = PIN1_bm;     // set PB1 to OUTPUT
  PORTB.OUTCLR = PIN1_bm;     // disable RS485

  // Configure USART
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, !CLKCTRL_PEN_bm);     // clear Prescalar to run at 20 MHz (needed?)
  while (!(CLKCTRL.MCLKSTATUS & CLKCTRL_OSC20MS_bm)) {};
  
  PORTA.DIRSET = PIN1_bm;   // Configure MOSI as output
  PORTA.DIRCLR = PIN2_bm;   // Configure MISO as input (not currently used)
  PORTA.DIRSET = PIN3_bm;   // Configure SCK as output

  PORTMUX.CTRLB |= PORTMUX_USART0_bm;     // Select the alternate pin positions for USART0
  
  // Initialize SPI0 in Master mode, set prescalar
  USART0.CTRLC = USART_CMODE_MSPI_gc | USART_CHSIZE_8BIT_gc;
  USART0.BAUD = 128;       // baudrate is 1/32 of perif clock (1/16 is limit)

  
  //Enable
  USART0.CTRLB = USART_TXEN_bm; // | USART_RXEN_bm;
}


void loop(){
  
  PORTB.OUTCLR = PIN3_bm;     // Red
  
  // Buffer A is the 0-5 volts output
  //  0b00010000  write to A buffer, and update A DAC
  //  0b11111111  full value
  //  0b11111111  full value
  PORTB.OUTCLR = PIN0_bm;     // configure digitizer to listen
  while (!(USART0.STATUS & USART_DREIF_bm));
  USART0.TXDATAL = 0b00010000;
  while (!(USART0.STATUS & USART_DREIF_bm));
  USART0.TXDATAL = 0b11111111;
  while (!(USART0.STATUS & USART_DREIF_bm));
  USART0.TXDATAL = 0b11111111;
  while (!(USART0.STATUS & USART_DREIF_bm));

  _delay_us(10);              // not sure why this is needed
  PORTB.OUTSET = PIN0_bm;     // configure digitizer to stop listening
  
  _delay_ms(500);
  
  PORTB.OUTSET = PIN3_bm;     // Green

  // Buffer A is the 0-5 volts output
  //  0b00010000  write to A buffer, and update A DAC
  //  0b00000000  zero value
  //  0b00000000  zero value
  PORTB.OUTCLR = PIN0_bm;     // configure digitizer to listen
  while (!(USART0.STATUS & USART_DREIF_bm));
  USART0.TXDATAL = 0b00010000;
  while (!(USART0.STATUS & USART_DREIF_bm));
  USART0.TXDATAL = 0b00000000;
  while (!(USART0.STATUS & USART_DREIF_bm));
  USART0.TXDATAL = 0b00000000;
  while (!(USART0.STATUS & USART_DREIF_bm));

  _delay_us(10);              // not sure why this is needed
  PORTB.OUTSET = PIN0_bm;     // configure digitizer to stop listening
  
  _delay_ms(500);

}



