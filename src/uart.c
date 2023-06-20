#include "uart.h"
#include "timer.h"
#include "delay.h"
//#include<stdio.h>
#include<string.h>

#define DATA_WIDTH		8
#define	STOP_BIT			1
#define PARITY_BIT		0

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

static void uart_try_tx(void)
{
	if(uart_ctrl.rx_done) {
		delay_ms(20); //目前1000波特率，写死延时2ms
	} else {
		return;
	}
	if(uart_ctrl.rx_done) {
		//确定rx已经空闲
		if (uart_ctrl.tx_done != 0) {
			uart_ctrl.tx_done = 0;
			TX_TRIGGER();
		}
	}
}

static void uart_putc(unsigned char c)
{
	unsigned char i = 0;

	/* Wait for tx fifo is not full */
	while (uart_fifo.tct >= COM_TX1_Lenth) {
			uart_try_tx();
	}

	i = uart_fifo.twi;		/* Put a byte into Tx fifo */
	uart_fifo.tbuf[i] = c;
	uart_fifo.twi = ++i % COM_TX1_Lenth;
	EA = 0;
	uart_fifo.tct++;
	EA = 1;
	
	uart_try_tx();

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
	retry_tx:
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
	AUXR &=  ~(1<<4);   //Timer2 停止运行
	
	//tx请求
	if (!uart_ctrl.tx_done) {
		TX_TRIGGER_RESET();
		T2H = 0xFF;
    T2L = 0x00; //255个周期给timer重设置
    AUXR |=  (1<<4);    //Timer2 开始运行	
	} 
	else {
		//rx时序控制
		uart_ctrl.rx_done = 0;
		uart_ctrl.rx_sm = SM_BUS_IDLE;
    T2H = (65536 - UART3_BitTime / 4) / 256;  //半个数据位
    T2L = (65536 - UART3_BitTime / 4) % 256;  //半个数据位
    AUXR |=  (1<<4);    //Timer2 开始运行
	}

}




