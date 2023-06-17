#include "uart.h"
#include "timer.h"
#include "delay.h"

/*
������Ҫcpuȫ�̲�����ʱ����uartʱ��
���Լ�ʹrx/tx�������ӵģ�Ҳ�޷�����ȫ˫����
Ϊ�˴Ӱ�˫����������ȫ˫����
��Ҫ�����ǽ�ų�cpu����ͬʱҪ��֤��ȷ����ʱ

���ڲ����ʽϸߵ�uart����1M�����ʣ���Ҫ1us�ľ�ȷ��ʱ�������������к���ʱ�Ĵ��롣

���嵽���õ����оƬ�����35MHZ����Ƶ��1Tָ�����ڡ�1usʱ�䣬��Լ����35��ָ�
���Ҫ��취���㣬���ʵ�ֵ�uart�շ������У������ִ��ʱ�䡣

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
	
	//����������֡���ɱ��ж�
	EA = 0;
	//��ʼ�ź�
	tx_bit_w(0);
	delay_us(PERIOD);
	//LSB����
	for (i = 0; i < DATA_WIDTH; i++)
	{
		tx_bit_w(c & 0x01);
		c >>= 1;
		delay_us(PERIOD);  
	}
	if (STOP_BIT) {
		//�����ź�
		tx_bit_w(1);
		delay_us(PERIOD * STOP_BIT);
	}
	EA = 1;
}