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
#include <util/delay.h>
#include <stdio.h>

// Set baudrate for RS485
//#define BAUD_RATE 115200
#define BAUD_RATE 57600
//#define BAUD_RATE 9600

char print_string[256];
char laser_power_str[10];
char board_temp_str[10];


static void USART0_sendChar(char c) {
  while (!(USART0.STATUS & USART_DREIF_bm)); //Wait for Data Register Empty Interrupt Flag is set
  USART0.TXDATAL = c;
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
  
  // Configure AI (10 bit) on PA4 and PA7 for LSRSENSE and BOARD_TEMP
  PORTA.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;             // Disable digital buffer, also disables the pullup
  PORTA.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;             // Disable digital buffer, also disables the pullup
  ADC0.CTRLA = ADC_ENABLE_bm;                             // Enable ADC
  ADC0.CTRLC = ADC_REFSEL_INTREF_gc | ADC_PRESC_DIV16_gc; // use internal reference / prescaler: 16
  VREF.CTRLA = VREF_ADC0REFSEL_4V34_gc;                   // use internal 4.34 V reference 
  ADC0.SAMPCTRL = 0;                                      // not sure
  
}

void loop() {
  
  // Read PA4
  ADC0.MUXPOS = ADC_MUXPOS_AIN4_gc;             // use PA4/AI4 as input
  ADC0.COMMAND = ADC_STCONV_bm;                 // Start Conversion
  while((ADC0.INTFLAGS & ADC_RESRDY_bm) == 0){} // Waiting for the result
  float laser_power = ADC0.RES * 4340.0 / 1024.0;  // this is mV

  // Read PA7
  ADC0.MUXPOS = ADC_MUXPOS_AIN7_gc;             // use PA7/AI7 as input
  ADC0.COMMAND = ADC_STCONV_bm;                 // Start Conversion
  while((ADC0.INTFLAGS & ADC_RESRDY_bm) == 0){} // Waiting for the result
  float board_temp = ADC0.RES * 4340.0 / 1024.0;  // this is mV
  board_temp = (board_temp - 500.)/10.;           // this is T in C, for MCP9700
  board_temp = board_temp * 9./5. + 32.;          // this is T in F
  
  
  dtostrf(laser_power, 6, 2, laser_power_str); // dtostrf(value, width, precision, output_buffer)
  dtostrf(board_temp,  6, 2, board_temp_str); // dtostrf(value, width, precision, output_buffer)
  sprintf(print_string, "Readings: PA4 %s, PA7 %s,\r\n", laser_power_str, board_temp_str);
  
  for (size_t i=0; i< strlen(print_string); i++) {
    USART0_sendChar(print_string[i]);
  }
  _delay_ms(500);
  
  
}

