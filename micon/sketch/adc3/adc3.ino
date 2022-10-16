//example 3 of ADC

void setup()
{
   DDRB |= _BV(PB5);
}

float a;
int i;
int j;

void output(float n)
{ 
  a=(.../360.0)*n; //欲しい目標角度を入力してください（0~360）
  for(i=0;i<=a;i++){ PORTB |= _BV(PB5);}
  for(j=0;j<=(n-a);j++){ PORTB = 0b00000000;}
  i=0;
  j=0;
}

ISR(ADC_vect)
{
  output(ADC);
}

ISR(TIMER0_OVF_vect){
  TCNT0 = 99;
}

void initialize_timer()
{
  //タイマーの設定
  TCCR0A = 0;
  TCCR0B = _BV(CS02) | _BV(CS00);  //1/1024分周比
  TCNT0 = ...;//カウンタ値の初期化
  TIMSK0 = _BV(TOIE0);//オーバーフロー割り込み許可を設定
}

int main(int argc, char* argv[])
{
  setup();

  initialize_timer();

ADCSRA = _BV(...) | _BV(ADPS2)|_BV(...)| _BV(...);
  //AD変換を許容、プリスケーラの設定、AD変換割り込みの許可、オートトリガモード設定
  ADCSRB = _BV(...);//トリガーソースはタイマー0オーバーフロー割り込み

  ADMUX = _BV(REFS0);//基準電圧の設定
  ADCSRA |= _BV(...);//AD変換の開始

  sei();
  while (1);

}
