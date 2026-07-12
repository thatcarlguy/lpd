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

  // Configure SPI
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, !CLKCTRL_PEN_bm);     // clear Prescalar to run at 20 MHz (needed?)
  while (!(CLKCTRL.MCLKSTATUS & CLKCTRL_OSC20MS_bm)) {};
  
  PORTA.DIRSET = PIN1_bm;   // Configure MOSI as output
  PORTA.DIRCLR = PIN2_bm;   // Configure MISO as input (not currently used)
  PORTA.DIRSET = PIN3_bm;   // Configure SCK as output

  // Initialize SPI0 in Master mode, set prescalar
  SPI0.CTRLA = SPI_MASTER_bm | SPI_PRESC_DIV16_gc;
  
  // Disable use of Slave Select
  SPI0.CTRLB |= SPI_SSD_bm;

  // 
  
  //Enable
  SPI0.CTRLA |= SPI_ENABLE_bm;
  
}

void loop(){
  
  PORTB.OUTCLR = PIN3_bm;     // Red
  
  // Buffer A is the 0-5 volts output
  //  0b00010000  write to A buffer, and update A DAC
  //  0b11111111  full value
  //  0b11111111  full value
  PORTB.OUTCLR = PIN0_bm;     // configure digitizer to listen
  SPI0.DATA = 0b00010000;
  while (!(SPI0.INTFLAGS & SPI_IF_bm));    // Wait for the Transfer Complete Interrupt Flag (SPI_IF_bm)
  SPI0.INTFLAGS = SPI_TXCIF_bm;             // Clear the transmit complete flag (write 1 to clear)

  SPI0.DATA = 0b11111111;
  while (!(SPI0.INTFLAGS & SPI_IF_bm));    // Wait for the Transfer Complete Interrupt Flag (SPI_IF_bm)
  SPI0.INTFLAGS = SPI_TXCIF_bm;             // Clear the transmit complete flag (write 1 to clear)
  
  SPI0.DATA = 0b11111111;
  while (!(SPI0.INTFLAGS & SPI_IF_bm));    // Wait for the Transfer Complete Interrupt Flag (SPI_IF_bm)
  SPI0.INTFLAGS = SPI_TXCIF_bm;             // Clear the transmit complete flag (write 1 to clear)
  PORTB.OUTSET = PIN0_bm;     // configure digitizer to stop listening
  
  _delay_ms(500);
  
  PORTB.OUTSET = PIN3_bm;     // Green

  // Buffer A is the 0-5 volts output
  //  0b00010000  write to A buffer, and update A DAC
  //  0b00000000  zero value
  //  0b00000000  zero value
  PORTB.OUTCLR = PIN0_bm;     // configure digitizer to listen
  SPI0.DATA = 0b00010000;
  while (!(SPI0.INTFLAGS & SPI_IF_bm));    // Wait for the Transfer Complete Interrupt Flag (SPI_IF_bm)
  SPI0.INTFLAGS = SPI_TXCIF_bm;             // Clear the transmit complete flag (write 1 to clear)

  SPI0.DATA = 0b00000000;
  while (!(SPI0.INTFLAGS & SPI_IF_bm));    // Wait for the Transfer Complete Interrupt Flag (SPI_IF_bm)
  SPI0.INTFLAGS = SPI_TXCIF_bm;             // Clear the transmit complete flag (write 1 to clear)
  
  SPI0.DATA = 0b00000000;
  while (!(SPI0.INTFLAGS & SPI_IF_bm));    // Wait for the Transfer Complete Interrupt Flag (SPI_IF_bm)
  SPI0.INTFLAGS = SPI_TXCIF_bm;             // Clear the transmit complete flag (write 1 to clear)
  PORTB.OUTSET = PIN0_bm;     // configure digitizer to stop listening
  
  _delay_ms(500);

}



