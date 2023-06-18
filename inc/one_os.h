//=========================================简易任务调度器==============================================
#ifndef __ONE_OS_H
#define __ONE_OS_H

typedef struct Tasks
{
	unsigned char TaskNumber; //任务的编号 
	unsigned char Run;    // 0表示任务不运行，1表示任务运行
  unsigned int Timer;   // 任务执行间隔时间
  unsigned int ItvTime; // 数值上等于Timer
  void (*TaskHook)(void); //任务函数
}Tasks;  

extern volatile unsigned long os_sec;
extern volatile unsigned short  os_msec;
void TaskProcess(void);
void TaskRemarks(void);

#define idle_mode()		(PCON |= 0x01)

#define power_down()	(PCON |= 0x02)

#endif
