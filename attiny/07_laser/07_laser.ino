// LPD Board Evaluation
//
//  Board - ATTiny 1604 (not optiboot)
//  Speed - 20 MHz (using 5V)
//  Programmer - SerialUPDI - SLOW: 57600 baud.
//
// Turn on the laser and wait
//
//   PA6 - DOUT        Digital output for laser power control
//
// Carl Hall, carl@LEAero.us
// 20260722

void setup() {
  PORTB.DIRSET = PIN3_bm;     // set PB3 to OUTPUT for Status LED
  PORTB.OUTCLR = PIN3_bm;     // Red
  //PORTB.OUTSET = PIN3_bm;     // Green
  

  PORTA.DIRSET = PIN6_bm;     // laser control pin
  PORTA.OUTSET = PIN6_bm;     // high for laser on
  //PORTA.OUTCLR = PIN6_bm;     // low for laser off

}

void loop() {
}
