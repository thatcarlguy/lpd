// LPD Board Evaluation
//
//  Board - ATTiny 1604 (not optiboot)
//  Speed - 20 MHz (using 5V)
//  Programmer - SerialUPDI - SLOW: 57600 baud.
//
// https://learn.adafruit.com/adafruit-updi-friend/advanced-reprogramming-with-updi
// https://wolles-elektronikkiste.de/en/programming-the-tinyavr-series-0-1-2-part-1
//
// Blink the status light
//
//   PB3 - DOUT        Digital output for status LED
//
// Carl Hall, carl@LEAero.us
// 20260706

void setup() {
  // configure SELECT and DEBUG
  PORTB.DIRSET = PIN3_bm;     // set PB3 to OUTPUT for Status LED
}

void loop() {
  // sleep for a second, flashing the light
  PORTB.OUTCLR = PIN3_bm;   // Red
  delay(500);
  
  PORTB.OUTSET = PIN3_bm;   // Green
  delay(500);

  PORTB.DIRCLR = PIN3_bm;   // set to high impediance, doesn't really show well though (dim both lights)
  delay(500);
  PORTB.DIRSET = PIN3_bm;   // set PB3 to OUTPUT for Status LED

}

/*
void setup() {
  pinMode(5, OUTPUT);
}

void loop() {
  digitalWrite(5, HIGH);
  delay(1000);
  digitalWrite(5, LOW);
  delay(1000);
}
*/