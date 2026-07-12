// LPD Control Program
//
// TODO: 1. check pin swaps
//       2. 
//       3. 
//
// Carl Hall, carl@LEAero.us
// 20260706
//

void setup() {
  // Configure Counter (PA5)
  PORTA.DIRCLR = PIN5_bm;   // Configure PA5 as input
  EVSYS.CHANNEL0 = EVSYS_GENERATOR_PORTA_PIN5_gc;  // Route the External Pin (PA4) to Event Channel 0
  EVSYS.USER0 = EVSYS_CHANNEL_CHANNEL0_gc;      // Set Event User 0 (TCB0) to listen to Event Channel 0
  TCB0.CCMP = TARGET_COUNT;    // Set the compare limit to our target event count
  TCB0.CTRLB = TCB_CNTMODE_FRQ_gc; // Mode for Event counting / input capture
  TCB0.EVCTRL = TCB_EDGE_bm | TCB_CAPTEI_bm; // Count positive edges from event
  TCB0.INTCTRL = TCB_CAPT_bm; // Enable Capture/Compare Interrupt
  TCB0.CTRLA = TCB_ENABLE_bm; // Start TCB0

  // Configure Analog Inputs (PA4, PA7)
  ADC0.CTRLA = ADC_ENABLE_bm; // Enable ADC
  ADC0.CTRLC = ADC_REFSEL_INTREF_gc | ADC_PRESC_DIV16_gc; // use internal reference / prescaler: 16
  VREF.CTRLA = VREF_ADC0REFSEL_4V34_gc; // use internal 4.34 V reference 
  ADC0.SAMPCTRL = 0;
  PORTA.PIN4CTRL = (~PORT_PULLUPEN_bm) | PORT_ISC_INPUT_DISABLE_gc; // Disable pullup and digital buffer for PA4/AIN4 for Laser PD power feedback sensing
  PORTA.PIN7CTRL = (~PORT_PULLUPEN_bm) | PORT_ISC_INPUT_DISABLE_gc; // Disable pullup and digital buffer for PA7/AIN7 for Board Temperature

  // Configure Ditigal Inputs (PB2)
  PORTB.DIRCLR = PIN2_bm;     // set PB2 to INPUT for Interlock (TODO move this to PB2)

  // Configure Digital Outputs (PA6, PB0, PB1, PB3)
  PORTA.DIRSET = PIN6_bm;     // set PA6 to OUTPUT for Laser Control
  PORTB.DIRSET = PIN3_bm;     // set PB3 to OUTPUT for Status LED
  PORTB.DIRSET = PIN1_bm;     // set PB1 to OUTPUT for DO Select
  PORTB.DIRSET = PIN0_bm;     // set PB0 to OUTPUT for AO select

  // Configure SPI (PA1-3)
  PORTA.DIRSET = PIN1_bm;     // set PA1/MOSI to OUTPUT
  PORTA.DIRCLR = PIN2_bm;     // set PA2/MOSI to INPUT
  PORTA.DIRSET = PIN3_bm;     // set PA3/MOSI to OUTPUT
  SPI0.CTRLA |= SPI_MASTER_bm; // set Master operation
  SPI0.CTRLA |= SPI_PRESC_DIV16_gc; // set clock speed
  SPI0.CTRLB |= SPI_SSD_bm;    // disable use of !SS (PA6)
  SPI0.CTRLA |= SPI_ENABLE_bm; // enable SPI 

}

void loop() {
  // put your main code here, to run repeatedly:

}
*/

/* OLD Pinout:  2 - PA4 - Counter    - TCA0/W04
            3 - PA5 - Laser PD   - ADC0/AIN5
            4 - PA6 - Laser Ctrl - PORTA6 I/O Configuration output
            5 - PA7 - Interlock  - PORTA7 I/O Configuration input
            6 - PB3 - Status LED - PORTB3 I/O Configuration output
            7 - PB2 - Board Temp - PORTB2 I/O Configuration input
            8 - PB1 - DO Select  - PORTB1 I/O Configuration output
            9 - PB0 - AO Select  - PORTB0 I/O Configuration output
           11 - PA1 - MOSI       - SPI0/Output
           12 - PA2 - MISO       - SPI0/Input
           13 - PA3 - SCLK       - SPI0/Output

*/

// https://wolles-elektronikkiste.de/en/programming-the-tinyavr-series-0-1-2-part-1


  

// take data on analog inputs
ADC0.MUXPOS = ADC_MUXPOS_AIN5_gc; // use AIN5 as input
ADC0.COMMAND = ADC_STCONV_bm; // Start Conversion
while((ADC0.INTFLAGS & ADC_RESRDY_bm) == 0){} // Waiting for the result
float result = ADC0.RES * 4340.0 / 1024.0;


// counter stuff
uint16_t current_event_count = TCB0.CNT;

TCB0.SINGLE.CTRLA |= ~(TCA_SINGLE_ENABLE_bm); // switch off the timer
TCB0.SINGLE.CNT   = 0;                        // Set TCA0 counter to 0
TCB0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm;     // turn it back on





