#include "stc15.h"
#include "typedef.h"
#include "timer.h"
#include "one_os.h"
#include "config.h"
#include "uart.h"
#include "delay.h"
//#include <stdio.h>
//P30 P31 P32 P33 P34 P35
//其中P30是rx，P31是tx

void pla_init()
{
	timer0_init(1000);
	uart_init();
	EA = 1; // 打开总中断
}

void delay_test()
{
	while(1) {
		P32 = 0;
		delay_us(10);
		P32 = 1;
		delay_us(10);
	//	uart_automic_read();
	}
}
void main()
{
	char c = 0;
	pla_init();
	//delay_test();
	for(;;) {
		//TaskProcess();
		c = getchar();
		delay_ms(5);
		putchar(c);
	}
	return;
}
