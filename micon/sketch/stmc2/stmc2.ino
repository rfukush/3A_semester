
boolean x = false;
boolean x_old = false;
boolean state = false;

void setup() {  
  DDRB |= _BV(DDB5);
  DDRD &= ~_BV(DDD2);
  PORTD |= _BV(PORTD2);
}

void loop() {
  x = bit_is_clear(PIND, PIND2);
  
  if( x && !x_old ) {
    state = !state;
  }

  x_old = x;
  
  if ( state ) {
    PORTB |= _BV(PORTB5);
  } else {
    PORTB &= ~_BV(PORTB5);
  }
}
