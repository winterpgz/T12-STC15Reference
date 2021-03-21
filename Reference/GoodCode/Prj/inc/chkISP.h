/*
说明: 实现运行中不停电自定义下载，无仿真器时方便调试
使用方法:
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
    UCHAR Temp;          //定义临时变量 
   
	if(RI)                        //判断是接收中断产生
	{
		RI=0;                      //标志位清零
		Temp=SBUF;                 //读入缓冲区的值
		P1=Temp;                   //把值输出到P1口，用于观察
		SBUF=Temp;                 //把接收到的值再发回电脑端
		#ifdef __CHKISP_H__
		ISPCheck( Temp);
		#endif
	}
	if(TI)                        //如果是发送标志位，清零
		TI=0;
} 
*/
#ifndef __CHKISP_H__
#define __CHKISP_H__
sfr IAP_CONTR   =   0xC7;   //0000,x000 EEPROM控制寄存器
void ISPCheck(unsigned char tmp)
{
	/*功能: 检测是否需要软复位进入ISP下载模式*/
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
		//延时一秒，软复位到系统ISP 程序区的程序 这里要根据mcu修改
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
		//从用户应用程序区(AP 区)软件复位并切换到系统ISP 监控程序区开始执行程序
		IAP_CONTR = 0x60; 
	}   
}
#endif