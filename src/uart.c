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

/* ��Ϊ�ǰ�˫����rx��tx����ͨ��cpu���Ƶģ�
ͬһʱ�䣬cpuֻ�ܿ���һ��gpio��ʱ��
���rxʱ����tx��txʱҲ����rx(���������޷������ⲿ�豸��rxʱ��)
*/

static void uart_putc(unsigned char uContent)
{
	unsigned char i = 0;
	
	//����������֡���ɱ��ж�
	EA = 0;//1
	//��ʼ�ź�
	tx_bit_w(0);	// 1
	NOP(12);
	//LSB����
	for (;i < DATA_WIDTH; i++) //5, 7
	{
		NOP(40);
		NOP(13);
		tx_bit_w(uContent & 0x01);//4
		uContent >>= 1;	//4
	}//3
	//���һ��bit��������ʱ��
	NOP(40);
	NOP(15);
	if (STOP_BIT) {
		//�����ź�
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
		//���ݽ��� 
		for(;i < DATA_WIDTH; i++) { // 5 7
			uContent >>= 1; //4
			NOP(40);
			NOP(4);
			if(rx_bit_r()) //4?
				uContent |= 0x80; //4
			else
				uContent |= 0x00;//4
		}//3
		//ֹͣλ
		/* ��Ϊ���������ģ��ģ���һ֡���ݽ�����ϣ��������̿��жϣ���������һ֡����ʼ�ź�
			 ���ǵ���ǰ����֡������Ϻ󣬻�����һЩ�������У���ˣ����жϽ��յ�״̬�£�
			 ֹͣλ����ʱ�����ʵ����̣�����ȥ��ֹͣλ����ʱ���Ա���һ֡Ҳ��˳�����ա�
		ȥ����ʱ������:
			 �����ж����½��ش����ġ���Ϊ��һ֡���ݵ�ֹͣλ�����ܲ���һ�������أ���ʱ�򴥷����жϣ�
			 ��������ʼ�źţ����ǽ����źţ���ˣ������Ҫ��ȷ���������жϣ�����Ҫ��ʱ�������źŵ�������
				�Ѿ���ȥ��
		*/
		//if (STOP_BIT)
		//	delay_us(PERIOD * STOP_BIT);
		
	}

	return uContent;

}//2


void ext_init() // �ⲿ�ж�4��ʼ��
{
    P3M0 = 0x00;
    P3M1 = 0x00;
		//IP2 |= PSH; 										//PX4�����ȼ�
		INT_CLKO |= 0x40;               //(EX4 = 1)ʹ��INT4�ж�
}

static volatile uart_fifo_t uart_fifo;

void uart_automic_read(void)
{
	uint8 d;
	
	//��ʾrx���ڽ���
	uart_rx_done = 0; //2
	
	d = uart_getc();
	//����Լ33��������
	if (uart_fifo.rct < COM_RX1_Lenth) {	/* Store it into the rx fifo if not full */
			uart_fifo.rct++;
			uart_fifo.rbuf[uart_fifo.rwi] = d;
			uart_fifo.rwi = ++uart_fifo.rwi % COM_RX1_Lenth;
	}
	
	//��ʾrx�ѽ���
	uart_rx_done = 1;	
}
//��Ӳ����ȡrx����
void exint4() interrupt INT4_VECTOR          //INT4�ж����
{//�ж���Ӧ����������3~8,ȡ5
	//����Ҫ�ֶ�����жϱ�־,���ȹر��ж�,��ʱϵͳ���Զ�����ڲ����жϱ�־
	EA = 0;
	INT_CLKO &= 0xBF; //1
	//�����ش����ģ��ǽ����źţ����жϺ�ֱ�ӷ��ء�
//	if(rx_bit_r()) {
//			INT_CLKO |= 0x40;               //Ȼ���ٿ��жϼ���
//			return;
//	}
	uart_automic_read(); //2
	
	INT_CLKO |= 0x40;    //1           //Ȼ���ٿ��жϼ���
	EA = 1;
}

