#include "stc15.h"
#include "typedef.h"
#include "one_os.h"
#define FSOC		35 //单片机频率35MHZ
//有T0和T2

//传入溢出时间，传出要设置的初值
static u16 
primary_value(u16 overflow_time_us)
{
	  return (65536 - FSOC * overflow_time_us);
}

//定时器0 nus触发一次
void timer0_init(u16 nus) // 定时器0初始化
{
	u16 times;
	
	times = primary_value(nus);
	AUXR |= 0x80; //工作在1T模式 
	TMOD &= 0xF0; // 设置定时器0为工作模式0 16位定时器模式，工作模式工作在0
	TH0 = times >> 8;
	TL0 = (times & 0xFF);
	
	TR0 = 1; // 打开定时器 0
	ET0 = 1; // 打开定时器 0 中断

}

//定时器2 nus触发一次
void timer2_init(u16 nus) // 定时器0初始化
{
	u16 times;
	
	times = primary_value(nus);
	AUXR |= 0x04; //工作在1T模式 
	
	TMOD &= 0xF0; // 设置定时器2为工作模式0 16位定时器模式，工作模式工作在0
	T2H = times >> 8;
	T2L = (times & 0xFF);
	
	AUXR |= 0x10;                   //定时器2开始计时 
  IE2 |= 0x04;                    //开定时器2中断
}


//定时器0
void tm0_isr() interrupt 1
{

}

void tm2_isr() interrupt 12           //中断入口
{
	//systick
	TaskRemarks();
}
