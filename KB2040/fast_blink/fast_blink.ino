/*
Super simple pulsed light generator

Looks like the clock is 400ns, so lets generate 


*/

void setup() {
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
}

void loop() {
  // 10 us = 24 cycles
  // 100 us = 240 cycles (therabouts)
  
  // 10 us of GPIO2 HIGH, 10 us delay
  for(int i=0; i<24; ++i){digitalWrite(2, HIGH);}
  for(int i=0; i<240; ++i){digitalWrite(2, LOW); }
  
  // 10 us of GPIO3 HIGH, 10 us delay
  for(int i=0; i<24; ++i){digitalWrite(3, HIGH);}
  for(int i=0; i<240; ++i){digitalWrite(3, LOW); }

  // 10 us of GPIO4 HIGH, 10 us delay
  for(int i=0; i<24; ++i){digitalWrite(4, HIGH);}
  for(int i=0; i<240; ++i){digitalWrite(4, LOW); }

  // 10 us of GPIO5 HIGH, 10 us delay
  for(int i=0; i<24; ++i){digitalWrite(5, HIGH);}
  for(int i=0; i<240; ++i){digitalWrite(5, LOW); }
  
  // 100 us delay
  delay(100);
}
