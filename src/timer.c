#include "stc15.h"
#include "typedef.h"
#include "one_os.h"
#include "config.h"
#define FSOC		MAIN_Fosc/1000000 //��Ƭ��Ƶ��35MHZ
//��T0��T2

//�������ʱ�䣬����Ҫ���õĳ�ֵ
static u16 
primary_value(u16 overflow_time_us)
{
	  return (65536 - FSOC * overflow_time_us);
}

//��ʱ��0 nus����һ��
void timer0_init(u16 nus) // ��ʱ��0��ʼ��
{
	u16 times;
	
	times = primary_value(nus);
	AUXR |= 0x80; //������1Tģʽ 
	TMOD &= 0xF0; // ���ö�ʱ��0Ϊ 16λ��ʱ��ģʽ,ģʽ0
	TH0 = times >> 8;
	TL0 = (times & 0xFF);
	
	TR0 = 1; // �򿪶�ʱ�� 0
	PT0 = 1; // �����ȼ�
	ET0 = 1; // �򿪶�ʱ�� 0 �ж�
}
#if 0
//��ʱ��2 nus����һ��
void timer2_init(u16 nus) // ��ʱ��2��ʼ��
{
	u16 times;
	
	times = primary_value(nus);
	AUXR |= 0x04; //������1Tģʽ 
	
	TMOD &= 0xF0; // ���ö�ʱ��2Ϊ����ģʽ0 16λ��ʱ��ģʽ������ģʽ������0
	T2H = times >> 8;
	T2L = (times & 0xFF);
	
	AUXR |= 0x10;                   //��ʱ��2��ʼ��ʱ 
  IE2 |= 0x04;                    //����ʱ��2�ж�
}
#endif


//��ʱ��0
void tm0_isr() interrupt 1
{
	//systick
	TaskRemarks();
}