// another counter example
volatile bool tca0Ovf = false;  // Flag for TCA OVF
volatile bool cmp0Match = false;  // Flag for CMP0 Match
ISR(TCA0_OVF_vect){  // TCA0 Overflow ISR
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm; /* Clear the interrupt flag */
    tca0Ovf = true;
}
ISR(TCA0_CMP0_vect){  // TCA0/CMP0 compare match ISR
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP0_bm; /* Clear the interrupt flag */
    cmp0Match = true;
}
void setup() {
    Serial.begin(115200);
    PORTB.DIRCLR = PIN1_bm; // PB1 as input 
    PORTB.PIN1CTRL = PORT_PULLUPEN_bm; // Pullup enable
    EVSYS.SYNCCH1 = EVSYS_SYNCCH1_PORTB_PIN1_gc; // PB1 is sync. event generator
    EVSYS.SYNCUSER0 = EVSYS_SYNCUSER0_SYNCCH1_gc;  // TCA0 is sync. event user
    TCA0.SINGLE.CTRLD = 0; // set single mode
    TCA0.SINGLE.CTRLA = 0;  // switch off the timer, set clock divider to DIV1 
    TCA0.SINGLE.PER = 19;  // TCA0 counter limit
    TCA0.SINGLE.CMP0 = 9;  // Compare match value
    TCA0.SINGLE.CNT = 0;  // Set TCA0 counter to 0
    TCA0.SINGLE.EVCTRL = TCA_SINGLE_EVACT_POSEDGE_gc | TCA_SINGLE_CNTEI_bm; // count pos. edge of events (RISING) & enable event count
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc; // normal mode => TOP = PER
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm; // clear the overflow interrupt flag
    TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP0_bm | TCA_SINGLE_OVF_bm; // enable CMP0 and OVF interrupts 
    TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm;
}
void loop(){
    if(cmp0Match){
        Serial.println("Key has been pressed 10 times");
        cmp0Match = false;
    }
    
    if(tca0Ovf){
        Serial.println("Key has been pressed another 10 times");
        Serial.println("Timer/Counter A restarts");
        tca0Ovf = false;
    }
    // Serial.println(TCA0.SINGLE.CNT);  // uncomment to see the TCA0 Counter
    //delay(100); 
}




// SPI
// 1. Pull Chip Select LOW to initiate communication
    PORTA.OUTCLR = PIN4_bm;

    // 2. Write data to the DATA register
    SPI0.DATA = data;

    // 3. Wait for the transmit complete flag (TXCIF)
    while (!(SPI0.INTFLAGS & SPI_TXCIF_bm));

    // 4. Clear the transmit complete flag (write 1 to clear)
    SPI0.INTFLAGS = SPI_TXCIF_bm;

    // 5. Pull Chip Select HIGH to end communication
    PORTA.OUTSET = PIN4_bm;

also

void sendSPIString(String message) {
  // Get pointer to the character array
  const char* text = message.c_str(); 
  
  // Begin transaction with target device settings (e.g., 4MHz, MSBFIRST, MODE0)
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  
  // Select the slave device
  digitalWrite(CS_PIN, LOW); 
  
  // Loop through and transmit until the null terminator '\0' is reached
  for (int i = 0; text[i] != '\0'; i++) {
    SPI.transfer(text[i]); 
  }
  
  // Optional: Send a newline or null character if your receiver expects a packet terminator
  SPI.transfer('\n'); 

  // Deselect the slave device and release the bus
  digitalWrite(CS_PIN, HIGH); 
  SPI.endTransaction();
}






// Hardware External Event Counter Example for ATtiny85
// Hardware input: PB2 (T0 clock source pin / Physical Pin 7)
// Hardware output: PB0 (Physical Pin 5)

#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint32_t totalOverflows = 0;

void init_hardware_counter() {
  // 1. Set PB0 as output (to drive an LED or signal)
  DDRB |= (1 << PB0);
  
  // 2. Configure PB2 (T0) as an input
  DDRB &= ~(1 << PB2);
  PORTB |= (1 << PB2); // Enable pull-up on PB2 if reading mechanical switches

  // 3. Set Timer0 to Normal Mode (counts 0 to 255)
  TCCR0A = 0x00; 

  // 4. Set Clock Source bits to count external pulses on T0 pin
  // CS02=1, CS01=1, CS00=0 sets clock source to T0 pin, falling edge.
  // (Change to CS00=1 for rising edge)
  TCCR0B = (1 << CS02) | (1 << CS01) | (0 << CS00);

  // 5. Enable Timer0 Overflow Interrupt to catch counts past 255
  TIMSK |= (1 << TOIE0);
  
  // Reset hardware counter register
  TCNT0 = 0;
  
  // Enable Global Interrupts
  sei();
}

// ISR triggered automatically every time TCNT0 overflows past 255
ISR(TIMER0_OVF_vect) {
  totalOverflows++; 
}

int main(void) {
  init_hardware_counter();

  while (1) {
    // Read the instantaneous counter value straight from the register
    uint8_t currentPulses = TCNT0;
    
    // Calculate precise absolute count: (Overflows * 256) + current register value
    uint32_t absoluteCount = (totalOverflows * 256) + currentPulses;

    // Example Action: Turn on the PB0 LED when absolute count reaches 1000
    if (absoluteCount >= 1000) {
      PORTB |= (1 << PB0); 
    } else {
      PORTB &= ~(1 << PB0);
    }
  }
  return 0;
}