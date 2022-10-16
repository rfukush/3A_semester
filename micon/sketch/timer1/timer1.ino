//example 1 of timer0

void setup_io()
{
	Serial.begin(57600);
}

void initialize_timer()
{
	//タイマーの設定
	TCCR0A = 0;
	TCCR0B = _BV(CS02) | _BV(CS00);  //1024分周比
	TCNT0 = 0; //カウンタ値の初期化
}

void print_status(unsigned char status)
{
	static unsigned char prev_status = 0;
	if (prev_status != status >> 4) {
		Serial.println(status >> 4);
	}
	prev_status = status >> 4;
}

int main(int argc, char* argv[])
{
	unsigned char state = 0;

	setup_io();
	initialize_timer();

	while (1) {
		loop_until_bit_is_set(TIFR0, ...);
		print_status(++state);
		TIFR0 |= _BV(...);
	}

	return 0;
}
