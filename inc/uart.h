
/*------------------------------------------------------------------*/
/* --- STC MCU International Limited -------------------------------*/
/* --- STC 1T Series MCU RC Demo -----------------------------------*/
/* --- Mobile: (86)13922805190 -------------------------------------*/
/* --- Fax: 86-0513-55012956,55012947,55012969 ---------------------*/
/* --- Tel: 86-0513-55012928,55012929,55012966 ---------------------*/
/* --- Web: www.GXWMCU.com -----------------------------------------*/
/* --- QQ:  800003751 ----------------------------------------------*/
/* If you want to use the program or the program referenced in the  */
/* article, please specify in which data and procedures from STC    */
/*------------------------------------------------------------------*/


#ifndef __UART_H
#define __UART_H	 

#include	"config.h"
#include "typedef.h"

#define	COM_RX1_Lenth	8


typedef struct uart_fifo_t{
	uint16 rri, rwi, rct;
	uint8	 rbuf[COM_RX1_Lenth];
} uart_fifo_t;

void uart_init();
char putchar(char c);
char _getkey(void);
char getchar(void);
int puts(const char *s);
char *gets(char *s, int n);
uint8 uart_fifo_count(void);

#endif

