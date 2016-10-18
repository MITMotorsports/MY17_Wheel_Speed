#include <Arduino.h>

#include "Wheel_Speed_Algos.h"

const int STARBOARD_PIN = 2;
const int PORT_PIN = 3;
const int RPM_PERIOD = 20;

volatile unsigned int sboard_curr = 0;
volatile unsigned int port_curr = 0;
volatile unsigned int sboard_old = 0;
volatile unsigned int port_old = 0;

void sboard_click()
{
	sboard_old = sboard_curr;
	sboard_curr = TCNT1;
}

void port_click()
{
	port_old = port_old;
	port_curr= TCNT1;
}

//interrupt routine for timer2 overflow.
//timer 2 is an 8bit timer with a 1024 scaling
//Will run at 8MHz/256/1024 so ~30Hz
ISR(TIMER2_OVF_vect)
{
	//allow this interrupt to be interrupted
	sei();
	TCNT1 = 0x0000;
	int sboard_delta = sboard_curr - sboard_old;
	int port_delta = port_curr - port_old;
	//Make sure that it's the difference between with ints and stuff
	sboard_delta = sboard_delta < 0 ? sboard_delta : (sboard_curr - (sboard_old - INT_MAX));
	port_delta = port_delta < 0 ? port_delta : (port_curr - (port_old - 0xFFFF));
	//timer1 overflows with a freq. of 8MHz/65536/1024 so ~ 1.9 seconds
	//timer1 is at 125,000 Hz so one unit is 8*10^-6
	//one minute is 7,500,000 timer units
	int sboard_rpm = 7500000000 / sboard_delta / 22;
	int port_rpm = 75000000 / port_delta / 22;

	Serial.print("port rpm:");
	Serial.println(port_rpm);
	Serial.print("starboard rpm:");
	Serial.println(sboard_rpm);
}

void setup()
{

	Serial.begin(115200);

	pinMode(STARBOARD_PIN, INPUT_PULLUP);
	pinMode(PORT_PIN, INPUT_PULLUP);
	
	//disable interrupts
	cli();

	attachInterrupt(digitalPinToInterrupt(STARBOARD_PIN), sboard_click, RISING);
	attachInterrupt(digitalPinToInterrupt(PORT_PIN), port_click, RISING);

	//enable the timer2 - 8bit timer
	TIMSK = 1<<TOIE2;
	TCNT2 = 0x00;
	//prescalar of 1024
	TCCR2 = (1<<CA22) | (1<<CS20);

	//enable timer 1 - 16bit timer
	TIMSK |= 1<<TOIE1;
	TCNT1=0x0000;
	//prescalar of 64
	TCCR1 = (1<<CS10) | (1<<CS11);

	//enable interrupts
	sei();
}
