// LPD Board Evaluation
//
// Flash Settings:
//   Board - ATTiny 1604 (not optiboot)
//   Speed - 20 MHz (using 5V)
//   Programmer - SerialUPDI - SLOW: 57600 baud.
//   milli/micro/timer - disabled
//
//
// Carl Hall, carl@LEAero.us
// 20260722

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdint.h>

// Current version, printed to RS485 on power on
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

// Calibration for Laser diode power vs photodiode sense, in mW laser per mV reading
//#define LASER_CAL_MW_MV 1.0   // check
#define LASER_CAL_MW_MV 0.004615   // 6 mW / 1300 mV


uint16_t debug_value = 65535;
char print_string[256];
char laser_power_str[16];
char board_temp_str[16];
char interlock_str[16];

volatile uint16_t particle_count = 0;

// Routines for sending characters through USART
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

// Interrupt routine for periodic updating of outputs
ISR(RTC_PIT_vect) {
  // TODO store and reset the counter, maybe the below function has a parameter of counts?
  update_outputs();               // Update the outputs
  RTC.PITINTFLAGS = RTC_PI_bm;    // Clear the Periodic Interrupt flag
}

// Interrupt routine for laser interlock
ISR(PORTB_PORT_vect){
  PORTA.OUTCLR = PIN6_bm;     // turn laser off
  PORTB.OUTSET = PIN3_bm;     // green status LED
  PORTB.INTFLAGS = PIN2_bm;   // clear interrupt
}


void setup(){
  // Configure PB3 for Status LED, set to high impedance for now to get both LEDs dimly lit
  PORTB.DIRCLR = PIN3_bm;

  // Configure PB0/PB1 for AO/DO Select
  PORTB.DIRSET = PIN0_bm;     // set PB0 to OUTPUT
  PORTB.OUTSET = PIN0_bm;     // disable analog output
  PORTB.DIRSET = PIN1_bm;     // set PB1 to OUTPUT
  PORTB.OUTCLR = PIN1_bm;     // disable RS485

  // Configure PA1/PA2/PA3 for USART for SPI/RS485, initialize for RS485 for printing debug info
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, !CLKCTRL_PEN_bm);     // clear Prescalar to run at 20 MHz (needed?)
  while (!(CLKCTRL.MCLKSTATUS & CLKCTRL_OSC20MS_bm)) {};
  PORTA.DIRSET = PIN1_bm;   // Configure TXD/MOSI as output
  PORTA.DIRCLR = PIN2_bm;   // Configure RXD/MISO as input (not currently used)
  PORTA.DIRSET = PIN3_bm;   // Configure CLK/SCK as output
  PORTMUX.CTRLB |= PORTMUX_USART0_bm;     // Select the alternate pin positions for USART0
  USART0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc | USART_SBMODE_1BIT_gc | USART_CHSIZE_8BIT_gc;   // Set Frame Format for RS485: 8 data bits, 1 stop bit, no parity
  USART0.BAUD = (uint16_t) (F_CPU * 64 / (16 * (float) BAUD_RATE) + 0.5);  // Configure USART Baud Rate
  USART0.CTRLB = USART_TXEN_bm; // start transmission
  
  // Configure PA4/PA7 as AI for Laser Power and Board Temperature
  PORTA.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;             // PA4 - Disable digital buffer, also disables the pullup
  PORTA.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;             // PA7 - Disable digital buffer, also disables the pullup
  ADC0.CTRLA = ADC_ENABLE_bm;                             // Enable ADC
  ADC0.CTRLC = ADC_REFSEL_INTREF_gc | ADC_PRESC_DIV16_gc; // use internal reference / clock prescaler: 16 (for slow accurate reading)
  VREF.CTRLA = VREF_ADC0REFSEL_4V34_gc;                   // use internal 4.34 V reference 
  ADC0.SAMPCTRL = 0;                                      // not sure
  
  // Configure PA6 for Laser Control
  PORTA.DIRSET = PIN6_bm;     // set PA6 to OUTPUT
  PORTA.OUTCLR = PIN6_bm;     // turn laser off

  // Print out to RS485 some debug info
  PORTB.OUTSET = PIN1_bm;     // enable RS485
  _delay_ms(100);
  USART_sendString("Laser Particle Detector\r\n");
  USART_sendString(HARDWARE_VERSION);
  USART_sendString(FIRMWARE_VERSION);
  _delay_ms(1);                             // without this the \r\n is cut off for some reason
  PORTB.OUTCLR = PIN1_bm;     // disable RS485

  // Turn status light to Green
  PORTB.DIRSET = PIN3_bm;     // set PB3 to OUTPUT for Status LED
  PORTB.OUTSET = PIN3_bm;     // Green

  // Wait a bit, then start the interrupt routines
  _delay_ms(1000);

  // Configure PB2 for Interlock
  PORTB.DIRCLR = PIN2_bm;                 // set PB2 to INPUT (redundant)
  PORTB.PIN2CTRL = PORT_ISC_FALLING_gc;   // Internal Pull-Up disabled / Interrupt on falling edge (interlock broken)

  // Configure PA5 for Counter
  // TODO  ....
  PORTA.DIRCLR   = PIN5_bm;                       // PA5 as input 
  PORTA.PIN5CTRL = PORT_PULLUPEN_bm;              // Pullup enable (we don't need this i think)
  EVSYS.SYNCCH0 = EVSYS_SYNCCH0_PORTA_PIN5_gc;    // PA5 is syncron. event generator
  EVSYS.SYNCUSER0 = EVSYS_SYNCUSER0_SYNCCH0_gc;   // TCA0 is syncron. event user
  TCA0.SINGLE.CTRLD = 0;                          // set single mode
  TCA0.SINGLE.CTRLA = 0;                          // switch off the timer, set clock divider to DIV1 
  TCA0.SINGLE.PER = 0xffff;                       // TCA0 counter limit
  TCA0.SINGLE.CNT = 0;                            // Set TCA0 counter to 0
  TCA0.SINGLE.EVCTRL = TCA_SINGLE_EVACT_POSEDGE_gc | TCA_SINGLE_CNTEI_bm; // count pos. edge of events (RISING) & enable event count
  TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;  // normal mode => TOP = PER
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;         // clear the overflow interrupt flag
  TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm;         // turn it on

  // Setup polling routine to update outputs
  RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;                // set internal clock @ 32768 Hz
  RTC.PITINTCTRL = RTC_PI_bm;                       // enable PIT interrupt
  RTC.PITCTRLA = OUTPUT_RATE | RTC_PITEN_bm;        // Start periodic interrupt timer

}

