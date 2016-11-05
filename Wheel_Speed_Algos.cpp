#include <Arduino.h>

#include "Wheel_Speed_Algos.h"

const int STARBOARD_PIN = 2;
const int PORT_PIN = 3;

volatile uint16_t sboard_curr = 0;
volatile uint16_t port_curr = 0;
volatile uint16_t sboard_old = 0;
volatile uint16_t port_old = 0;

void sboard_click()
{
	sboard_old = sboard_curr;
	sboard_curr = TCNT1;
}

void port_click()
{
	port_old = port_curr;
	port_curr= TCNT1;
}

//interrupt routine for timer2 overflow.
//timer 2 is an 8bit timer with a 1024 scaling
//Will run at 8MHz/256/1024 so ~30Hz
//also am only pretty sure of 8MHz, if it's in fact 16 just some constant changing.
//TIMER2_OVF_vect is triggered every time timer2 overflows
ISR(TIMER2_OVF_vect)
{
	//timer1 is at 125,000 Hz (8 MHz / 64) so one unit is 8*10^-6
	//one minute is 7,500,000 timer units
	//8 MHz / 64 * 60 = MAGIC because that's the number of timer clicks per minute
	#define MAGIC 7500000
	//22 teeth on the wheel, 22 clicks per rotation, 1 semirotation is 1/22 rotation
	#define TEETH 22

	uint16_t local_sboard_curr = sboard_curr;
	uint16_t local_port_curr = port_curr;
	//allow this interrupt to be interrupted
	sei();
	//make it rarer that it requres more logic to handle
	TCNT1 = 0;
	//using longs is safer because we need signed and the ability to interpret > 16 bits
	int32_t sboard_delta = local_sboard_curr - local_sboard_old;
	int32_t port_delta = local_port_curr - local_port_old;
	//make sure that it's the absolute difference regardless of overflows
	//assuming 16bit integers. I'm pretty sure that's what this is.
	sboard_delta = sboard_delta >= 0 ? sboard_delta : (local_sboard_curr - (local_sboard_old - UINT_MAX));
	port_delta = port_delta >= 0 ? port_delta : (local_port_curr - (local_port_old - UINT_MAX));
	//converts clicks into RPM
	//proper math is that one click per tick times MAGIC (7,500,000) / teeth (22) will net RPM.
	uint32_t sboard_rpm = sboard_delta == 0 ? 0 : MAGIC / sboard_delta / TEETH;
	uint32_t port_rpm = port_delta == 0 ? 0 : MAGIC / port_delta / TEETH;

	Serial.print("port rpm:");
	Serial.println(port_rpm);
	Serial.print("starboard rpm:");
	Serial.println(sboard_rpm);

	#undef MAGIC
	#undef TEETH
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

	//Link to datasheet with most of the magic numbers on it
	//http://www.atmel.com/Images/Atmel-42735-8-bit-AVR-Microcontroller-ATmega328-328P_datasheet.pdf
	//enable the timer2 - 8bit timer
	TIMSK = 1<<TOIE2;
	//set the count to 0
	TCNT2 = 0;
	//CA22 is timer 2 prescalar of 1024
	//CS20 enables timer2
	TCCR2 = (1<<CA22) | (1<<CS20);

	//enable timer 1 - 16bit timer
	TIMSK |= 1<<TOIE1;
	//set the count to 0
	TCNT1=0;
	//CS10 starts timer 1
	//CS11 is timer 1 prescalar of 64
	TCCR1 = (1<<CS10) | (1<<CS11);

	//enable interrupts
	sei();
}
