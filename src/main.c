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

volatile unsigned char bdata g_event = 0;
sbit uart_rx_done = g_event^7;
sbit uart_tx_done = g_event^6;

void pla_init()
{
	P3 = 0;
	timer0_init(1000);
	uart_init();
	EA = 1; // 打开总中断
}

void main()
{
	char c = 0;
	pla_init();
	
	for(;;) {
		//TaskProcess();
		c = getchar();
		if (c == 0x12)
			c =0x00;
		delay_ms(200);
		putchar(c);
		delay_ms(40);
		//putchar(c+0x1);
		delay_ms(40);
	}
	
	return;
}
