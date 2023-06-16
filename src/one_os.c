#include "stc15.h"
#include "typedef.h"
#include "timer.h"
#include "one_os.h"
#include <rtx51tny.h>
#define  TASK_MAX(task)   (sizeof(task)/sizeof(task[0]))
#define  pwm		P32
void task1()
{
	static u16 period = 10; //100hz
	static u16 duty = 7;
	static u16 counter = 0;
	
	counter++;
	counter %= period;
	
	if (counter < duty) {
		pwm = 1;
	}
	else
		pwm = 0;
}

void task2()
{
	P33 = 0;
}

static Tasks task[]=   
{   
	{0,1,1,1,task1},
	{1,1,1000,1000,task2},
};

void TaskHangup(unsigned char Task_Num)//����������������������������
{ 
	task[Task_Num].Run=0;
}

void TaskRecovery(unsigned char Task_Num)//����ָ������������������������
{
	task[Task_Num].Run=1;
}

void TaskRemarks(void) //���ڶ�ʱ���ж�����
{
	unsigned char i;
	for (i=0; i<TASK_MAX(task); i++)          
	{
		if (task[i].Timer)          
		{
			task[i].Timer--;        
			if (task[i].Timer == 0)
			{
					task[i].Timer = task[i].ItvTime;
					task[i].Run = 1;           
			}
		}
	}
}

void TaskProcess(void)//�������while(1)ѭ������
{
  unsigned char  i;
	for (i=0; i<TASK_MAX(task); i++) {         
		if (task[i].Run) {
             task[i].Run = 0;     
			 task[i].TaskHook();  
    }
  }   
	idle_mode();//����͹���ģʽ		
}
