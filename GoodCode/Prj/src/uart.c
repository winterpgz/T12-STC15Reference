/*
说明: 对配置信息进行设置操作通过串口
*/

#include "uart.h"
#include <REG52.H>
#include <string.h>
#include "chkISP.h"

//define baudrate const
//BAUD = 65536 - SYSclk/3/BAUDRATE/M (1T:M=1; 12T:M=12)
//NOTE: (SYSclk/3/BAUDRATE) must be greater than 98, (RECOMMEND GREATER THAN 110)
//#define BAUD  0xF400                    // 1200bps @ 11.0592MHz
//#define BAUD  0xFA00                   // 2400bps @ 11.0592MHz
//#define BAUD  0xFD00                   // 4800bps @ 11.0592MHz
//#define BAUD  0xFE80                    // 9600bps @ 11.0592MHz
//#define BAUD  0xFF40                    //19200bps @ 11.0592MHz
//#define BAUD  0xFFA0    //38400bps @ 11.0592MHz
//#define BAUD  0xEC00                   // 1200bps @ 18.432MHz
//#define BAUD  0xF600                    // 2400bps @ 18.432MHz
//#define BAUD  0xFB00                   // 4800bps @ 18.432MHz
//#define BAUD  0xFD80                   // 9600bps @ 18.432MHz
//#define BAUD  0xFEC0                    //19200bps @ 18.432MHz
//#define BAUD  0xFF60                    //38400bps @ 18.432MHz
//#define BAUD  0xE800                    // 1200bps @ 22.1184MHz
//#define BAUD  0xF400                    // 2400bps @ 22.1184MHz
//#define BAUD  0xFA00                   // 4800bps @ 22.1184MHz
#define BAUD  0xFD00                   // 9600bps @ 22.1184MHz
//#define BAUD  0xFE80                    //19200bps @ 22.1184MHz
//#define BAUD  0xFF40                    //38400bps @ 22.1184MHz
//#define BAUD  0xFF80                    //57600bps @ 22.1184MHz

//#define BAUD  0xFB83                   // 9600bps @ 22.1184MHz

sfr AUXR = 0x8E;
sbit RXB = P3^0;                            //define UART TX/RX port
sbit TXB = P3^1;

typedef bit BOOL;
typedef unsigned char BYTE;
typedef unsigned int WORD;

BYTE TBUF,RBUF;
BYTE TDAT,RDAT;
BYTE TCNT,RCNT;
BYTE TBIT,RBIT;
BOOL TING,RING;
BOOL TEND,REND;


unsigned char uartDataSize; //串口已读取数据长度
unsigned char uartbuf[16];  //串口数据缓存

void UART_SendByte(char byte)
{
    TEND = 0;
    TBUF = byte;
    TING = 1;
    while (!TEND);
}

void UART_SendBuf(unsigned char *buf,unsigned char len)
{
    unsigned char i;

    for (i=0; i< len; i++)
    {
        UART_SendByte(buf[i]);
    }
}

void UART_SendString(char *str)
{
    while (*str)
    {
        UART_SendByte(*(str++));
    }
}

void UART_Interrupt(void)
{
    /*功能; 串口中断程序 模拟*/
    if (REND)                       //判断是接收中断产生
    {
        REND=0;                      //标志位清零
        uartbuf[uartDataSize++ & 0x0f] = RBUF;  //读入缓冲区的值
        //SBUF=Temp; //把接收到的值再发回电脑端
#ifdef __CHKISP_H__
        ISPCheck( RBUF);
#endif
    }
}

//initial UART module variable
void UartInit(void)
{
    //TMOD |= 0x00;                          //timer0 in 16-bit auto reload mode
    //AUXR = AUXR | 0x80;                          //timer0 working at 1T mode
    TL0 = BAUD;
    TH0 = BAUD>>8;                        //initial timer0 and set reload value
    TR0 = 1;                                //tiemr0 start running
    ET0 = 1;                                //enable timer0 interrupt
    //PT0 = 1;                                //低中断优先级
    //EA = 1;                                 //open global interrupt switch

    TING = 0;
    RING = 0;
    TEND = 1;
    REND = 0;
    TCNT = 0;
    RCNT = 0;

    uartDataSize = 0;
}

//Timer interrupt routine for UART
void Timer0_Interrupt(void) interrupt 1 using 1
{
    if (RING)
    {
        if (--RCNT == 0)
        {
            RCNT = 3;                   //reset send baudrate counter
            if (--RBIT == 0)
            {
                RBUF = RDAT;              //save the data to RBUF
                RING = 0;                 //stop receive
                REND = 1;                 //set receive completed flag
                UART_Interrupt();         //模拟调用中断处理程序
            }
            else
            {
                RDAT >>= 1;
                if (RXB) RDAT |= 0x80;  //shift RX data to RX buffer
            }
        }
    }
    else if (!RXB)
    {
        RING = 1;                         //set start receive flag
        RCNT = 4;                         //initial receive baudrate counter
        RBIT = 9;                         //initial receive bit number (8 data bits + 1 stop bit)
    }
    if (--TCNT == 0)
    {
        TCNT = 3;                         //reset send baudrate counter
        if (TING)                         //judge whether sending
        {
            if (TBIT == 0)
            {
                TXB = 0;                  //send start bit
                TDAT = TBUF;      //load data from TBUF to TDAT
                TBIT = 9;                //initial send bit number (8 data bits + 1 stop bit)
            }
            else
            {
                TDAT >>= 1;             //shift data to CY
                if (--TBIT == 0)
                {
                    TXB = 1;
                    TING = 0;           //stop send
                    TEND = 1;           //set send completed flag
                }
                else
                {
                    TXB = CY;           //write CY to TX port
                }
            }
        }
    }
}