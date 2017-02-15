#include<STC15F104E_PLUS.H>
#include <stdio.h>
#include <string.h>
#include <intrins.h>
#include "main.h"
#include "delay.h"
#include "uart.h"

//定义串转并端口可位寻址
//UCHAR bdata PORT1, PORT2;

//端口定义
sbit RA1=P3^3; //电容充放电
sbit RA0=P3^2; //比较器电平
sbit CTRL=P3^5; //T12加热控制

sbit LED=P3^4;  //led控制 调试使用

bit bSendData; //发送数据到串口
unsigned int ADCResult;	//AD返回值

SysConfig config; //系统参数
SPid pid; // PID 控制结构

unsigned char high_time, max_time,count;//占空比调节参数
unsigned int tickCount; //系统计时器

int pTerm, dTerm, iTerm, drive;

void main (void)
{
    //总中断开启
    EA = 1;

    //定时器初始化 定时器0用于AD计数与串口操作 定时器1用于系统计数pwm调整等
    //设置定时器0, 1工作在1T模式
    AUXR = 0xC0;
    //置定时器0, 1 工作模式0 16位自动重装定时器
    TMOD = 0x0;

    //将T12加热控制口设置为推挽输出 最高输出20ma 否则需要上拉电阻
    P3M1 = P3M1 & 0x1F;
    P3M0 = P3M0 | 0x20;
    //关闭T12加热
    CTRL = 0;

    /*功能: 初始化AD相关*/
    RA1 = 1; //电容放电
    //将电容放电口设置为推挽输出 最高输出20ma 否则需要上拉电阻
    P3M1 = P3M1 & 0x30;
    P3M0 = P3M0 | 0x08;

    while(1)
    {
        //串口初始化
        UartInit();
        //等待串口命令
        while( uartDataSize < 16);
        UART_Command();
        if (TR1)
        {
            //工作一定时间后停止工作
            bSendData = 0;
            while (tickCount < 30000)
            {
                if (bSendData)
                {
                    bSendData = 0;
                    //向串口发送数据
                    UartInit();
                    UART_SendByte(0xff);
                    UART_SendByte(0x55);
                    UART_SendByte(0xc);
                    UART_SendByte(0xff);
                    UART_SendBuf( (unsigned char *)&ADCResult, 2);
                    UART_SendByte( high_time);	//加热时间
                    UART_SendByte( max_time);
                    UART_SendBuf( (unsigned char *)&pTerm, 2);
                    UART_SendBuf( (unsigned char *)&iTerm, 2);
                    UART_SendBuf( (unsigned char *)&dTerm, 2);
                    UART_SendBuf( (unsigned char *)&drive, 2);

                }
            }
            StopWork();
        }
    }
}

void StartWork(void)
{
    /*功能: 开始工作*/
    tickCount = 0;
    count = 0;
    max_time = MAX_PWM_TIME;
    high_time = MAX_PWM_TIME; //0-100
    memset ( &pid,0,sizeof(SPid));
    /*pid.pGain = 40; // Set PID Coefficients  比例常数 Proportional Const
    pid.iGain = 1;    //积分常数 Integral Const
    pid.dGain = 50;   //微分常数 Derivative Const
    */
    pid.pGain = config.pGain; // Set PID Coefficients  比例常数 Proportional Const
    pid.iGain = config.iGain;    //积分常数 Integral Const
    pid.dGain = config.dGain;   //微分常数 Derivative Const

    //定时器1初始化
    //2毫秒@22.1184MHz 定时器时钟1T模式	16位自动重装定时器
    TL1 = 0x33;		//设置定时初值
    TH1 = 0x53;		//设置定时初值
    //1毫秒@33.1776MHz
    //TL1 = 0x66;		//设置定时初值
    //TH1 = 0x7E;		//设置定时初值

    TF1 = 0;		//清除TF1标志
    PT1	= 1;        //高中断优先级
    ET1 = 1;        //允许中断
    TR1 = 1;		//定时器1开始计时
}

void StopWork(void)
{
    /*功能: 停止工作*/
    TR1 = 0;
    //关闭T12加热
    CTRL = 0;
    LED = 1;	  //关闭led
}

void UART_Command(void)
{
    /*功能; 执行串口相关命令*/
    UCHAR cmd;

    uartDataSize = 0;

    //检测命令头 FF 55 00 00 ... 55 FF
    if (!((uartbuf[0] == 0xFF) && (uartbuf[1] == 0x55)	&& (uartbuf[14] == 0x55) && (uartbuf[15] == 0xFF)))
    {
        //命令头错误
        return;
    }

    cmd = uartbuf[2];
    switch ( cmd)
    {
    case 0:  //开始加热
        //系统参数
        memcpy(&config, &uartbuf[3], sizeof(SysConfig));
        uartbuf[2] = 0;	 //数据大小
        uartbuf[3] = 0;	 //命令ID
        UART_SendBuf(uartbuf, 4); //确认加热
        StartWork();
        break;
    case 1: //停止加热
        uartbuf[2] = 0;	 //数据大小
        uartbuf[3] = 1;
        UART_SendBuf(uartbuf, 4);
        StopWork();
        break;
    }
}

