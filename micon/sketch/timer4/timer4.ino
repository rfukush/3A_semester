//example 4 of timer0

void setup_io()
{
	Serial.begin(57600);
}

void initialize_timer()
{
	//タイマーの設定
	TCCR0A = _BV(...);//ctc動作を行わせる
	... = ....;//TCNT0のTOP値をとあるレジスタに代入
	TCCR0B = _BV(CS02) | _BV(CS00);  //分周比1024
	TCNT0 = 0;//カウンタ値の初期化
	TIMSK0 = _BV(....);//比較一致割り込み許可を設定
}

void print_status(unsigned char status)
{
	static unsigned char prev_status = 0;
	if (status >> 4 != prev_status) {
		Serial.println(status >> 4);
	}
	prev_status = status >> 4;
}

volatile unsigned char state;

//Timre0のOCR0Aとの比較一致割り込みで行う関数
ISR(TIMER0_COMPA_vect)
{
	state++;
	print_status(state);
}

int main(int argc, char* argv[])
{
	
	setup_io();
	initialize_timer();

	state = 0;

	sei();//全割り込みの許可を行う

	while (1);

	return 0;
}
