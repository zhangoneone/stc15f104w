//=========================================�������������==============================================
#ifndef __ONE_OS_H
#define __ONE_OS_H

typedef struct Tasks
{
	unsigned char TaskNumber; //����ı�� 
	unsigned char Run;    // 0��ʾ�������У�1��ʾ��������
  unsigned int Timer;   // ����ִ�м��ʱ��
  unsigned int ItvTime; // ��ֵ�ϵ���Timer
  void (*TaskHook)(void); //������
}Tasks;  

extern volatile unsigned long os_sec;
extern volatile unsigned short  os_msec;
void TaskProcess(void);
void TaskRemarks(void);

#define idle_mode()		(PCON |= 0x01)

#define power_down()	(PCON |= 0x02)

#endif
