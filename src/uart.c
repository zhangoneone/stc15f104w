#include "uart.h"
#include "timer.h"
#include "delay.h"
#include<stdio.h>
#include<string.h>

#define DATA_WIDTH		8
#define	STOP_BIT			1
#define PARITY_BIT		0

#define rx_bit_r()				P30
#define tx_bit_w(bits)		(P31 = bits)//(P32 = bits)

#define delay_us(us)		delay_us(us)
#define delay_ms(ms)		delay_ms(ms)

#define	BAUD_RATE			115200	//8.68us
//#define	BAUD_RATE			9600	//104us
#define	PERIOD					8

/* 因为是半双工，rx和tx都是通过cpu控制的，
同一时间，cpu只能控制一个gpio的时序，
因此rx时不可tx，tx时也不可rx(但是我们无法控制外部设备的rx时间)
*/
static uint8 rx_done = 1;

static void uart_putc(unsigned char uContent)
{
	unsigned char i = 0;
	
	//完整的数据帧不可被中断
	EA = 0;//1
	//起始信号
	tx_bit_w(0);	// 1
	NOP(12);
	//LSB优先
	for (; i < DATA_WIDTH; i++) //5, 7
	{
		NOP(40);
		NOP(13);
		tx_bit_w(uContent & 0x01);//4
		uContent >>= 1;	//4
	}//3
	if (STOP_BIT) {
		//结束信号
		tx_bit_w(1); //1
		NOP(40);
		NOP(18);
	}
	EA = 1; //1
}

static unsigned char uart_getc()
{
	unsigned char i; 
  unsigned char uContent = 0x00;

	//delay_us(PERIOD >> 1); //在比特信号的中间采样
	NOP(5);
	if(!rx_bit_r()) {
		//数据接收 
		NOP(30);	//延时1us
		for(i = 0; i < DATA_WIDTH; i++) {
			uContent >>= 1;
			NOP(35);	//延时1us
			NOP(8);	//延时0.2us
			//delay_us(PERIOD);
			if(rx_bit_r())
				uContent |= 0x80;
			else
				uContent |= 0x00;
		}
		//停止位
		/* 因为这里是软件模拟的，当一帧数据接收完毕，必须立刻开中断，侦听到下一帧的起始信号
			 考虑到当前数据帧接收完毕后，还会有一些代码运行，因此，在中断接收的状态下，
			 停止位的延时可以适当缩短，甚至去掉停止位的延时，以便下一帧也能顺利接收。
		去掉延时的条件:
			 接收中断是下降沿触发的。因为在一帧数据的停止位，可能产生一个上升沿，这时候触发的中断，
			 并不是起始信号，而是结束信号，因此，如果想要正确触发接收中断，至少要延时到结束信号的上升沿
				已经过去。
		*/
		//if (STOP_BIT)
		//	delay_us(PERIOD * STOP_BIT);
	}
	
	//if(rx_bit_r())
		return uContent;
	
	//return 0;
}


void ext_init() // 定时器0初始化
{
	  P0M0 = 0x00;
    P0M1 = 0x00;
    P1M0 = 0x00;
    P1M1 = 0x00;
    P2M0 = 0x00;
    P2M1 = 0x00;
    P3M0 = 0x00;
    P3M1 = 0x00;
    P4M0 = 0x00;
    P4M1 = 0x00;
    P5M0 = 0x00;
    P5M1 = 0x00;
    P6M0 = 0x00;
    P6M1 = 0x00;
    P7M0 = 0x00;
    P7M1 = 0x00;
	
		INT_CLKO |= 0x40;               //(EX4 = 1)使能INT4中断
}

static volatile uart_fifo_t uart_fifo;

void uart_automic_read(void)
{
	uint8 d;
	
	//提示rx正在进行
	rx_done = 0;
	
	d = uart_getc();
	if (uart_fifo.rct < COM_RX1_Lenth) {	/* Store it into the rx fifo if not full */
			uart_fifo.rct++;
			uart_fifo.rbuf[uart_fifo.rwi] = d;
			uart_fifo.rwi = ++uart_fifo.rwi % COM_RX1_Lenth;
	}
	
	//提示rx已结束
	rx_done = 1;	
}
//从硬件获取rx数据
void exint4() interrupt INT4_VECTOR          //INT4中断入口
{
	//若需要手动清除中断标志,可先关闭中断,此时系统会自动清除内部的中断标志
	INT_CLKO &= 0xBF;
	//上升沿触发的，是结束信号，清中断后，直接返回。
//	if(rx_bit_r()) {
//			INT_CLKO |= 0x40;               //然后再开中断即可
//			return;
//	}
	uart_automic_read();
	
	INT_CLKO |= 0x40;               //然后再开中断即可
}

void uart_init()
{
	ext_init();
//	uart_automic_read();
}

//重定向putchar和_getkey，以使用stdio中的所有库函数
char putchar(char c)
{
		
	/*因为uart传输是连续的，如果一个停止位时间内，rx上还没有产生下降沿，我们可以认为
	  这一波数据已经传输完成了。因此可以设置rx_done标志位。
	*/
	while (1) {
		delay_us(PERIOD * STOP_BIT * 2); //等待2倍的停止位
		if(rx_done) //确定rx已经空闲
			break;
	}

	uart_putc(c);
	
	return c;
}

char _getkey(void)
{
	uint8 d;
	uint8 i;

	/* Wait while rx fifo is empty */
	while (!uart_fifo.rct);

	i = uart_fifo.rri;			/* Get a byte from rx fifo */
	d = uart_fifo.rbuf[i];
	uart_fifo.rri = ++i % COM_RX1_Lenth;
	EA = 0;
	uart_fifo.rct--;
	EA = 1;

	return d;
}

char getchar(void)
{
	return _getkey();
}
