#include "stc15.h"
#include "typedef.h"
#include "timer.h"
#include "one_os.h"
#include <rtx51tny.h>
#include <stdio.h>
#define  TASK_MAX(task)   (sizeof(task)/sizeof(task[0]))
volatile unsigned long os_sec = 0;
volatile unsigned short  os_msec = 0;

extern void pwm_task();
extern void com_task();
static Tasks task[]=   
{   
	{0,1,1,1,pwm_task},
	{1,1,50,50,com_task},
};

void TaskRemarks(void) //放在定时器中断里面
{
	unsigned char i;
	
	os_msec++;
	if(os_msec == 999) {
		os_msec = 0;
		os_sec++;
	}
	
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

void TaskProcess(void)//放在你的while(1)循环里面
{
  unsigned char  i;
	for (i = 0; i<TASK_MAX(task); i++) {         
		if (task[i].Run) {
             task[i].Run = 0;     
			 task[i].TaskHook();  
    }
  }   
	idle_mode();//进入低功耗模式		
}
