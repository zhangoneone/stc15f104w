#include "stc15.h"
#include "typedef.h"
#include "timer.h"
#include "one_os.h"
//P30 P31 P32 P33 P34 P35
//其中P30是rx，P31是tx

void pla_init()
{
	timer0_init(10);
	timer2_init(1000);
	EA = 1; // 打开总中断
}

void main()
{
	pla_init();
	for(;;) {
		TaskProcess();
	}
	return;
}