
void setup() {
  DDRB |= ...;
  DDRD &= ...;
  PORTD |= ...;
}

void loop() {
  if (bit_is_set(PIND, ...)) {
    PORTB |= ...;
  }
  else {
    PORTB &= ...;
  }
}