void update_outputs(){
  
  ///////////////////////////////////////////////////////////////////////////////////
  // Check if laser can be turned on (interlock only powers off the laser)
  if(PORTB.IN & PIN2_bm){
    sprintf(interlock_str, "Running");
    PORTA.OUTSET = PIN6_bm;  // turn laser on
    PORTB.OUTCLR = PIN3_bm;  // red status LED
  }
  else{
    sprintf(interlock_str, "Not Running");
  }
  
  ///////////////////////////////////////////////////////////////////////////////////
  // Pull current counter value and reset
  particle_count = TCA0.SINGLE.CNT;
  TCA0.SINGLE.CNT = 0;

  ///////////////////////////////////////////////////////////////////////////////////
  // Analog/Digital Inputs
  
  // read value of laser power
  ADC0.MUXPOS = ADC_MUXPOS_AIN4_gc;                 // use PA4/AI4 as input
  ADC0.COMMAND = ADC_STCONV_bm;                     // Start Conversion
  while((ADC0.INTFLAGS & ADC_RESRDY_bm) == 0){}     // Waiting for the result
  float laser_power = ADC0.RES * 4340.0 / 1024.0;   // this is mV
  laser_power *= LASER_CAL_MW_MV;                   // mW
  dtostrf(laser_power, 6, 2, laser_power_str);      // dtostrf(value, width, precision, output_buffer)
  
  // read value of board temperature
  ADC0.MUXPOS = ADC_MUXPOS_AIN7_gc;               // use PA7/AI7 as input
  ADC0.COMMAND = ADC_STCONV_bm;                   // Start Conversion
  while((ADC0.INTFLAGS & ADC_RESRDY_bm) == 0){}   // Waiting for the result
  float board_temp = ADC0.RES * 4340.0 / 1024.0;  // this is mV
  //board_temp = (board_temp - 500.)/10. +   0.;    // this is T in C, for MCP9700, cal from factor spec sheet (500 mV at 0 C)
  board_temp = (board_temp - 780.)/10. +  22.;    // this is T in C, calibrated in office (780 mV at 22 C)
  //board_temp = board_temp * 9./5. + 32.;          // this is T in F
  dtostrf(board_temp,  6, 2, board_temp_str);     // dtostrf(value, width, precision, output_buffer)


  ///////////////////////////////////////////////////////////////////////////////////
  // Analog Outputs

  // Setup for SPI communication to analog digitizer
  USART0.CTRLC = USART_CMODE_MSPI_gc; 
  USART0.BAUD = 128;                            // baudrate is 1/32 of perif clock (this is hardset along with manual delays, so don't change independently)
  
  // Update Analog Output Voltage (VOUTB)
  PORTB.OUTCLR = PIN0_bm;                       // Configure analog output digitizer to listen
  USART_sendChar(0b00100100);                  // Update DAC output immediately
  USART_sendChar((debug_value >> 8) & 0xFF);    // upper 8 bits
  USART_sendChar(debug_value & 0xFF);           // lower 8 bits

  while (!(USART0.STATUS & USART_DREIF_bm));    // wait for transmission to complete
  _delay_us(10);                                // not sure why this is needed
  PORTB.OUTSET = PIN0_bm;                       // configure digitizer to stop listening
  
  _delay_us(10);                                // not sure why this is needed
  
  // Update Analog Output Current (VOUTA)
  PORTB.OUTCLR = PIN0_bm;                       // configure analog output digitizer to listen
  USART_sendChar(0b00010000);                   // Update DAC output immediately
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
  
  sprintf(print_string, "LPD SN01, %u Particles, %s mW, %s C, 25%% Laser Cooling, %s, %u\r\n", particle_count, laser_power_str, board_temp_str, interlock_str, debug_value);
  USART_sendString(print_string);
  _delay_ms(1);                             // without this the \r\n is cut off for some reason
  PORTB.OUTCLR = PIN1_bm;                 // Disable RS485
  
  ///////////////////////////////////////////////////////////////////////////////////

  // update value of debug level variable
  if (debug_value>16000){ debug_value = 0;    }
  else{                   debug_value = 65535;}

}


void loop(){}
