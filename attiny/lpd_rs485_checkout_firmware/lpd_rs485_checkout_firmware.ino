// LPD Board Evaluation
//
// Flash Settings:
//   Board - ATTiny 1604 (not optiboot)
//   Speed - 20 MHz (using 5V)
//   Programmer - SerialUPDI - SLOW: 57600 baud.
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
#define HARDWARE_VERSION "Hardware Ver: 0.0.0, 20260831\r\n"
#define FIRMWARE_VERSION "Firmware Ver: 0.0.0, 20260831\r\n"

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

uint16_t debug_value = 0;
char print_string[256];
char laser_power_str[16];
char board_temp_str[16];
char interlock_str[16];

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

// ISR for the interlock, that updates the power pin and status light

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
  
  // Configure PA4/PA7 for Laser Power and Board Temperature
  PORTA.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;             // PA4 - Disable digital buffer, also disables the pullup
  PORTA.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;             // PA7 - Disable digital buffer, also disables the pullup
  ADC0.CTRLA = ADC_ENABLE_bm;                             // Enable ADC
  ADC0.CTRLC = ADC_REFSEL_INTREF_gc | ADC_PRESC_DIV16_gc; // use internal reference / prescaler: 16
  VREF.CTRLA = VREF_ADC0REFSEL_4V34_gc;                   // use internal 4.34 V reference 
  ADC0.SAMPCTRL = 0;                                      // not sure

  // Configure PB2 for Interlock
  PORTB.DIRCLR = PIN2_bm;
  // do everything needed for the interrupt, but here update the LED based on the state of the interlock pin
  if(PORTB.IN & PIN2_bm){     // if interlock is high (connected)
    PORTB.OUTCLR = PIN3_bm;   // Red
  }
  else{
    PORTB.OUTSET = PIN3_bm;   // Green
  }
  
  // Configure PA6 for Laser Control, bypassed for now (since interlock isn't in yet)
  PORTA.DIRSET = PIN6_bm;     // set PA6 to OUTPUT
  PORTA.OUTCLR = PIN6_bm;     // keep laser off for now

  // Configure PA5 for Counter
  //   ....

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

  // Wait a bit, then start the polling routine which will turn on the laser
  _delay_ms(1000);

  // Setup polling routine to update outputs
  RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;                // set internal clock @ 32768 Hz
  RTC.PITINTCTRL = RTC_PI_bm;                       // enable PIT interrupt
  RTC.PITCTRLA = OUTPUT_RATE | RTC_PITEN_bm;        // Start periodic interrupt timer

}

void update_outputs(){
  // Flash the red light during the update routine
  PORTB.OUTCLR = PIN3_bm;     // red

  // fake update the outputs
  if(debug_value== 0){sprintf(print_string, "LPD SN01, 0 Particles, 9.98 mW, 25.0 C, Running\r\n");}
  if(debug_value== 1){sprintf(print_string, "LPD SN01, 10 Particles, 9.97 mW, 26.0 C, Running\r\n");}
  if(debug_value== 2){sprintf(print_string, "LPD SN01, 20 Particles, 9.96 mW, 25.0 C, Running\r\n");}
  if(debug_value== 3){sprintf(print_string, "LPD SN01, 30 Particles, 9.97 mW, 26.0 C, Running\r\n");}
  if(debug_value== 4){sprintf(print_string, "LPD SN01, 40 Particles, 9.98 mW, 25.0 C, Running\r\n");}
  if(debug_value== 5){sprintf(print_string, "LPD SN01, 50 Particles, 9.97 mW, 26.0 C, Running\r\n");}
  if(debug_value== 6){sprintf(print_string, "LPD SN01, 60 Particles, 9.96 mW, 25.0 C, Running\r\n");}
  if(debug_value== 7){sprintf(print_string, "LPD SN01, 70 Particles, 9.97 mW, 26.0 C, Running\r\n");}
  if(debug_value== 8){sprintf(print_string, "LPD SN01, 80 Particles, 9.98 mW, 25.0 C, Running\r\n");}
  if(debug_value== 9){sprintf(print_string, "LPD SN01, 90 Particles, 9.97 mW, 26.0 C, Running\r\n");}
  
  // Setup for RS485 communication
  USART0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc | USART_SBMODE_1BIT_gc | USART_CHSIZE_8BIT_gc;          // Set Frame Format for RS485: 8 data bits, 1 stop bit, no parity
  USART0.BAUD = (uint16_t) (F_CPU * 64 / (16 * (float) BAUD_RATE) + 0.5);  // Configure USART Baud Rate
  PORTB.OUTSET = PIN1_bm;                 // Enable RS485 chip
  
  USART_sendString(print_string);
  _delay_ms(1);                             // without this the \r\n is cut off for some reason
  PORTB.OUTCLR = PIN1_bm;                 // Disable RS485
  
  ///////////////////////////////////////////////////////////////////////////////////

  // update value of debug level variable
  debug_value++;
  if (debug_value>9){ debug_value = 0;    }

  PORTB.OUTSET = PIN3_bm;     // green
}


void loop(){}
