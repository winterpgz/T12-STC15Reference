#ifndef __UARTCONFIG_H__
#define __UARTCONFIG_H__

extern unsigned char uartDataSize; //串口已读取数据长度
extern unsigned char uartbuf[16]; //串口已读取数据长度

void UartInit(void);
void UART_SendByte(char byte);
void UART_SendBuf(unsigned char *buf,unsigned char len);
void UART_SendString(char *str);

#endif