#include "stc15.h"
#include "typedef.h"
#include "timer.h"
#include "one_os.h"

void pwm_task()
{
#define  pwm		P32
	
	u16 period = 10; //100hz
	u16 duty = 7;
	static u16 counter = 0;
	
	counter++;
	counter %= period;
	
	if (counter < duty) {
		pwm = 1;
	}
	else
		pwm = 0;
}