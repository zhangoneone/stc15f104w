#include "uart.h"
#include "timer.h"
#include "delay.h"
//#include<stdio.h>
#include<string.h>

#define DATA_WIDTH		8
#define	STOP_BIT			1
#define PARITY_BIT		0
#if (IRQ_UART == 0)
#define rx_bit_r()				P30
#define rx_bit_w(bits)		(P30 = bits)
#define tx_bit_w(bits)		(P31 = bits)//(P32 = bits)

#define delay_us(us)		delay_us(us)
#define delay_ms(ms)		delay_ms(ms)

//#define	BAUD_RATE			115200	//8.68us
//#define	BAUD_RATE			9600	//104us
#define	BAUD_RATE			460800	//2.17us
//#define	PERIOD					8

/* 因为是半双工，rx和tx都是通过cpu控制的，
同一时间，cpu只能控制一个gpio的时序，
因此rx时不可tx，tx时也不可rx(但是我们无法控制外部设备的rx时间)
*/

static void uart_putc(unsigned char uContent)
{
	unsigned char i = 0;
	
	//完整的数据帧不可被中断
	EA = 0;//1
	//起始信号
	tx_bit_w(0);	// 1
	NOP(12);
	//LSB优先
	for (;i < DATA_WIDTH; i++) //5, 7
	{
		NOP(40);
		NOP(13);
		tx_bit_w(uContent & 0x01);//4
		uContent >>= 1;	//4
	}//3
	//最后一个bit采样保持时间
	NOP(40);
	NOP(15);
	if (STOP_BIT) {
		//结束信号
		tx_bit_w(1); //1
		NOP(40);
		NOP(25);
	}
	EA = 1; //1
}

