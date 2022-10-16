
boolean state = false;

void setup() {  
  DDRB |= _BV(DDB5);
  DDRD &= ~_BV(DDD2);
  PORTD |= _BV(PORTD2);
}

void loop() {
  if ( bit_is_clear(PIND, PIND2) ) {
    state = !state;
  }
  
  if ( state ) {
    PORTB |= _BV(PORTB5);
  }
  else {
    PORTB &= ~_BV(PORTB5);
  }
}
