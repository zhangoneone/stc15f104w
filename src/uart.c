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
#define	PERIOD					1000

/* ��Ϊ�ǰ�˫����rx��tx����ͨ��cpu���Ƶģ�
ͬһʱ�䣬cpuֻ�ܿ���һ��gpio��ʱ��
���rxʱ����tx��txʱҲ����rx(���������޷������ⲿ�豸��rxʱ��)
*/
static uint8 rx_done = 1;

static void uart_putc(unsigned char uContent)
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
		tx_bit_w(uContent & 0x01);
		uContent >>= 1;
		delay_us(PERIOD);  
	}
	if (STOP_BIT) {
		//�����ź�
		tx_bit_w(1);
		delay_us(PERIOD * STOP_BIT);
	}
	EA = 1;
}

static unsigned char uart_getc()
{
	unsigned char i; 
  unsigned char uContent = 0x00;

	delay_us(PERIOD >> 1); //�ڱ����źŵ��м����
	if(!rx_bit_r()) {
		//���ݽ���     
		for(i = 0; i < DATA_WIDTH; i++) {
			uContent >>= 1;
			delay_us(PERIOD);
			if(rx_bit_r())
				uContent |= 0x80;
		}
		//ֹͣλ
		/* ��Ϊ���������ģ��ģ���һ֡���ݽ�����ϣ��������̿��жϣ���������һ֡����ʼ�ź�
			 ���ǵ���ǰ����֡������Ϻ󣬻�����һЩ�������У���ˣ����жϽ��յ�״̬�£�
			 ֹͣλ����ʱ�����ʵ����̣�����ȥ��ֹͣλ����ʱ���Ա���һ֡Ҳ��˳�����ա�
		ȥ����ʱ������:
			 �����ж����½��ش����ġ���Ϊ��һ֡���ݵ�ֹͣλ�����ܲ���һ�������أ���ʱ�򴥷����жϣ�
			 ��������ʼ�źţ����ǽ����źţ���ˣ������Ҫ��ȷ���������жϣ�����Ҫ��ʱ�������źŵ�������
				�Ѿ���ȥ��
		*/
		if (STOP_BIT)
			delay_us(PERIOD * STOP_BIT);
	}
	
	if(rx_bit_r())
		return uContent;
	
	return 0;
}


void ext_init() // ��ʱ��0��ʼ��
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
	
		INT_CLKO |= 0x40;               //(EX4 = 1)ʹ��INT4�ж�
}

static volatile uart_fifo_t uart_fifo;
//��Ӳ����ȡrx����
void exint4() interrupt INT4_VECTOR          //INT4�ж����
{
	uint8 d;
	uint8 i;

	//����Ҫ�ֶ�����жϱ�־,���ȹر��ж�,��ʱϵͳ���Զ�����ڲ����жϱ�־
	INT_CLKO &= 0xBF;
	//�����ش����ģ��ǽ����źţ����жϺ�ֱ�ӷ��ء�
	if(rx_bit_r()) {
			INT_CLKO |= 0x40;               //Ȼ���ٿ��жϼ���
			return;
	}
	
	//��ʾrx���ڽ���
	rx_done = 0;
	
	d = uart_getc();
	i = uart_fifo.rct;
	if (i < COM_RX1_Lenth) {	/* Store it into the rx fifo if not full */
			uart_fifo.rct = ++i;
			i = uart_fifo.rwi;
			uart_fifo.rbuf[i] = d;
			uart_fifo.rwi = ++i % COM_RX1_Lenth;
	}
	
	//��ʾrx�ѽ���
	rx_done = 1;	
	INT_CLKO |= 0x40;               //Ȼ���ٿ��жϼ���
}

void uart_init()
{
	ext_init();
}

//�ض���putchar��_getkey����ʹ��stdio�е����п⺯��
char putchar(char c)
{
		
	/*��Ϊuart�����������ģ����һ��ֹͣλʱ���ڣ�rx�ϻ�û�в����½��أ����ǿ�����Ϊ
	  ��һ�������Ѿ���������ˡ���˿�������rx_done��־λ��
	*/
	while (1) {
		delay_us(PERIOD * STOP_BIT * 2); //�ȴ�2����ֹͣλ
		if(rx_done) //ȷ��rx�Ѿ�����
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
