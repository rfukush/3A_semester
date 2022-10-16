//example 2 of ADC

void setup()
{
   DDRB |= _BV(PB5);
  ADCSRA = _BV(...) | _BV(ADPS2)|_BV(...)| _BV(...);
  //AD変換を許容、プリスケーラの設定、オートトリガモード設定、AD変換割り込みの許可、
  ADCSRB &= 11111000 ;//トリガーソースなし（フリーランニングモード）

  ADMUX = _BV(REFS0);//基準電圧の設定
  ADCSRA |= _BV(...);//AD変換の開始
}

float a;
void output(float n)
{
    a=(.../360.0)*n*10;//欲しい目標角度を入力してください(0~360)
    PORTB |= _BV(PB5);
    delayMicroseconds(a);
    PORTB = 0b00000000;
    delayMicroseconds(n*10 - a);
}

ISR(ADC_vect)
{
  output(ADC);
}

int main(int argc, char* argv[])
{

  setup();

  sei();
  while (1);

}
