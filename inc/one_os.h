//=========================================�������������==============================================

typedef struct Tasks
{
	unsigned char TaskNumber; //����ı�� 
	unsigned char Run;    // 0��ʾ�������У�1��ʾ��������
  unsigned int Timer;   // ����ִ�м��ʱ��
  unsigned int ItvTime; // ��ֵ�ϵ���Timer
  void (*TaskHook)(void); //������
}Tasks;  
void TaskHangup(unsigned char Task_Num);
void TaskRecovery(unsigned char Task_Num);
void TaskProcess(void);
void TaskRemarks(void);

#define idle_mode()		(PCON |= 0x01)

#define power_down()	(PCON |= 0x02)
