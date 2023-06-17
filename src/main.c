#include "stc15.h"
#include "typedef.h"
#include "timer.h"
#include "one_os.h"
#include "config.h"
#include "uart.h"
#include "delay.h"
//#include <stdio.h>
//P30 P31 P32 P33 P34 P35
//����P30��rx��P31��tx

void pla_init()
{
	timer0_init(1000);
	uart_init();
	EA = 1; // �����ж�
}

void main()
{
	char c = 0;
	pla_init();
	for(;;) {
		//TaskProcess();
		//scanf(str, "%s");
		c = getchar();
		//delay_ms(10);
		putchar(c);
		//delay_ms(100);
		//delay_ms(100);
	}
	return;
}
