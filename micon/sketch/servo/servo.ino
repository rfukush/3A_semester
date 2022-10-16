#include <avr/io.h>
#include <math.h>

void setup() {
  sei();
  DDRB = _BV(PB1) | _BV(PB2) | _BV(PB3) | _BV(PB4) | _BV(PB5);
  TCCR1A = _BV(COM1A1) | _BV(WGM11);
  TCCR1B = _BV(CS11) | _BV(WGM12) | _BV(WGM13);
  ICR1 = ....;
  TCNT1 = 0;
  ADCSRA = _BV(ADEN) | _BV(ADPS2);
  ADMUX = _BV(REFS0);
}

void loop() {
  int OCR1Amin = 1600;//0.8ms
  int OCR1Amax = 4000;//2.0ms
  int value = 0;
  int sine_curve;
  for(int i=0; i<OCR1Amax-OCR1Amin; i+=5){
    ADCSRA |= _BV(ADSC);
    loop_until_bit_is_set(ADCSRA, ADIF);
    value = ADC;
    OCR1A = (int) ((value/1024.) * (OCR1Amax - OCR1Amin));
    delay(5);
  }
}