void uart_init()
{
	rx_bit_w(1);
	ext_init();
	uart_rx_done = 1;
}

//�ض���putchar��_getkey����ʹ��stdio�е����п⺯��
char putchar(char c)
{
		
	/*��Ϊuart�����������ģ����һ��ֹͣλʱ���ڣ�rx�ϻ�û�в����½��أ����ǿ�����Ϊ
	  ��һ�������Ѿ���������ˡ���˿�������rx_done��־λ��
	*/
	while (1) {
		NOP(40);
		NOP(40);
		NOP(40);
		NOP(16);//�ȴ�2����ֹͣλ
		if(uart_rx_done) //ȷ��rx�Ѿ�����
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
//#define	TX_TRIGGER()			{rx_bit_w(1);rx_bit_w(0);} //�����½���
#define	TX_TRIGGER()			{P30 = 0;} //��rx�����½���,�����ж�
#define	TX_TRIGGER_RESET(){P30 = 1;} //�ָ�rx�Ŀ���״̬
#define UART3_Baudrate  1000UL    //���岨����

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
		AUXR &=  ~(1<<4);   //Timer2 ֹͣ����
    T2H = (65536 -  UART3_BitTime) / 256;  //һ������λ
    T2L = (65536 -  UART3_BitTime) % 256;  //һ������λ
    AUXR |=  (1<<4);    //Timer2 ��ʼ����
}
static void timer_unset(void)
{
	AUXR &=  ~(1<<4);   //Timer2 ֹͣ����
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
	
	AUXR &=  ~(1<<4);       // Timer2 ֹͣ����
  T2H = (65536 - UART3_BitTime) / 256;    // ����λ
  T2L = (65536 - UART3_BitTime) % 256;    // ����λ
	IP2 |= PSH; 										//PX4�����ȼ�
  INT_CLKO |=  (1 << 6);  // ����INT4�ж�
  IE2  |=  (1<<2);        // ����Timer2�ж�
  AUXR |=  (1<<2);        // 1T
	
	uart_ctrl.rx_done = 1;
	uart_ctrl.tx_done = 1;
	
	uart_ctrl.tx_sm = SM_BUS_IDLE;	
	uart_ctrl.rx_sm = SM_BUS_IDLE;

}

//�ض���putchar��_getkey����ʹ��stdio�е����п⺯��
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
// ����: void   timer2_int (void) interrupt 12
// ����: Timer2�жϴ������.
// ����: None
// ����: none.
// �汾: V1.0, 2012-11-22
//========================================================================
void timer2_int (void) interrupt TIMER2_VECTOR
{
	u8 i = 0;
	
	//rx
	if (!uart_ctrl.rx_done) {
		//�տ�ʼλ
		if (uart_ctrl.rx_sm == SM_BUS_IDLE) {
			if (!rx_bit_r()) {
				//reset����ʱ��
				timer_set();
				uart_ctrl.rx_work_bits = 0;
				uart_ctrl.rx_bits = DATA_WIDTH;
				uart_ctrl.stop_bits = STOP_BIT;
			} else {
				uart_ctrl.rx_sm = SM_BUS_IDLE;
				uart_ctrl.rx_bits = 0;
				uart_ctrl.stop_bits = 0;
				//ֹͣrxʱ�����
				timer_unset();
				//���������ж�
				EX_INT_ENABLE();
				uart_ctrl.rx_done = 1;
				return;
			}

			uart_ctrl.rx_sm = SM_START_BIT;
			
		}
		//������λ
		else if (uart_ctrl.rx_sm == SM_START_BIT) {
			if (uart_ctrl.rx_bits--) {
				uart_ctrl.rx_work_bits >>= 1;
				if(rx_bit_r())
					uart_ctrl.rx_work_bits |= 0x80;
			} else
				uart_ctrl.rx_sm = SM_DATA_BITS;
		}
		//��ֹͣλ
		else if (uart_ctrl.rx_sm == SM_DATA_BITS) {
			if (uart_ctrl.stop_bits) {
				uart_ctrl.stop_bits--;
			} else {
				uart_ctrl.rx_sm = SM_STOP_BITS;
			}
		}
		//Э�����
		else if (uart_ctrl.rx_sm == SM_STOP_BITS) {
			if (uart_fifo.rct < COM_RX1_Lenth) {	/* Store it into the rx fifo if not full */
				uart_fifo.rct++;
				i = uart_fifo.rwi;
				uart_fifo.rbuf[i] = uart_ctrl.rx_work_bits;
				uart_fifo.rwi = ++i % COM_RX1_Lenth;
			}
			
			uart_ctrl.rx_sm = SM_BUS_IDLE;
			//ֹͣrxʱ�����
			timer_unset();
			//���������ж�
			EX_INT_ENABLE();
			uart_ctrl.rx_done = 1;
			return;
		}

	}

}




static u8 uart_try_tx()
{
	if(uart_ctrl.rx_done) {
		delay_ms(20); //Ŀǰ1000�����ʣ�д����ʱ2ms
	} else {
		return;
	}
	if(uart_ctrl.rx_done) {
		//ȷ��rx�Ѿ�����
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
		//����ʼλ		
		if (uart_ctrl.tx_sm == SM_BUS_IDLE) {
			//��ʼtxʱ�����
			timer_set();
			uart_ctrl.tx_bits = DATA_WIDTH;
			uart_ctrl.stop_bits = STOP_BIT;
			
			//��ȡ�����͵��ֽ�
			if (uart_fifo.tct) {	/* There is any data in the tx fifo */
				i = uart_fifo.tri;
				uart_ctrl.tx_work_bits = uart_fifo.tbuf[i];
				uart_fifo.tri = ++i % COM_TX1_Lenth;
				uart_fifo.tct--;
			} else {
				//ֹͣtxʱ�����
				timer_unset();
				uart_ctrl.tx_done = 1;
				uart_ctrl.tx_sm = SM_BUS_IDLE;
				//���������ж�
				EX_INT_ENABLE();
				return;
			}
			//������ʼλ
			tx_bit_w(0);
			uart_ctrl.tx_sm = SM_START_BIT;
			
		} 
		//������λ
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
		//��ֹͣλ
		else if (uart_ctrl.tx_sm == SM_DATA_BITS) {
			if (uart_ctrl.stop_bits) {
				tx_bit_w(1);
				uart_ctrl.stop_bits--;
			} else {
				uart_ctrl.tx_sm = SM_STOP_BITS;
			}
		}
		//Э�����
		else if (uart_ctrl.tx_sm == SM_STOP_BITS) {
			if (!uart_fifo.tct) {
				//ֹͣtxʱ�����
				timer_unset();
				uart_ctrl.tx_done = 1;
				uart_ctrl.tx_sm = SM_BUS_IDLE;
				//���������ж�
				EX_INT_ENABLE();
				return;
			} else {
				uart_ctrl.tx_sm = SM_BUS_IDLE;	
				goto retry_tx;
			}
		}

	}
	
}


/********************* INT4�жϺ��� *************************/
void Ext_INT4 (void) interrupt INT4_VECTOR
{
	
	EX_INT_DISABLE();  //��ֹINT4�ж�

	//rxʱ�����
	uart_ctrl.rx_done = 0;
	uart_ctrl.rx_sm = SM_BUS_IDLE;
	AUXR &=  ~(1<<4);   //Timer2 ֹͣ����
  T2H = (65536 - UART3_BitTime / 4) / 256;  //�������λ
  T2L = (65536 - UART3_BitTime / 4) % 256;  //�������λ
  AUXR |=  (1<<4);    //Timer2 ��ʼ����

}



#endif


