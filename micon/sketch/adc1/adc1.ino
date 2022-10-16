//example 1 of ADC

void setup()
{
   DDRB |= _BV(PB5);
   Serial.begin(57600);
}

float a;
void output(int n)
{
    a = 1000 * (1.0/1023 * n);
    PORTB |= _BV(PB5);
    delayMicroseconds(a);
    PORTB = 0b00000000;
    delayMicroseconds(1000 - a);
}

int main(int argc, char* argv[])
{
  setup();

  ADCSRA = _BV(ADEN) | _BV(ADPS2);//AD変換を有効化、プリスケーラの設定

  ADMUX = _BV(REFS0);//基準電圧の設定

  while (1) {
    ADCSRA |= _BV(...);//AD変換の開始

    loop_until_bit_is_set(ADCSRA,...);//AD完了フラグを監視
    
    output(ADC);
    Serial.println(ADC);

    ADCSRA |= _BV(...);//AD完了割り込みを有効にする（オプション）
  }
}
