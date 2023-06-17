#include "uart.h"
#include "timer.h"
#include "delay.h"

/*
由于需要cpu全程参与延时控制uart时序，
所以即使rx/tx分线连接的，也无法做到全双工。
为了从半双工，升级成全双工，
首要问题是解放出cpu，但同时要保证精确的延时

对于波特率较高的uart，如1M波特率，需要1us的精确延时，包含代码运行和延时的代码。

具体到在用的这块芯片，最高35MHZ的主频，1T指令周期。1us时间，大约运行35条指令。
因此要想办法计算，软件实现的uart收发函数中，代码的执行时间。

*/

#define DATA_WIDTH		8
#define	STOP_BIT			1
#define PARITY_BIT		0

#define rx_bit_r()				P30
#define tx_bit_w(bits)		(P31 = bits)//(P32 = bits)

#define delay_us(us)		delay_us(us)
#define delay_ms(ms)		delay_ms(ms)

#define	BAUD_RATE			115200	//8.68us
//#define	BAUD_RATE			9600	//104us
#define	PERIOD					50


static void uart_putc(unsigned char c)
{
	unsigned char i;
	
	//完整的数据帧不可被中断
	EA = 0;
	//起始信号
	tx_bit_w(0);
	delay_us(PERIOD);
	//LSB优先
	for (i = 0; i < DATA_WIDTH; i++)
	{
		tx_bit_w(c & 0x01);
		c >>= 1;
		delay_us(PERIOD);  
	}
	if (STOP_BIT) {
		//结束信号
		tx_bit_w(1);
		delay_us(PERIOD * STOP_BIT);
	}
	EA = 1;
}