static unsigned char uart_getc()
{
	unsigned char i = 0; 
  unsigned char uContent = 0x00; //2

	if(!rx_bit_r()) { //2
		NOP(5); 
		//数据接收 
		for(;i < DATA_WIDTH; i++) { // 5 7
			uContent >>= 1; //4
			NOP(40);
			NOP(4);
			if(rx_bit_r()) //4?
				uContent |= 0x80; //4
			else
				uContent |= 0x00;//4
		}//3
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

	return uContent;

}//2


void ext_init() // 外部中断4初始化
{
    P3M0 = 0x00;
    P3M1 = 0x00;
		//IP2 |= PSH; 										//PX4高优先级
		INT_CLKO |= 0x40;               //(EX4 = 1)使能INT4中断
}

static volatile uart_fifo_t uart_fifo;

void uart_automic_read(void)
{
	uint8 d;
	
	//提示rx正在进行
	uart_rx_done = 0; //2
	
	d = uart_getc();
	//以下约33机器周期
	if (uart_fifo.rct < COM_RX1_Lenth) {	/* Store it into the rx fifo if not full */
			uart_fifo.rct++;
			uart_fifo.rbuf[uart_fifo.rwi] = d;
			uart_fifo.rwi = ++uart_fifo.rwi % COM_RX1_Lenth;
	}
	
	//提示rx已结束
	uart_rx_done = 1;	
}
//从硬件获取rx数据
void exint4() interrupt INT4_VECTOR          //INT4中断入口
{//中断响应机器周期是3~8,取5
	//若需要手动清除中断标志,可先关闭中断,此时系统会自动清除内部的中断标志
	EA = 0;
	INT_CLKO &= 0xBF; //1
	//上升沿触发的，是结束信号，清中断后，直接返回。
//	if(rx_bit_r()) {
//			INT_CLKO |= 0x40;               //然后再开中断即可
//			return;
//	}
	uart_automic_read(); //2
	
	INT_CLKO |= 0x40;    //1           //然后再开中断即可
	EA = 1;
}

void uart_init()
{
	rx_bit_w(1);
	ext_init();
	uart_rx_done = 1;
}

//重定向putchar和_getkey，以使用stdio中的所有库函数
char putchar(char c)
{
		
	/*因为uart传输是连续的，如果一个停止位时间内，rx上还没有产生下降沿，我们可以认为
	  这一波数据已经传输完成了。因此可以设置rx_done标志位。
	*/
	while (1) {
		NOP(40);
		NOP(40);
		NOP(40);
		NOP(16);//等待2倍的停止位
		if(uart_rx_done) //确定rx已经空闲
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

uint8 uart_fifo_count(void)
{
	return uart_fifo.rct;
}

char getchar(void)
{
	return _getkey();
}

int puts(const char *s)
{
	unsigned char i = 0;
	
	while(*s) {
		putchar(*s++);
		i++;
	}
	
	return i;
}

char *gets(char *s, int n)
{
	while(--n >= 0)
		*s++ = getchar();
	
	return s;
	
}

#endif

#if (IRQ_UART == 1)

#define rx_bit_r()				P30
#define rx_bit_w(bits)		(P30 = bits)
#define tx_bit_w(bits)		(P31 = bits)//(P32 = bits)
//#define	TX_TRIGGER()			{rx_bit_w(1);rx_bit_w(0);} //产生下降沿
#define	TX_TRIGGER()			{P30 = 0;} //在rx产生下降沿,进入中断
#define	TX_TRIGGER_RESET(){P30 = 1;} //恢复rx的空闲状态
#define UART3_Baudrate  1000UL    //定义波特率

#define UART3_BitTime   		(MAIN_Fosc / UART3_Baudrate)
#define	EX_INT_ENABLE()			(INT_CLKO |= 0x40)
#define	EX_INT_DISABLE()		(INT_CLKO &= 0xBF)
#define	TIMER_START()
#define	TIMER_STOP()

#define	SM_BUS_IDLE					(0x01U << 0)
#define	SM_START_BIT				(0x01U << 1)
#define	SM_DATA_BITS				(0x01U << 2)
#define	SM_STOP_BITS				(0x01U << 3)

typedef struct soft_uart_ctrl_t {
	u8 rx_bits, tx_bits;
	u8 rx_done, tx_done;
	u8 rx_sm, tx_sm;
	u8 rx_work_bits, tx_work_bits;
	u8 stop_bits;
} soft_uart_ctrl_t;

static volatile uart_fifo_t uart_fifo;
static volatile soft_uart_ctrl_t uart_ctrl;

static void timer_set(void)
{
		AUXR &=  ~(1<<4);   //Timer2 停止运行
    T2H = (65536 -  UART3_BitTime) / 256;  //一个数据位
    T2L = (65536 -  UART3_BitTime) % 256;  //一个数据位
    AUXR |=  (1<<4);    //Timer2 开始运行
}
static void timer_unset(void)
{
	AUXR &=  ~(1<<4);   //Timer2 停止运行
}

static void uart_putc(unsigned char c)
{
	unsigned char i = 0;

	/* Wait for tx fifo is not full */
	if (uart_fifo.tct >= COM_TX1_Lenth)
		return;

	i = uart_fifo.twi;		/* Put a byte into Tx fifo */
	uart_fifo.tbuf[i] = c;
	uart_fifo.twi = ++i % COM_TX1_Lenth;
	EA = 0;
	uart_fifo.tct++;
	EA = 1;

	return;
}
extern void timer2_int (void);
void uart_init()
{
	P3M0 = 0x00;
  P3M1 = 0x00;
	rx_bit_w(1);
	
	AUXR &=  ~(1<<4);       // Timer2 停止运行
  T2H = (65536 - UART3_BitTime) / 256;    // 数据位
  T2L = (65536 - UART3_BitTime) % 256;    // 数据位
	IP2 |= PSH; 										//PX4高优先级
  INT_CLKO |=  (1 << 6);  // 允许INT4中断
  IE2  |=  (1<<2);        // 允许Timer2中断
  AUXR |=  (1<<2);        // 1T
	
	uart_ctrl.rx_done = 1;
	uart_ctrl.tx_done = 1;
	
	uart_ctrl.tx_sm = SM_BUS_IDLE;	
	uart_ctrl.rx_sm = SM_BUS_IDLE;

}

//重定向putchar和_getkey，以使用stdio中的所有库函数
char putchar(char c)
{
	
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

uint8 uart_fifo_count(void)
{
	return uart_fifo.rct;
}

char getchar(void)
{
	return _getkey();
}

int puts(const char *s)
{
	unsigned char i = 0;
	
	while(*s) {
		putchar(*s++);
		i++;
	}
	
	return i;
}

char *gets(char *s, int n)
{
	while(--n >= 0)
		*s++ = getchar();
	
	return s;
	
}





//========================================================================
// 函数: void   timer2_int (void) interrupt 12
// 描述: Timer2中断处理程序.
// 参数: None
// 返回: none.
// 版本: V1.0, 2012-11-22
//========================================================================
void timer2_int (void) interrupt TIMER2_VECTOR
{
	u8 i = 0;
	
	//rx
	if (!uart_ctrl.rx_done) {
		//收开始位
		if (uart_ctrl.rx_sm == SM_BUS_IDLE) {
			if (!rx_bit_r()) {
				//reset控制时序
				timer_set();
				uart_ctrl.rx_work_bits = 0;
				uart_ctrl.rx_bits = DATA_WIDTH;
				uart_ctrl.stop_bits = STOP_BIT;
			} else {
				uart_ctrl.rx_sm = SM_BUS_IDLE;
				uart_ctrl.rx_bits = 0;
				uart_ctrl.stop_bits = 0;
				//停止rx时序控制
				timer_unset();
				//监听串口中断
				EX_INT_ENABLE();
				uart_ctrl.rx_done = 1;
				return;
			}

			uart_ctrl.rx_sm = SM_START_BIT;
			
		}
		//收数据位
		else if (uart_ctrl.rx_sm == SM_START_BIT) {
			if (uart_ctrl.rx_bits--) {
				uart_ctrl.rx_work_bits >>= 1;
				if(rx_bit_r())
					uart_ctrl.rx_work_bits |= 0x80;
			} else
				uart_ctrl.rx_sm = SM_DATA_BITS;
		}
		//收停止位
		else if (uart_ctrl.rx_sm == SM_DATA_BITS) {
			if (uart_ctrl.stop_bits) {
				uart_ctrl.stop_bits--;
			} else {
				uart_ctrl.rx_sm = SM_STOP_BITS;
			}
		}
		//协议结束
		else if (uart_ctrl.rx_sm == SM_STOP_BITS) {
			if (uart_fifo.rct < COM_RX1_Lenth) {	/* Store it into the rx fifo if not full */
				uart_fifo.rct++;
				i = uart_fifo.rwi;
				uart_fifo.rbuf[i] = uart_ctrl.rx_work_bits;
				uart_fifo.rwi = ++i % COM_RX1_Lenth;
			}
			
			uart_ctrl.rx_sm = SM_BUS_IDLE;
			//停止rx时序控制
			timer_unset();
			//监听串口中断
			EX_INT_ENABLE();
			uart_ctrl.rx_done = 1;
			return;
		}

	}

}




static u8 uart_try_tx()
{
	if(uart_ctrl.rx_done) {
		delay_ms(20); //目前1000波特率，写死延时2ms
	} else {
		return;
	}
	if(uart_ctrl.rx_done) {
		//确定rx已经空闲
		if (uart_ctrl.tx_done != 0) {
			return (uart_ctrl.tx_done = 0);
		}
	}
}


u8 uart_fifo_send(void)
{
	u8 i = 0;
	uart_try_tx();
	//tx
	if (!uart_ctrl.tx_done) {
		//发开始位		
		if (uart_ctrl.tx_sm == SM_BUS_IDLE) {
			//开始tx时序控制
			timer_set();
			uart_ctrl.tx_bits = DATA_WIDTH;
			uart_ctrl.stop_bits = STOP_BIT;
			
			//获取待发送的字节
			if (uart_fifo.tct) {	/* There is any data in the tx fifo */
				i = uart_fifo.tri;
				uart_ctrl.tx_work_bits = uart_fifo.tbuf[i];
				uart_fifo.tri = ++i % COM_TX1_Lenth;
				uart_fifo.tct--;
			} else {
				//停止tx时序控制
				timer_unset();
				uart_ctrl.tx_done = 1;
				uart_ctrl.tx_sm = SM_BUS_IDLE;
				//监听串口中断
				EX_INT_ENABLE();
				return;
			}
			//产生开始位
			tx_bit_w(0);
			uart_ctrl.tx_sm = SM_START_BIT;
			
		} 
		//发数据位
		else if (uart_ctrl.tx_sm == SM_START_BIT) {
			if (uart_ctrl.tx_bits--) {
				if (uart_ctrl.tx_work_bits & 0x01)
					tx_bit_w(1);
				else
					tx_bit_w(0);
				
				uart_ctrl.tx_work_bits >>= 1;
			} else {
				uart_ctrl.tx_sm = SM_DATA_BITS;
			}	
		}
		//发停止位
		else if (uart_ctrl.tx_sm == SM_DATA_BITS) {
			if (uart_ctrl.stop_bits) {
				tx_bit_w(1);
				uart_ctrl.stop_bits--;
			} else {
				uart_ctrl.tx_sm = SM_STOP_BITS;
			}
		}
		//协议结束
		else if (uart_ctrl.tx_sm == SM_STOP_BITS) {
			if (!uart_fifo.tct) {
				//停止tx时序控制
				timer_unset();
				uart_ctrl.tx_done = 1;
				uart_ctrl.tx_sm = SM_BUS_IDLE;
				//监听串口中断
				EX_INT_ENABLE();
				return;
			} else {
				uart_ctrl.tx_sm = SM_BUS_IDLE;	
				goto retry_tx;
			}
		}

	}
	
}


/********************* INT4中断函数 *************************/
void Ext_INT4 (void) interrupt INT4_VECTOR
{
	
	EX_INT_DISABLE();  //禁止INT4中断

	//rx时序控制
	uart_ctrl.rx_done = 0;
	uart_ctrl.rx_sm = SM_BUS_IDLE;
	AUXR &=  ~(1<<4);   //Timer2 停止运行
  T2H = (65536 - UART3_BitTime / 4) / 256;  //半个数据位
  T2L = (65536 - UART3_BitTime / 4) % 256;  //半个数据位
  AUXR |=  (1<<4);    //Timer2 开始运行

}



#endif


