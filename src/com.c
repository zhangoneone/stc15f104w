#include "com.h"
#include "stc15.h"
#include "typedef.h"
#include "timer.h"
#include "one_os.h"
#include "config.h"
#include "delay.h"
#include <stdio.h>
#include "uart.h"

/*

1 byte command
6 bytes data
1 bytes checksum

cmd: 5 bits op, 3 bits data size
data:
checksum: sum % 0xFF

*/
#define	CMD													(0)
#define	DATA_BEGIN									(1)
#define	CHECKSUM										(7)
#define	COM_SIZE										(8)
#define OPS(cmd)										(cmd >> 3)
#define DSIZE(cmd)									(cmd & 0x07)
#define MKCMD(ops, dsize)						((ops << 3) | (dsize))
#define	MKPKG(pkg, cmd, d, sum)		  

#define	CMD_LOOP_BACK								0x0
#define	CMD_OS_TIME									0x1
#define	CMD_PWM_DUTY								0x2
#define	CMD_PWM_FRE									0x3


static u8 checksum(const u8 *var)
{
	u8 i = 0;
	u16 sum = 0;
	for (; i < COM_SIZE - 1; i++) {
		sum += var[i];
	}
	
	return (sum % (0xFFU));
}
/*
00 00 01 02 03 04 05 0f

*/
static u8 com_parse(u8 *var)
{
	u8 ops, dsize, sum;
	
	ops = OPS(var[CMD]);
	dsize = DSIZE(var[CMD]);
	sum = checksum(var);

	if(sum != var[CHECKSUM]) {
		printf("数据包错误\r\n");
		return 0;
	}
	
	switch(ops) {
		case CMD_LOOP_BACK:
			printf("%c%c%c%c%c%c%c%c", 
							var[0], var[1], var[2], var[3],
							var[4], var[5], var[6], var[7]);
		break;
		
		case CMD_OS_TIME:
			printf("系统时间:%ld.%u\r\n", os_sec, os_msec);
		break;
		
		case CMD_PWM_DUTY:
			printf("pwm占空比是:7:3\r\n");
		break;
		
		case CMD_PWM_FRE:
			printf("pwm频率是:100Hz\r\n");
		break;
		
		default:
			printf("命令错误\r\n");
		break;
	}
	
	return 0;
}

void com_task()
{
	u8 pkg_size = 0;
	u8 com[COM_SIZE];
	
	//扫描uart缓存的数据
	if(uart_fifo_count() < COM_SIZE) {
		return;
	} else {
		for (pkg_size = 0; pkg_size < COM_SIZE; pkg_size++)
			com[pkg_size] = getchar();
		
		//收集满一次完整数据，处理
		com_parse(com);
	}
}