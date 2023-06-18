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
		IP2 |= PSH; 										//PX4�����ȼ�
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


