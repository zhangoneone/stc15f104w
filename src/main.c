#include "stc15.h"
#include "typedef.h"
#include "timer.h"
#include "one_os.h"
//P30 P31 P32 P33 P34 P35
//����P30��rx��P31��tx

void pla_init()
{
	timer0_init(10);
	timer2_init(1000);
	EA = 1; // �����ж�
}

void main()
{
	pla_init();
	for(;;) {
		TaskProcess();
	}
	return;
}