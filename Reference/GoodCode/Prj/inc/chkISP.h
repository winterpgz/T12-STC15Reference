/*
˵��: ʵ�������в�ͣ���Զ������أ��޷�����ʱ�������
ʹ�÷���:
void UART_Init()
{
	PCON|=0x80;
	TMOD|=0x20;
	SCON=0x50;
	TH1=RELOAD;
	TL1=RELOAD;
	TR1=1;
	ES=1;
	EA=1;
}

void UART_Interrupt (void) interrupt 4 
{
    UCHAR Temp;          //������ʱ���� 
   
	if(RI)                        //�ж��ǽ����жϲ���
	{
		RI=0;                      //��־λ����
		Temp=SBUF;                 //���뻺������ֵ
		P1=Temp;                   //��ֵ�����P1�ڣ����ڹ۲�
		SBUF=Temp;                 //�ѽ��յ���ֵ�ٷ��ص��Զ�
		#ifdef __CHKISP_H__
		ISPCheck( Temp);
		#endif
	}
	if(TI)                        //����Ƿ��ͱ�־λ������
		TI=0;
} 
*/
#ifndef __CHKISP_H__
#define __CHKISP_H__
sfr IAP_CONTR   =   0xC7;   //0000,x000 EEPROM���ƼĴ���
void ISPCheck(unsigned char tmp)
{
	/*����: ����Ƿ���Ҫ��λ����ISP����ģʽ*/
	unsigned char code isp_comm[16]={0x12,0x34,0x56,0x78,0x90,0xab,0xcd,0xef,0x12,0x34,0x56,0x78,0x90,0xab,0xcd,0xef};
	static unsigned char isp_counter=0;
	unsigned char t, i, j, k;
	

	if(tmp != isp_comm[isp_counter++]) 
	{
		isp_counter = 0;  
		return;
	}
	else if (isp_counter > 15) 
	{
		EA = 0;
		//��ʱһ�룬��λ��ϵͳISP �������ĳ��� ����Ҫ����mcu�޸�
		t = 3;
		while(--t)
		{
			i = 84;
			j = 247;
			k = 23;
			do
			{
				do
				{
					while (--k);
				} while (--j);
			} while (--i);
		}
		//���û�Ӧ�ó�����(AP ��)�����λ���л���ϵͳISP ��س�������ʼִ�г���
		IAP_CONTR = 0x60; 
	}   
}
#endif