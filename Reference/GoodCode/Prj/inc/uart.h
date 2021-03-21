#ifndef __UARTCONFIG_H__
#define __UARTCONFIG_H__

extern unsigned char uartDataSize; //�����Ѷ�ȡ���ݳ���
extern unsigned char uartbuf[16]; //�����Ѷ�ȡ���ݳ���

void UartInit(void);
void UART_SendByte(char byte);
void UART_SendBuf(unsigned char *buf,unsigned char len);
void UART_SendString(char *str);

#endif