void GetADCResult(void)
{
    /*
    功能:
    读取ADC并返回结果 模式 检测端口电平
    通过LM258比较器实现AD
    定时器溢出时间计算 单位ms 设系统时钟 SYSclk = 22118400
    1T模式
    1000/(22118400/65536)	  = 2.96
    12T模式
    1000/(22118400/12/65536)  = 35.55
    Multisim 仿真结果
    103 充电到2.5v 约0.952ms  T1下更高速度
    223 充电到2.5v 约2.104ms  T1下更高精度
    104 充电到2.5v 约9.947ms  T12模式
    */

    //检测基准电压
    //EA = 0;   //关闭全部中断
    TR0=0;	  //关闭计时器
    ET0 = 0; //不允许中断

    TF0=0;    //清溢出位
    TH0=0x0;  //给定初值
    TL0=0x0;
    RA1 = 0; //电容充电
    TR0=1; //开始计数
    //等待电容电压与基准电压相同或者定时器溢出
    while(!RA0 && !TF0);

    TR0=0;	  //关闭计时器
    //EA = 1;   //开启全部中断
    RA1 = 1; //电容放电

    if (TF0)
    {
        //定时器溢出 基准电压太高? 电容充电电压太低? 电容太大?
        //ADCResult = 0xfffe;
        ADCResult = 0x400 - config.ad_zeroValueFix;
    }
    else
    {
        //只取计数值高10位
        ADCResult = ((unsigned int)TH0 << 2) | (TL0 >> 6);
        /*
        调零运算
        因为运放有失调电压lm358为3mv, 所以单电源需要调零. 因为3脚接10K电阻所以也会有压差.
        可以在T12烙铁头为常温时的值作为0值. 由 T=T0 温差电势=0
        */
        if (ADCResult >= config.ad_zeroValueFix)
            ADCResult = ADCResult-config.ad_zeroValueFix;
        else
            ADCResult = 0;
    }
    //delayms( 2); //等待电容放电完成 如果不是连续调用可以注释掉

    //放电延迟 数值范围
    //1ms 4
    //2ms 5
    //3ms 6
    //5ms 7
    //20ms 9
    //100ms 7

    //输入电压=参考电压*输入电压计时器值/参考电压计时器值
    //V1=Vref*T2/T1
}

int UpdatePID(int error, int position)
{
    /*功能: PID计算*/
    pTerm = pid.pGain * error;   // calculate the proportional term

    // calculate the integral state with appropriate limiting
    pid.iState += error;
    // Maximum and minimum allowable integrator state
    if (pid.iState < 0) pid.iState = 0;
    if (pid.iState > 500) pid.iState = 100;
    //else if (pid.iState < pid.iMin) pid.iState = pid.iMin;

    iTerm = (pid.iGain * pid.iState) / 100;  // calculate the integral term

    dTerm = pid.dGain * (pid.dState - position) ;
    pid.dState = position;

    //drive = pTerm + dTerm + iTerm;
    drive = pTerm + iTerm + dTerm;

    if (drive<0)
        return 0;
    else if (drive>100)
        return 100;
    else
        return drive;
}

void Timer1_Interrupt(void) interrupt 3
{
    /*功能: 用于计时,PWM等*/
    int error;

    tickCount++;
    count++; //PWM计数器

    //PWM与PID控制
    if (count<=max_time)
    {
        //PWM周期
        if (count <= high_time)
        {
            CTRL = 1; //加热
            LED = 0;	  //开启led
        }
        else
        {
            CTRL = 0; //停止
            LED = 1;	  //关闭led
        }
    }
    else
    {
        TR1 = 0; //关闭定时器
        CTRL = 0; //停止加热
        LED = 1;	  //关闭led

        if (high_time > 98)
            delayms(4); //等待T12上的电容泄放干净

        GetADCResult();	 //读取AD值

        //PID计算
        max_time = MAX_PWM_TIME;
        error = (int)config.set_temper-(int)ADCResult;
        if(error>40)  //设置的温度比实际的温度是否是大于n度
        {
            //全功率加热 500ms
            max_time = 250;
            high_time = 250;
        }
        else if(error<-40)
        {
            //不加热
            max_time = 100;
            high_time = 0;
        }
        else  //如果是在n度范围内，则运行PID计算
        {
            high_time = UpdatePID(error,  ADCResult);
        }


        bSendData = 1;

        count = 0;
        TR1 = 1; //开启定时器
    }
}