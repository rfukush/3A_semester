void setup() {
  DDRB = ...;
}

void loop() {
  PORTB |= ...;
  delay(500.0);
  PORTB &= ...;
  delay(100.0);
}
